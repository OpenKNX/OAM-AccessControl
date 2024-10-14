#include "Fingerprint.h"

Fingerprint::Fingerprint(uint32_t overridePassword)
    : _finger(Adafruit_Fingerprint(&SCANNER_SERIAL, overridePassword))
{
    _delayMs = _delayCallbackDefault;
}

Fingerprint::Fingerprint(DelayCalback delayCallback, uint32_t overridePassword)
    : _finger(Adafruit_Fingerprint(&SCANNER_SERIAL, overridePassword))
{
    _delayMs = delayCallback;
}

bool Fingerprint::start()
{
    scannerReady = false;

    _finger.begin(57600, SCANNER_SERIAL_RX_PIN, SCANNER_SERIAL_TX_PIN);

    // uint32_t timeoutTimer = delayTimerInit();
    // while(!delayCheck(timeoutTimer, 500) && _finger.verifyPassword() != FINGERPRINT_OK)
    //     _delayMs(10);

    _delayMs(500);

    switch (_finger.verifyPassword())
    {
        case FINGERPRINT_OK:
            logInfoP("Found fingerprint sensor!");
            break;
        case FINGERPRINT_PASSFAIL:
            logErrorP("Fingerprint password invalid!");
            return false;
        default:
            logErrorP("Fingerprint scanner not found!");
            return false;
    }

    scannerReady = true;
    return true;
}

void Fingerprint::logSystemParameters()
{
    logInfoP("System parameters:");
    logIndentUp();
    logInfoP("Status register: %d", _finger.status_reg);
    logInfoP("System identifier code: %d", _finger.system_id);
    logInfoP("Finger library size: %d", _finger.capacity);
    logInfoP("Security level: %d", _finger.security_level);
    logInfoP("Device address: %d", _finger.device_addr);
    logInfoP("Data packet size: %d", _finger.packet_len);
    logInfoP("Baud settings: %d", _finger.baud_rate);
    logInfoP("Stored templates: %d", getTemplateCount());
#ifdef OPENKNX_DEBUG
    _listTemplates();
#endif
    logIndentDown();
}

void Fingerprint::close()
{
    _finger.close();
    scannerReady = false;
}

std::string Fingerprint::logPrefix()
{
    return "Fingerprint";
}

bool Fingerprint::setLed(State state)
{
    if (!scannerReady)
        return false;

    switch (state)
    {
        case None:
            return _finger.LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_WHITE, 0) == FINGERPRINT_OK;
        case ScanFinger:
            return _finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE, 0) == FINGERPRINT_OK;
        case ScanMatch:
        case Success:
            return _finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_GREEN, 0) == FINGERPRINT_OK;
        case ScanMatchNoAction:
        case DeleteNotFound:
            return _finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_YELLOW, 0) == FINGERPRINT_OK;
        case ScanNoMatch:
        case Failed:
        case Locked:
            return _finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED, 0) == FINGERPRINT_OK;
        case WaitForFinger:
            return _finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 20, FINGERPRINT_LED_BLUE, 0) == FINGERPRINT_OK;
        case RemoveFinger:
            return _finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 20, FINGERPRINT_LED_WHITE, 0) == FINGERPRINT_OK;
        case EnrollCreateModel:
            return _finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 20, FINGERPRINT_LED_GREEN, 0) == FINGERPRINT_OK;
        case Busy:
            return _finger.LEDcontrol(FINGERPRINT_LED_FLASHING, 20, FINGERPRINT_LED_GREEN, 0) == FINGERPRINT_OK;
        default:
            logErrorP("Fingerprint::setLed: ERROR: Invalid state.");
            return false;
    }
}

bool Fingerprint::setLed(uint8_t color, uint8_t control, uint8_t speed, uint8_t count)
{
    return _finger.LEDcontrol(control, speed, color, count) == FINGERPRINT_OK;
}

bool Fingerprint::hasFinger()
{
    if (!scannerReady)
        return false;

    return _finger.getImage() == FINGERPRINT_OK;
}

uint16_t Fingerprint::getTemplateCount()
{
    if (!scannerReady)
        return 0;

    uint8_t p = _finger.getTemplateCount();
    if (p != FINGERPRINT_OK)
        return 0;

    return _finger.templateCount;
}

bool Fingerprint::_listTemplates()
{
    uint8_t p = _finger.getTemplateIndices();
    if (p == FINGERPRINT_OK)
    {
        logDebugP("Stored template locations:");
        logIndentUp();

        for (uint16_t i = 0; i < _finger.templateCount; ++i)
        {
            logDebugP("%u", _finger.templates[i]);
        }

        logIndentDown();
        return true;
    }
    else
    {
        logErrorP("Error getting template locations (%u).", p);
    }

    return false;
}

bool Fingerprint::hasLocation(uint16_t location)
{
     if (!scannerReady)
        return false;

    uint8_t p = _finger.getTemplateIndices();
    if (p != FINGERPRINT_OK)
        return false;

    for (size_t i = 0; i < _finger.templateCount; ++i)
    {
        if (_finger.templates[i] == location)
            return true;
    }

    return false;
}

uint16_t* Fingerprint::getLocations()
{
     if (!scannerReady)
        return nullptr;

    uint8_t p = _finger.getTemplateIndices();
    if (p != FINGERPRINT_OK)
        return nullptr;

    return _finger.templates;
}

uint16_t Fingerprint::getNextFreeLocation()
{
    if (!scannerReady)
        return -1;

    _finger.getTemplateIndices();

    uint8_t lastIndex = 0;
    for (int i = 0; i < _finger.templateCount; i++)
    {
        uint8_t index = _finger.templates[i];
        if (index - lastIndex > 1)
            return lastIndex + 1;

        lastIndex++;
    }

    return lastIndex <= _finger.capacity ? lastIndex : -1;
}

Fingerprint::FindFingerResult Fingerprint::findFingerprint()
{
    FindFingerResult findFingerResult;
    if (!scannerReady)
        return findFingerResult;

    logDebugP("Find finger:");
    logIndentUp();

    setLed(ScanFinger);

    logDebugP("Templating...");
    uint8_t p = _finger.image2Tz();
    if (p == FINGERPRINT_OK)
    {
        logDebugP("Templated");
    }
    else
    {
        switch (p)
        {
            case FINGERPRINT_IMAGEMESS:
                logDebugP("Image too messy");
                break;
            case FINGERPRINT_PACKETRECIEVEERR:
                logDebugP("Communication error");
                break;
            case FINGERPRINT_FEATUREFAIL:
                logDebugP("Could not find fingerprint features");
                break;
            case FINGERPRINT_INVALIDIMAGE:
                logDebugP("Invalid image");
                break;
            default:
                logDebugP("Unknown error");
                break;
        }

        logIndentDown();
        return findFingerResult;
    }

    _delayMs(10);

    logDebugP("Searching... ");
    p = _finger.fingerSearch();
    if (p == FINGERPRINT_OK)
    {
        //setLed(ScanMatch);

        u_int16_t location = _finger.fingerID;
        
#ifdef CALC_TEMPLATE_CHECKUM
        GetNotepadPageIndexResult getNotepadPageIndexResult = _getNotepadPageIndex(location);
        u_int8_t page = getNotepadPageIndexResult.page;
        u_int8_t index = getNotepadPageIndexResult.index;

        uint8_t content[32];
        _finger.readNotepad(page, content);

        uint8_t checksumBytes[2];
        checksumBytes[0] = content[index];
        checksumBytes[1] = content[index + 1];
        uint16_t checksum = ((unsigned short)checksumBytes[1] << 8) | (unsigned char)checksumBytes[0];
        logDebugP("page=%d, index=%d, checksumBytes=%d/%d, checksum=%d", page, index, checksumBytes[0], checksumBytes[1], checksum);
        findFingerResult.checksum = checksum;
#endif

        logDebugP("Match #%d with confidence %d", _finger.fingerID, _finger.confidence);

        findFingerResult.found = true;
        findFingerResult.location = location;

        logIndentDown();
        return findFingerResult;
    }
    else
    {
        findFingerResult.found = false;
        findFingerResult.location = 0;
#ifdef CALC_TEMPLATE_CHECKUM
        findFingerResult.checksum = 0;
#endif
    }

    logDebugP("No match");
    //setLed(ScanNoMatch);

    logIndentDown();
    return findFingerResult;
}

bool Fingerprint::createTemplate()
{
    if (!scannerReady)
        return false;

    logDebugP("Create template:");
    logIndentUp();

    u_int8_t p;
    for (u_int8_t i = 1; i < 7; i++)
    {
        setLed(WaitForFinger);

        if (i == 1)
        {
            logDebugP("Place finger on sensor...");
        }
        else
        {
            logDebugP("Place same finger again...");
        }

        logIndentUp();
        ulong start = delayTimerInit();
        while (true)
        {
            p = _finger.getImage();
            if (p == FINGERPRINT_OK)
            {
                logDebugP("Image taken");
                break;
            }

            switch (p)
            {
                case FINGERPRINT_NOFINGER:
                    if (delayCheck(start, 10000))
                    {
                        logDebugP("Cancel");
                        setLed(Failed);
                        logIndentDown();
                        logIndentDown();
                        return false;
                    }

                    logDebugP("Waiting");
                    break;
                case FINGERPRINT_IMAGEFAIL:
                    logDebugP("Imaging error");
                    setLed(Failed);
                    logIndentDown();
                    logIndentDown();
                    return false;
                default:
                    logDebugP("Other error");
                    setLed(Failed);
                    logIndentDown();
                    logIndentDown();
                    return false;
            }

            _delayMs(10);
        }
        logIndentDown();

        logDebugP("Templating...");
        logIndentUp();
        p = _finger.image2Tz(i);
        switch (p)
        {
            case FINGERPRINT_OK:
                logDebugP("Templated");
                break;
            case FINGERPRINT_IMAGEMESS:
                logDebugP("Imaging too messy");
                setLed(Failed);
                logIndentDown();
                logIndentDown();
                return false;
            case FINGERPRINT_FEATUREFAIL:
                logDebugP("Could not identify features");
                setLed(Failed);
                logIndentDown();
                logIndentDown();
                return false;
            case FINGERPRINT_INVALIDIMAGE:
                logDebugP("Image invalid");
                setLed(Failed);
                logIndentDown();
                logIndentDown();
                return false;
            default:
                logDebugP("Other error");
                setLed(Failed);
                logIndentDown();
                logIndentDown();
                return false;
        }
        logIndentDown();

        if (i < 6)
        {
            logDebugP("Remove finger");
            setLed(RemoveFinger);
            _delayMs(500);
            while (p != FINGERPRINT_NOFINGER)
            {
                p = _finger.getImage();
                 _delayMs(10);
            }
        }
    }

    setLed(EnrollCreateModel);
    _delayMs(1000);

    logDebugP("Creating model...");
    logIndentUp();
    p = _finger.createModel();
    switch (p)
    {
        case FINGERPRINT_OK:
            logDebugP("Created");
            setLed(Success);
            break;
        case FINGERPRINT_ENROLLMISMATCH:
            logDebugP("Prints did not match");
            setLed(Failed);
            break;
        case FINGERPRINT_PACKETRECIEVEERR:
            logDebugP("Packet receive error");
            setLed(Failed);
            break;
        default:
            logDebugP("Other error: %u", p);
            setLed(Failed);
            break;
    }
    _delayMs(1000);
    logIndentDown();

    logIndentDown();
    return p == FINGERPRINT_OK;
}

bool Fingerprint::retrieveTemplate(uint8_t templateData[])
{
    logDebugP("Receive template:");
    logIndentUp();
    uint8_t p = _finger.get_template_buffer(TEMPLATE_SIZE, templateData);
    logDebugP("Received");
    logIndentDown();

    return p == FINGERPRINT_OK;
}

bool Fingerprint::sendTemplate(uint8_t templateData[])
{
    logDebugP("Send template:");
    logIndentUp();
    uint8_t p = _finger.write_template_to_sensor(TEMPLATE_SIZE, templateData);
    _delayMs(1000); // needed, otherwise store might fail
    logDebugP("Sent");
    logIndentDown();

    return p == FINGERPRINT_OK;
}

#ifdef CALC_TEMPLATE_CHECKUM
bool Fingerprint::writeCrc(uint16_t location, uint8_t *templateData, uint32_t secret)
{
    logDebugP("Calculate and store CRC:");
    logIndentUp();

    uint8_t dataWithChecksum[TEMPLATE_SIZE + 4];
    memcpy(dataWithChecksum, templateData, TEMPLATE_SIZE);
    memcpy(dataWithChecksum + TEMPLATE_SIZE, &secret, sizeof(secret));

    uint16_t checksum = crc16(dataWithChecksum, TEMPLATE_SIZE + 4);

    uint8_t checksumBytes[2];
    checksumBytes[0] = checksum & 0xff;
    checksumBytes[1] = (checksum >> 8) & 0xff;

    GetNotepadPageIndexResult getNotepadPageIndexResult = _getNotepadPageIndex(location);
    uint8_t page = getNotepadPageIndexResult.page;
    uint8_t index = getNotepadPageIndexResult.index;
    logDebugP("page=%d index=%d checksumBytes=%d/%d checksum=%d... ", page, index, checksumBytes[0], checksumBytes[1], checksum);

    uint8_t content[32];
    _finger.readNotepad(page, content);

    content[index] = checksumBytes[0];
    content[index + 1] = checksumBytes[1];

    uint8_t p = _finger.writeNotepad(page, content) == FINGERPRINT_OK;
    logDebugP("Stored");

    logIndentDown();
    return p == FINGERPRINT_OK;
}
#endif

bool Fingerprint::loadTemplate(uint16_t location)
{
    if (!scannerReady)
        return false;

    logDebugP("Load model #%d:", location);
    logIndentUp();
    bool success = _finger.loadModel(location) == FINGERPRINT_OK;
    logDebugP("Loaded");
    logIndentDown();

    return success;
}

bool Fingerprint::storeTemplate(uint16_t location)
{
    if (!scannerReady)
        return false;

    logDebugP("Store model #%d:", location);
    logIndentUp();
    bool success = _finger.storeModel(location) == FINGERPRINT_OK;
    logDebugP("Stored");
    logIndentDown();

    return success;
}

bool Fingerprint::deleteTemplate(uint16_t location)
{
    if (!scannerReady)
        return false;

    if (!hasLocation(location))
    {
        setLed(DeleteNotFound);
        return false;
    }

    setLed(Busy);

    logDebugP("Fingerprint: Delete %d:", location);
    logIndentUp();
    bool success = _finger.deleteModel(location) == FINGERPRINT_OK;
    logDebugP("Deleted");
    logIndentDown();

    if (success)
    {
        setLed(Success);
    }
    else
    {
        setLed(Failed);
    }

    return success;
}

bool Fingerprint::setPassword(uint32_t newPasswort)
{
    if (!scannerReady)
        return false;

    setLed(Busy);

    logDebugP("Set standard password:");
    logIndentUp();
    bool success = _finger.setPassword(newPasswort) == FINGERPRINT_OK;
    logDebugP("Set");
    logIndentDown();

    if (success)
    {
        setLed(Success);
    }
    else
    {
        setLed(Failed);
    }

    return success;
}

bool Fingerprint::emptyDatabase()
{
    if (!scannerReady)
        return false;

    setLed(Busy);

    logDebugP("Fingerprint: Empty database:");
    logIndentUp();
    bool success = _finger.emptyDatabase() == FINGERPRINT_OK;
    logDebugP("Emptied");
    logIndentDown();

    if (success)
    {
        setLed(Success);
    }
    else
    {
        setLed(Failed);
    }

    return success;
}

bool Fingerprint::checkSensor()
{
    if (!scannerReady)
        return false;

    return _finger.checkSensor() == FINGERPRINT_OK;
}

#ifdef CALC_TEMPLATE_CHECKUM
Fingerprint::GetNotepadPageIndexResult Fingerprint::_getNotepadPageIndex(u_int16_t templateLocation)
{
    u_int16_t globalIndex = templateLocation * 2;
    u_int8_t page = floor(globalIndex / 32);
    u_int8_t index = globalIndex - page * 32;

    GetNotepadPageIndexResult getNotepadPageIndexResult;
    getNotepadPageIndexResult.page = page;
    getNotepadPageIndexResult.index = index;
    return getNotepadPageIndexResult;
}
#endif

void Fingerprint::_delayCallbackDefault(uint32_t period)
{
    delay(period);
}