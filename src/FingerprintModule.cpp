#include "FingerprintModule.h"

const std::string FingerprintModule::name()
{
    return "Fingerprint";
}

const std::string FingerprintModule::version()
{
    return MAIN_Version;
}

void FingerprintModule::setup()
{
    logInfoP("Setup fingerprint module");
    logIndentUp();

    initFlash();
    initFingerprintScanner();

    for (uint16_t i = 0; i < ParamFIN_VisibleActions; i++)
    {
        _channels[i] = new ActionChannel(i, finger);
        _channels[i]->setup();
    }

    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);
    digitalWrite(LED_RED_PIN, HIGH);

    pinMode(SCANNER_TOUCH_PIN, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(SCANNER_TOUCH_PIN), FingerprintModule::interruptDisplayTouched, FALLING);

    pinMode(TOUCH_LEFT_PIN, INPUT);
    pinMode(TOUCH_RIGHT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(TOUCH_LEFT_PIN), FingerprintModule::interruptTouchLeft, CHANGE);
    attachInterrupt(digitalPinToInterrupt(TOUCH_RIGHT_PIN), FingerprintModule::interruptTouchRight, CHANGE);

    logInfoP("Fingerprint start");
    finger.start();

    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, HIGH);
    finger.setLed(Fingerprint::State::Success);

    KoFIN_LedRingColor.valueNoSend((uint8_t)0, Dpt(5, 10));
    KoFIN_LedRingControl.valueNoSend((uint8_t)FINGERPRINT_LED_OFF, Dpt(5, 10));
    KoFIN_LedRingSpeed.valueNoSend((uint8_t)0, Dpt(5, 10));
    KoFIN_LedRingCount.valueNoSend((uint8_t)0, Dpt(5, 10));

    initResetTimer = delayTimerInit();
    logInfoP("Fingerprint module ready.");
    logIndentDown();
}

void FingerprintModule::initFingerprintScanner()
{
    uint32_t scannerPassword = _fingerprintStorage.readInt(FLASH_SCANNER_PASSWORD_OFFSET);
    logDebugP("Initialize scanner with password: %u", scannerPassword);
    finger = Fingerprint(delayCallback, scannerPassword);
}

void FingerprintModule::initFlash()
{
    _fingerprintStorage.init("fingerprint", FINGERPRINT_FLASH_OFFSET, FINGERPRINT_FLASH_SIZE);
    uint32_t magicWord = _fingerprintStorage.readInt(0);
    if (magicWord != FLASH_MAGIC_WORD)
    {
        logInfoP("Flash contents invalid:");
        logDebugP("Indentification code read: %u", magicWord);
        logIndentUp();

        uint8_t clearBuffer[FLASH_SECTOR_SIZE] = {};
        for (size_t i = 0; i < FINGERPRINT_FLASH_SIZE / FLASH_SECTOR_SIZE; i++)
            _fingerprintStorage.write(FLASH_SECTOR_SIZE * i, clearBuffer, FLASH_SECTOR_SIZE);
        _fingerprintStorage.commit();
        logDebugP("Flash cleared.");

        _fingerprintStorage.writeInt(0, FLASH_MAGIC_WORD);
        _fingerprintStorage.commit();
        logDebugP("Indentification code written.");

        logIndentDown();
    }
    else
        logInfoP("Flash contents valid.");
}

void FingerprintModule::interruptDisplayTouched()
{
    touched = true;
}

void FingerprintModule::interruptTouchLeft()
{
    KoFIN_TouchPcbButtonLeft.value(digitalRead(TOUCH_LEFT_PIN) == HIGH, DPT_Switch);
}

void FingerprintModule::interruptTouchRight()
{
    KoFIN_TouchPcbButtonRight.value(digitalRead(TOUCH_RIGHT_PIN) == HIGH, DPT_Switch);
}

void FingerprintModule::loop()
{
    if (delayCallbackActive)
        return;

    if (touched)
    {
        touched = false;
        logInfoP("Touched");

        KoFIN_Touched.value(true, DPT_Switch);

        unsigned long captureStart = delayTimerInit();
        while (!delayCheck(captureStart, CAPTURE_RETRIES_TOUCH_TIMEOUT))
        {
            if (finger.hasFinger())
            {
                Fingerprint::FindFingerResult findFingerResult = finger.findFingerprint();

                if (findFingerResult.found)
                {
                    logInfoP("Finger found in location %d", findFingerResult.location);
                    processScanSuccess(findFingerResult.location);
                }
                else
                {
                    finger.setLed(Fingerprint::ScanNoMatch);

                    logInfoP("Finger not found");
                    KoFIN_ScanSuccess.value(true, DPT_Switch);

                    KoFIN_ScanSuccessData.valueNoSend((uint32_t)0, Dpt(15, 1, 0)); // access identification code (unknown)
                    KoFIN_ScanSuccessData.valueNoSend(true, Dpt(15, 1, 1));        // detection error
                    KoFIN_ScanSuccessData.valueNoSend(false, Dpt(15, 1, 2));       // permission accepted
                    KoFIN_ScanSuccessData.valueNoSend(false, Dpt(15, 1, 3));       // read direction (not used)
                    KoFIN_ScanSuccessData.valueNoSend(false, Dpt(15, 1, 4));       // encryption (not used for now)
                    KoFIN_ScanSuccessData.value((uint8_t)0, Dpt(15, 1, 5));        // index of access identification code (not used)
                }

                resetLedsTimer = delayTimerInit();
                break;
            }
        }

        touched = false;
    }

    if (enrollRequestedTimer > 0 and delayCheck(enrollRequestedTimer, ENROLL_REQUEST_DELAY))
    {
        bool success = enrollFinger(enrollRequestedLocation);
        if (success)
            startSyncSend(enrollRequestedLocation, false); // model should still be loaded

        enrollRequestedTimer = 0;
        enrollRequestedLocation = 0;
    }

    if (initResetTimer > 0 && delayCheck(initResetTimer, INIT_RESET_TIMEOUT))
    {
        finger.setLed(Fingerprint::State::None);
        digitalWrite(LED_GREEN_PIN, LOW);
        initResetTimer = 0;
    }

    if (resetLedsTimer > 0 && delayCheck(resetLedsTimer, LED_RESET_TIMEOUT))
    {
        setLedDefault();
        resetLedsTimer = 0;
    }

    for (uint16_t i = 0; i < ParamFIN_VisibleActions; i++)
        _channels[i]->loop();

    if (syncRequestedTimer > 0 && delayCheck(syncRequestedTimer, SYNC_AFTER_ENROLL_DELAY))
    {
        startSyncSend(syncRequestedFingerId);

        syncRequestedTimer = 0;
        syncRequestedFingerId = 0;
    }
    
    processSyncSend();
}

void FingerprintModule::setLedDefault()
{
    finger.setLed(KoFIN_LedRingColor.value(Dpt(5, 10)), KoFIN_LedRingControl.value(Dpt(5, 10)), KoFIN_LedRingSpeed.value(Dpt(5, 10)), KoFIN_LedRingCount.value(Dpt(5, 10)));
}

void FingerprintModule::processScanSuccess(uint16_t location, bool external)
{
    KoFIN_ScanSuccess.value(true, DPT_Switch);
    KoFIN_ScanSuccessId.value(location, Dpt(7, 1));

    KoFIN_ScanSuccessData.valueNoSend(location, Dpt(15, 1, 0)); // access identification code
    KoFIN_ScanSuccessData.valueNoSend(false, Dpt(15, 1, 1));    // detection error
    KoFIN_ScanSuccessData.valueNoSend(true, Dpt(15, 1, 2));     // permission accepted
    KoFIN_ScanSuccessData.valueNoSend(false, Dpt(15, 1, 3));    // read direction (not used)
    KoFIN_ScanSuccessData.valueNoSend(false, Dpt(15, 1, 4));    // encryption (not used for now)
    KoFIN_ScanSuccessData.value((uint8_t)0, Dpt(15, 1, 5));     // index of access identification code (not used)

    bool actionFound = false;
    for (size_t i = 0; i < ParamFINACT_FingerActionCount; i++)
    {
        uint16_t fingerId = knx.paramWord(FINACT_faFingerId + FINACT_ParamBlockOffset + i * FINACT_ParamBlockSize);
        if (fingerId == location)
        {
            uint16_t actionId = knx.paramWord(FINACT_faActionId + FINACT_ParamBlockOffset + i * FINACT_ParamBlockSize) - 1;
            if (actionId < FIN_VisibleActions)
            {
                _channels[actionId]->processScan(location); // #ToDo: when not executed, not green
                actionFound = true;
            }
            else
                logInfoP("Invalid ActionId: %d", actionId);
        }
    }

    if (actionFound)
    {
        if (!external)
            finger.setLed(Fingerprint::ScanMatch);
    }
    else
    {
        if (!external)
            finger.setLed(Fingerprint::ScanMatchNoAction);
        
        KoFIN_TouchedNoAction.value(true, DPT_Switch);
    }
}

bool FingerprintModule::enrollFinger(uint16_t location)
{
    logInfoP("Enroll request:");
    logIndentUp();

    bool success = finger.createTemplate();
    if (success)
    {
        success = finger.storeTemplate(location);
        if (!success)
        {
            logInfoP("Storing template failed.");
        }
    }
    else
    {
        logInfoP("Creating template failed.");
    }

    if (success)
    {
        logInfoP("Enrolled to location %d.", location);
        KoFIN_EnrollSuccess.value(true, DPT_Switch);
        KoFIN_EnrollSuccessId.value(location, Dpt(7, 1));

        KoFIN_EnrollSuccess.valueNoSend(location, Dpt(15, 1, 0)); // access identification code
        KoFIN_EnrollSuccess.valueNoSend(false, Dpt(15, 1, 1));    // detection error
        KoFIN_EnrollSuccess.valueNoSend(true, Dpt(15, 1, 2));     // permission accepted
        KoFIN_EnrollSuccess.valueNoSend(false, Dpt(15, 1, 3));    // read direction (not used)
        KoFIN_EnrollSuccess.valueNoSend(false, Dpt(15, 1, 4));    // encryption (not used for now)
        KoFIN_EnrollSuccess.value((uint8_t)0, Dpt(15, 1, 5));     // index of access identification code (not used)

        finger.setLed(Fingerprint::State::Success);
    }
    else
    {
        logInfoP("Enrolling template failed.");
        KoFIN_EnrollSuccess.value(true, DPT_Switch);
        KoFIN_EnrollFailedId.value(location, Dpt(7, 1));

        KoFIN_EnrollSuccessData.valueNoSend(location, Dpt(15, 1, 0)); // access identification code
        KoFIN_EnrollSuccessData.valueNoSend(true, Dpt(15, 1, 1));     // detection error
        KoFIN_EnrollSuccessData.valueNoSend(false, Dpt(15, 1, 2));    // permission accepted
        KoFIN_EnrollSuccessData.valueNoSend(false, Dpt(15, 1, 3));    // read direction (not used)
        KoFIN_EnrollSuccessData.valueNoSend(false, Dpt(15, 1, 4));    // encryption (not used for now)
        KoFIN_EnrollSuccessData.value((uint8_t)0, Dpt(15, 1, 5));     // index of access identification code (not used)

        finger.setLed(Fingerprint::State::Failed);
    }

    logIndentDown();
    resetLedsTimer = delayTimerInit();

    return success;
}

bool FingerprintModule::deleteFinger(uint16_t location)
{
    logInfoP("Delete request:");
    logIndentUp();

    bool success = finger.deleteTemplate(location);
    if (success)
    {
        logInfoP("Template deleted from location %d.", location);
        KoFIN_DeleteSuccess.value(true, DPT_Switch);
        KoFIN_DeleteSuccessId.value(location, Dpt(7, 1));

        KoFIN_DeleteSuccess.valueNoSend(location, Dpt(15, 1, 0)); // access identification code
        KoFIN_DeleteSuccess.valueNoSend(false, Dpt(15, 1, 1));    // detection error
        KoFIN_DeleteSuccess.valueNoSend(true, Dpt(15, 1, 2));     // permission accepted
        KoFIN_DeleteSuccess.valueNoSend(false, Dpt(15, 1, 3));    // read direction (not used)
        KoFIN_DeleteSuccess.valueNoSend(false, Dpt(15, 1, 4));    // encryption (not used for now)
        KoFIN_DeleteSuccess.value((uint8_t)0, Dpt(15, 1, 5));     // index of access identification code (not used)
    }
    else
    {
        logInfoP("Deleting template failed.");
        KoFIN_DeleteSuccess.value(true, DPT_Switch);
        KoFIN_DeleteFailedId.value(location, Dpt(7, 1));

        KoFIN_DeleteSuccessData.valueNoSend(location, Dpt(15, 1, 0)); // access identification code
        KoFIN_DeleteSuccessData.valueNoSend(true, Dpt(15, 1, 1));     // detection error
        KoFIN_DeleteSuccessData.valueNoSend(false, Dpt(15, 1, 2));    // permission accepted
        KoFIN_DeleteSuccessData.valueNoSend(false, Dpt(15, 1, 3));    // read direction (not used)
        KoFIN_DeleteSuccessData.valueNoSend(false, Dpt(15, 1, 4));    // encryption (not used for now)
        KoFIN_DeleteSuccessData.value((uint8_t)0, Dpt(15, 1, 5));     // index of access identification code (not used)
    }

    logIndentDown();
    resetLedsTimer = delayTimerInit();

    return success;
}

void FingerprintModule::processInputKo(GroupObject& iKo)
{
    uint16_t location;

    uint16_t lAsap = iKo.asap();
    switch (lAsap)
    {
        case FIN_KoLock:
            KoFIN_LockStatus.value(KoFIN_Lock.value(DPT_Switch), DPT_Switch);
            logInfoP("Locked: %d", KoFIN_Lock.value(DPT_Switch));
            break;
        case FIN_KoLedRingColor:
        case FIN_KoLedRingControl:
        case FIN_KoLedRingSpeed:
        case FIN_KoLedRingCount:
            setLedDefault();
            logInfoP("LED ring: color=%u, control=%u, speed=%u, count=%u", (uint8_t)KoFIN_LedRingColor.value(Dpt(5, 10)), (uint8_t)KoFIN_LedRingControl.value(Dpt(5, 10)), (uint8_t)KoFIN_LedRingSpeed.value(Dpt(5, 10)), (uint8_t)KoFIN_LedRingCount.value(Dpt(5, 10)));
            break;
        case FIN_KoEnrollNext:
        case FIN_KoEnrollId:
        case FIN_KoEnrollData:
            if (lAsap == FIN_KoEnrollNext)
            {
                location = finger.getNextFreeLocation();
                logInfoP("Next availabe location: %d", location);
            }
            else if (lAsap == FIN_KoEnrollId)
            {
                location = iKo.value(Dpt(7, 1));
                logInfoP("Location provided: %d", location);
            }
            else
            {
                location = iKo.value(Dpt(15, 1, 0));
                logInfoP("Location provided: %d", location);
            }

            enrollRequestedTimer = delayTimerInit();
            enrollRequestedLocation = location;
            break;
        case FIN_KoDeleteId:
        case FIN_KoDeleteData:
            if (lAsap == FIN_KoDeleteId)
            {
                location = iKo.value(Dpt(7, 1));
                logInfoP("Location provided: %d", location);
            }
            else
            {
                location = iKo.value(Dpt(15, 1, 0));
                logInfoP("Location provided: %d", location);
            }

            deleteFinger(location);
            break;
        case FIN_KoExternFingerId:
            location = iKo.value(Dpt(7, 1));
            logInfoP("FingerID received: %d", location);

            processScanSuccess(location, true);
            break;
        case FIN_KoTouchPcbLedRed:
            if (iKo.value(DPT_Switch))
                digitalWrite(LED_RED_PIN, HIGH);
            else
                digitalWrite(LED_RED_PIN, LOW);
            
            break;
        case FIN_KoTouchPcbLedGreen:
            if (iKo.value(DPT_Switch))
                digitalWrite(LED_GREEN_PIN, HIGH);
            else
                digitalWrite(LED_GREEN_PIN, LOW);
            
            break;
        case FIN_KoSync:
            processSyncReceive(iKo.valueRef());
            break;
        default:
        {
            for (uint16_t i = 0; i < ParamFIN_VisibleActions; i++)
                _channels[i]->processInputKo(iKo);
        }
    }
}

void FingerprintModule::startSyncSend(uint16_t fingerId, bool loadModel)
{
    if (!ParamFIN_EnableSync ||
        syncReceiving)
        return;

    logInfoP("Sync-Send: started: fingerId=%u, loadModel=%u, syncDelay=%u", fingerId, loadModel, ParamFIN_SyncDelay);

    finger.setLed(Fingerprint::State::Busy);

    bool success;
    if (loadModel)
    {
        success = finger.loadTemplate(fingerId);
        if (!success)
        {
            logErrorP("Sync-Send: loading template failed");
            return;
        }
    }

    uint8_t syncSendBufferTemp[SYNC_BUFFER_SIZE];
    success = finger.retrieveTemplate(syncSendBufferTemp);
    if (!success)
    {
        logErrorP("Sync-Send: retrieving template failed");
        return;
    }

    setLedDefault();

    uint32_t storageOffset = FIN_CaclStorageOffset(fingerId);
    uint8_t personData[29] = {};
    _fingerprintStorage.read(storageOffset, personData, 29);
    memcpy(syncSendBuffer + TEMPLATE_SIZE, personData, 29);

    const int maxDstSize = LZ4_compressBound(SYNC_BUFFER_SIZE);
    const int compressedDataSize = LZ4_compress_default((char*)syncSendBufferTemp, (char*)syncSendBuffer, SYNC_BUFFER_SIZE, maxDstSize);

    syncSendBufferLength = compressedDataSize;
    syncSendPacketCount = ceil(syncSendBufferLength / (float)SYNC_SEND_PACKET_DATA_LENGTH) + 1; // currently separated control packet
    uint16_t checksum = crc16(syncSendBuffer, syncSendBufferLength);

    logDebugP("Sync-Send (1/%u): control packet: bufferLength=%u, lengthPerPacket=%u, checksum=%u, fingerId=%u%", syncSendPacketCount, syncSendBufferLength, SYNC_SEND_PACKET_DATA_LENGTH, checksum, fingerId);

    /*
    Sync Control Packet Layout:
    -    0: 1 byte : sequence number (0: control packet)
    -    1: 1 byte : sync type (0: new template sync)
    -    2: 1 byte : sync data format version (currently always 0)
    -  3-4: 2 bytes: total data content size
    -    5: 1 byte : max. payload data length per data packet
    -    6: 1 byte : number of data packets
    -  7-8: 2 bytes: checksum
    - 9-10: 2 bytes: finger ID
    */

    uint8_t *data = KoFIN_Sync.valueRef();
    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
    data[3] = syncSendBufferLength >> 8;
    data[4] = syncSendBufferLength;
    data[5] = SYNC_SEND_PACKET_DATA_LENGTH;
    data[6] = syncSendPacketCount;
    data[7] = checksum >> 8;
    data[8] = checksum;
    data[9] = fingerId >> 8;
    data[10] = fingerId;
    KoFIN_Sync.objectWritten();

    syncSendTimer = delayTimerInit();
    syncSendPacketSentCount = 1;
    syncSending = true;
}

void FingerprintModule::processSyncSend()
{
    if (!syncSending ||
        !delayCheck(syncSendTimer, ParamFIN_SyncDelay))
        return;

    syncSendTimer = delayTimerInit();

    uint8_t *data = KoFIN_Sync.valueRef();
    data[0] = syncSendPacketSentCount;
    uint8_t dataPacketNo = syncSendPacketSentCount - 1; // = sequence number - 1
    uint16_t dataOffset = dataPacketNo * SYNC_SEND_PACKET_DATA_LENGTH;
    uint8_t dataLength = dataOffset + SYNC_SEND_PACKET_DATA_LENGTH < syncSendBufferLength ? SYNC_SEND_PACKET_DATA_LENGTH : syncSendBufferLength - dataOffset;
    memcpy(data + 1, syncSendBuffer + dataOffset, dataLength);
    KoFIN_Sync.objectWritten();

    syncSendPacketSentCount++;
    logDebugP("Sync-Send (%u/%u): data packet: dataPacketNo=%u, dataOffset=%u, dataLength=%u", syncSendPacketSentCount, syncSendPacketCount, dataPacketNo, dataOffset, dataLength);

    if (syncSendPacketSentCount == syncSendPacketCount)
    {
        logDebugP("Sync-Send: finished");

        syncSending = false;
    }
}

void FingerprintModule::processSyncReceive(uint8_t* data)
{
    if (syncSending)
        return;
    
    if (data[0] == 0) // sequence number
    {
        switch (data[1]) // sync type
        {
            case 0: // template sync
                if (data[2] != 0)
                {
                    logInfoP("Sync-Receive: Unsupported sync version: %u", data[2]);
                    return;
                }

                syncReceiveBufferLength = (data[3] << 8) | data[4];
                syncReceiveLengthPerPacket = data[5];
                syncReceivePacketCount = data[6];
                syncReceiveBufferChecksum = (data[7] << 8) | data[8];
                syncReceiveFingerId = (data[9] << 8) | data[10];

                logDebugP("Sync-Receive (1/%u): control packet: bufferLength=%u, lengthPerPacket=%u, checksum=%u, fingerId=%u%", syncReceivePacketCount, syncReceiveBufferLength, syncReceiveLengthPerPacket, syncReceiveBufferChecksum, syncReceiveFingerId);

                memset(syncReceivePacketReceived, 0, sizeof(syncReceivePacketReceived));
                syncReceivePacketReceived[0] = true;
                syncReceivePacketReceivedCount = 1;
                syncReceiving = true;

                return;
            default:
                logInfoP("Sync-Receive: Unsupported sync type: %u", data[1]);
                syncReceiving = false;
                return;
        }
    }

    if (!syncReceiving)
    {
        logInfoP("Sync-Receive: data packet without control packet");
        return;
    }

    uint8_t sequenceNo = data[0];
    if (syncReceivePacketReceived[sequenceNo])
    {
        logInfoP("Sync-Receive: same packet already received");
        return;
    }

    syncReceivePacketReceived[sequenceNo] = true;
    uint8_t dataPacketNo = sequenceNo - 1;
    uint16_t dataOffset = dataPacketNo * syncReceiveLengthPerPacket;
    uint8_t dataLength = dataOffset + syncReceiveLengthPerPacket < syncReceiveBufferLength ? syncReceiveLengthPerPacket : syncReceiveBufferLength - dataOffset;
    memcpy(syncReceiveBuffer + dataOffset, data + 1, dataLength);

    syncReceivePacketReceivedCount++;
    logDebugP("Sync-Receive (%u/%u): data packet: dataPacketNo=%u, dataOffset=%u, dataLength=%u", syncReceivePacketReceivedCount, syncReceivePacketCount, dataPacketNo, dataOffset, dataLength);

    if (syncReceivePacketReceivedCount == syncReceivePacketCount)
    {
        finger.setLed(Fingerprint::State::Busy);

        uint16_t checksum = crc16(syncReceiveBuffer, syncReceiveBufferLength);
        if (syncReceiveBufferChecksum == checksum)
            logDebugP("Sync-Receive: finished (checksum=%u)", syncReceiveBufferChecksum);
        else
        {
            logErrorP("Sync-Receive: finished failed (checksum expected=%u, calculated=%u)", syncReceiveBufferChecksum, checksum);
            finger.setLed(Fingerprint::State::Failed);
            resetLedsTimer = delayTimerInit();
            return;
        }

        uint8_t syncSendBufferTemp[SYNC_BUFFER_SIZE];
        const int decompressedSize = LZ4_decompress_safe((char*)syncReceiveBuffer, (char*)syncSendBufferTemp, syncReceiveBufferLength, SYNC_BUFFER_SIZE);
        if (decompressedSize != SYNC_BUFFER_SIZE)
        {
            logErrorP("Sync-Receive: decompression failed (size expected=%u, received=%u)", SYNC_BUFFER_SIZE, decompressedSize);
            finger.setLed(Fingerprint::State::Failed);
            resetLedsTimer = delayTimerInit();
            return;
        }

        bool success;
        success = finger.sendTemplate(syncSendBufferTemp);
        if (!success)
        {
            logErrorP("Sync-Receive: sending template failed");
            finger.setLed(Fingerprint::State::Failed);
            resetLedsTimer = delayTimerInit();
            return;
        }

        success = finger.storeTemplate(syncReceiveFingerId);
        if (!success)
        {
            logErrorP("Sync-Receive: storing template failed");
            finger.setLed(Fingerprint::State::Failed);
            resetLedsTimer = delayTimerInit();
            return;
        }

        uint32_t storageOffset = FIN_CaclStorageOffset(syncReceiveFingerId);
        _fingerprintStorage.write(storageOffset, syncReceiveBuffer + TEMPLATE_SIZE, 29);
        _fingerprintStorage.commit();

        finger.setLed(Fingerprint::State::Success);
        resetLedsTimer = delayTimerInit();
        syncReceiving = false;
    }
}

bool FingerprintModule::processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
{
    if (!knx.configured() || objectIndex != 160 || propertyId != 3)
        return false;

    switch(data[0])
    {
        case 1:
            handleFunctionPropertyEnrollFinger(data, resultData, resultLength);
            return true;
        case 2:
            handleFunctionPropertySyncFinger(data, resultData, resultLength);
            return true;
        case 3:
            handleFunctionPropertyDeleteFinger(data, resultData, resultLength);
            return true;
        case 4:
            handleFunctionPropertyChangeFinger(data, resultData, resultLength);
            return true;
        case 6:
            handleFunctionPropertyResetScanner(data, resultData, resultLength);
            return true;
        case 11:
            handleFunctionPropertySearchPersonByFingerId(data, resultData, resultLength);
            return true;
        case 12:
            handleFunctionPropertySearchFingerIdByPerson(data, resultData, resultLength);
            return true;
        case 21:
            handleFunctionPropertySetPassword(data, resultData, resultLength);
            return true;
    }

    return false;
}

void FingerprintModule::handleFunctionPropertyEnrollFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
{
    logInfoP("Function property: Enroll request");
    logIndentUp();

    uint16_t fingerId = (data[1] << 8) | data[2];
    logDebugP("fingerId: %d", fingerId);

    uint8_t personFinger = data[3];
    logDebugP("personFinger: %d", personFinger);

    uint8_t personName[28] = {};
    for (uint8_t i = 0; i < 28; i++)
    {
        memcpy(personName + i, data + 4 + i, 1);
        if (personName[i] == 0) // null termination
            break;
    }
    logDebugP("personName: %s", personName);

    uint32_t storageOffset = FIN_CaclStorageOffset(fingerId);
    logDebugP("storageOffset: %d", storageOffset);
    _fingerprintStorage.writeByte(storageOffset, personFinger); // only 4 bits used
    _fingerprintStorage.write(storageOffset + 1, personName, 28);
    _fingerprintStorage.commit();

    enrollRequestedTimer = delayTimerInit();
    enrollRequestedLocation = fingerId;
    
    resultData[0] = 0;
    resultLength = 1;
    logIndentDown();
}

void FingerprintModule::handleFunctionPropertySyncFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
{
    logInfoP("Function property: Sync request");
    logIndentUp();

    uint16_t fingerId = (data[1] << 8) | data[2];
    logDebugP("fingerId: %d", fingerId);

    if (finger.hasLocation(fingerId))
    {
        syncRequestedFingerId = fingerId;
        syncRequestedTimer = delayTimerInit();

        resultData[0] = 0;
    }
    else
        resultData[0] = 1;

    resultLength = 1;
    logIndentDown();
}

void FingerprintModule::handleFunctionPropertyDeleteFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
{
    logInfoP("Function property: Delete request");
    logIndentUp();

    uint16_t fingerId = (data[1] << 8) | data[2];
    logDebugP("fingerId: %d", fingerId);

    char personName[28] = {}; // empty

    uint32_t storageOffset = FIN_CaclStorageOffset(fingerId);
    _fingerprintStorage.writeByte(storageOffset, 0); // "0" for not set
    _fingerprintStorage.write(storageOffset + 1, *personName, 28);
    _fingerprintStorage.commit();

    bool success = deleteFinger(fingerId);
    
    resultData[0] = success ? 0 : 1;
    resultLength = 1;
    logIndentDown();
}

void FingerprintModule::handleFunctionPropertyChangeFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
{
    logInfoP("Function property: Change request");
    logIndentUp();

    uint16_t fingerId = (data[1] << 8) | data[2];
    logDebugP("fingerId: %d", fingerId);

    if (finger.hasLocation(fingerId))
    {
        uint8_t personFinger = data[3];
        logDebugP("personFinger: %d", personFinger);

        uint8_t personName[28] = {};
        for (uint8_t i = 0; i < 28; i++)
        {
            memcpy(personName + i, data + 4 + i, 1);
            if (personName[i] == 0) // null termination
                break;
        }
        logDebugP("personName: %s", personName);

        uint32_t storageOffset = FIN_CaclStorageOffset(fingerId);
        logDebugP("storageOffset: %d", storageOffset);
        _fingerprintStorage.writeByte(storageOffset, personFinger); // only 4 bits used
        _fingerprintStorage.write(storageOffset + 1, personName, 28);
        _fingerprintStorage.commit();

        syncRequestedFingerId = fingerId;
        syncRequestedTimer = delayTimerInit();

        resultData[0] = 0;
    }
    else
    {
        logInfoP("Finger not found");
        resultData[0] = 1;
    }

    resultLength = 1;
    logIndentDown();
}

void FingerprintModule::handleFunctionPropertyResetScanner(uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
{
    logInfoP("Function property: Reset scanner");
    logIndentUp();

    char personData[29] = {}; // empty
    for (uint16_t i = 0; i < MAX_FINGERS; i++)
    {
        uint32_t storageOffset = FIN_CaclStorageOffset(i);
        _fingerprintStorage.write(storageOffset, *personData, 29);
    }
    _fingerprintStorage.commit();

    bool success = finger.emptyDatabase();
    resetLedsTimer = delayTimerInit();

    resultData[0] = success ? 0 : 1;
    resultLength = 1;
    logIndentDown();
}

void FingerprintModule::handleFunctionPropertySetPassword(uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
{
    logInfoP("Function property: Set password");
    logIndentUp();

    uint8_t passwordOption = data[1];
    logDebugP("passwordOption: %d", passwordOption);

    uint8_t dataOffset = 1;

    char newPassword[16] = {};
    for (size_t i = 0; i < 16; i++)
    {
        dataOffset++;
        memcpy(newPassword + i, data + dataOffset, 1);

        if (newPassword[i] == 0) // null termination
            break;
    }

    uint32_t newPasswordCrc = 0;
    if (newPassword[0] != 48 || // = "0": if user inputs only "0", we just use it as is without CRC
        newPassword[1] != 0)    // null termination
        newPasswordCrc = CRC32::calculate(newPassword, 16);
    logDebugP("newPassword: %s (crc: %u)", newPassword, newPasswordCrc);

    // change password
    uint32_t oldPasswordCrc = 0;
    if (passwordOption == 2)
    {
        char oldPassword[16] = {};
        for (uint8_t i = 0; i < 16; i++)
        {
            dataOffset++;
            memcpy(oldPassword + i, data + dataOffset, 1);

            if (oldPassword[i] == 0) // null termination
                break;
        }

        if (oldPassword[0] != 48 || // = "0": if user inputs only "0", we just use it as is without CRC
            oldPassword[1] != 0)    // null termination
            oldPasswordCrc = CRC32::calculate(oldPassword, 16);
        logDebugP("oldPassword: %s (crc: %u)", oldPassword, oldPasswordCrc);
    }

    uint32_t currentCrc = _fingerprintStorage.readInt(FLASH_SCANNER_PASSWORD_OFFSET);
    logDebugP("currentCrc: %u", currentCrc);

    bool success = false;
    if (currentCrc == oldPasswordCrc)
    {
        logDebugP("Current matches old CRC.");
        logIndentUp();

        logInfoP("Setting new fingerprint scanner password.");
        logIndentUp();
        success = finger.setPassword(newPasswordCrc);
        resetLedsTimer = delayTimerInit();
        logInfoP(success ? "Success." : "Failed.");
        logIndentDown();
        
        if (success)
        {
            logDebugP("Saving new password in flash.");
            _fingerprintStorage.writeInt(FLASH_SCANNER_PASSWORD_OFFSET, newPasswordCrc);
            _fingerprintStorage.commit();

            finger.close();
            initFingerprintScanner();
            finger.start();
        }

        resetLedsTimer = delayTimerInit();
        logIndentDown();
    }
    else
        logDebugP("Invalid old password provided.");
    
    resultData[0] = success ? 0 : 1;
    resultLength = 1;
    logIndentDown();
}

void FingerprintModule::handleFunctionPropertySearchPersonByFingerId(uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
{
    logInfoP("Function property: Search person by FingerId");
    logIndentUp();

    uint16_t fingerId = (data[1] << 8) | data[2];
    logDebugP("fingerId: %d", fingerId);

    if (!finger.hasLocation(fingerId))
    {
        logDebugP("Unrecognized by scanner!");
        resultData[0] = 1;
        resultLength = 1;

        logIndentDown();
        return;
    }

    uint8_t personName[28] = {};

    uint32_t storageOffset = FIN_CaclStorageOffset(fingerId);
    logDebugP("storageOffset: %d", storageOffset);
    uint8_t personFinger = _fingerprintStorage.readByte(storageOffset);
    if (personFinger > 0)
    {
        _fingerprintStorage.read(storageOffset + 1, personName, 28);

        logDebugP("Found:");
        logIndentUp();
        logDebugP("personFinger: %d", personFinger);
        logDebugP("personName: %s", personName);
        logIndentDown();

        resultData[0] = 0;
        resultData[1] = personFinger;
        resultLength = 2;
        for (uint8_t i = 0; i < 28; i++)
        {
            memcpy(resultData + 2 + i, personName + i, 1);
            resultLength++;

            if (personName[i] == 0) // null termination
                break;
        }
    }
    else
    {
        logDebugP("Not found.");

        resultData[0] = 1;
        resultLength = 1;
    }

    logIndentDown();
}

void FingerprintModule::handleFunctionPropertySearchFingerIdByPerson(uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
{
    logInfoP("Function property: Search FingerId(s) by person");
    logIndentUp();

    uint8_t searchPersonFinger = data[1];
    logDebugP("searchPersonFinger: %d", searchPersonFinger); // can be "0" if only by name should be searched

    char searchPersonName[28] = {};
    uint8_t searchPersonNameLength = 28;
    for (size_t i = 0; i < 28; i++)
    {
        memcpy(searchPersonName + i, data + 2 + i, 1);
        if (searchPersonName[i] == 0) // null termination
        {
            searchPersonNameLength = i;
            break;
        }
    }
    logDebugP("searchPersonName: %s (length: %u)", searchPersonName, searchPersonNameLength);

    uint16_t* fingerIds = finger.getLocations();
    uint16_t templateCount = finger.getTemplateCount();

    uint32_t storageOffset = 0;
    uint8_t personFinger = 0;
    uint8_t personName[29] = {};
    uint8_t foundCount = 0;
    uint16_t foundTotalCount = 0;
    for (uint16_t i = 0; i < templateCount; i++)
    {
        uint16_t fingerId = fingerIds[i];
        storageOffset = FIN_CaclStorageOffset(fingerId);
        personFinger = _fingerprintStorage.readByte(storageOffset);
        if (searchPersonFinger > 0)
            if (searchPersonFinger != personFinger)
                continue;

        _fingerprintStorage.read(storageOffset + 1, personName, 28);
        if (strcasestr((char *)personName, searchPersonName) != nullptr)
        {
            logDebugP("Found:");
            logIndentUp();
            logDebugP("fingerId: %d", fingerId);
            logDebugP("personFinger: %d", personFinger);
            logDebugP("personName: %s", personName);
            logIndentDown();

            // we return max. 10 results
            if (foundCount <= 10)
            {
                resultData[3 + foundCount * 31] = fingerId >> 8;
                resultData[3 + foundCount * 31 + 1] = fingerId;
                resultData[3 + foundCount * 31 + 2] = personFinger;
                memcpy(resultData + 3 + foundCount * 31 + 3, personName, 28);

                foundCount++;
            }

            foundTotalCount++;
        }
    }
    
    resultData[0] = foundCount > 0 ? 0 : 1;
    resultData[1] = foundTotalCount >> 8;
    resultData[2] = foundTotalCount;
    resultLength = 3 + foundCount * 31;

    logDebugP("foundTotalCount: %u", foundTotalCount);
    logIndentDown();
}

void FingerprintModule::processAfterStartupDelay()
{
}

void FingerprintModule::delayCallback(uint32_t period)
{
    uint32_t start = delayTimerInit();
    delayCallbackActive = true;

    while (!delayCheck(start, period))
        openknx.loop();

    openknx.common.skipLooptimeWarning();
    delayCallbackActive = false;
}

FingerprintModule openknxFingerprintModule;

// void FingerprintModule::writeFlash()
// {
//     for (size_t i = 0; i < flashSize(); i++)
//     {
//         //openknx.flash.writeByte(0xd0 + i);
//     }
// }

// void FingerprintModule::readFlash(const uint8_t* data, const uint16_t size)
// {
//     // printHEX("RESTORE:", data,  len);
// }

// uint16_t FingerprintModule::flashSize()
// {
//     return 10;
// }