#define _GNU_SOURCE

#include "OpenKNX.h"
#include "hardware.h"
#include "Fingerprint.h"
#include "ActionChannel.h"
#include "CRC32.h"
#include "CRC16.h"
#include "lz4.h"


#define NFC_WIRE Wire
#define NFC_ADDR 0x28
#define NFC_SDA 20
#define NFC_SCL 21
#define NFC_IRQ 16
#define NFC_VEN 17
#define NFC_DWL_REQ 23


#define INIT_RESET_TIMEOUT 1000
#define LED_RESET_TIMEOUT 1000
#define ENROLL_REQUEST_DELAY 100
#define CAPTURE_RETRIES_TOUCH_TIMEOUT 500
#define CAPTURE_RETRIES_LOCK_TIMEOUT 3000
#define CHECK_SENSOR_DELAY 1000
#define SHUTDOWN_SENSOR_DELAY 3000

#define MAX_FINGERS 1500

#define FLASH_MAGIC_WORD 2912744758
#define FINGER_DATA_SIZE 29
#define FIN_CaclStorageOffset(fingerId) fingerId * FINGER_DATA_SIZE + 4096 + 1 // first byte free for finger info storage format version

#define FLASH_SCANNER_PASSWORD_OFFSET 5

#define SYNC_BUFFER_SIZE TEMPLATE_SIZE + FINGER_DATA_SIZE
#define SYNC_SEND_PACKET_DATA_LENGTH 13
#define SYNC_AFTER_ENROLL_DELAY 500
#define SYNC_IGNORE_DELAY 500

#ifdef SCANNER_PWR_PIN
  #define FINGER_PWR_ON    SCANNER_PWR_PIN_ACTIVE_ON == HIGH ? HIGH : LOW
  #define FINGER_PWR_OFF   SCANNER_PWR_PIN_ACTIVE_ON == HIGH ? LOW : HIGH
#endif

/*
Flash Storage Layout:
- 0-3: 4 bytes: int magic word
-   4: 1 byte main storage version format (currently 0)
- 5-8: 4 bytes: int fingerprint scanner password
*/

class FingerprintModule : public OpenKNX::Module
{
  public:
    void loop() override;
    void setup() override;
    void processAfterStartupDelay() override;
    void processInputKo(GroupObject &ko) override;
		bool processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength) override;
		// bool processFunctionPropertyState(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength) override;
    bool sendReadRequest(GroupObject &ko);

    const std::string name() override;
    const std::string version() override;
    void savePower() override;
    bool restorePower() override;
    bool processCommand(const std::string cmd, bool diagnoseKo);
    // void writeFlash() override;
    // void readFlash(const uint8_t* data, const uint16_t size) override;
    // uint16_t flashSize() override;

  private:
    static void interruptDisplayTouched();
    static void interruptTouchLeft();
    static void interruptTouchRight();
    bool switchFingerprintPower(bool on, bool testMode = false);
    void initFingerprintScanner(bool testMode = false);
    void initFlash();
    void processScanSuccess(uint16_t location, bool external = false);
    bool enrollFinger(uint16_t location);
    bool deleteFinger(uint16_t location, bool sync = true);
    bool searchForFinger();
    void resetRingLed();
    void startSyncDelete(uint16_t fingerId);
    void startSyncSend(uint16_t fingerId, bool loadModel = true);
    void processSyncSend();
    void processSyncReceive(uint8_t* data);
    void processInputKoLock(GroupObject &ko);
    void processInputKoTouchPcbLed(GroupObject &ko);
    void processInputKoEnroll(GroupObject &ko);
    void handleFunctionPropertyEnrollFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySyncFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertyDeleteFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertyChangeFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertyResetScanner(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySetPassword(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySearchPersonByFingerId(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySearchFingerIdByPerson(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    static void delayCallback(uint32_t period);
    void runTestMode();

    OpenKNX::Flash::Driver _fingerprintStorage;
    ActionChannel *_channels[FIN_ChannelCount];

    Fingerprint *finger = nullptr;
    bool hasLastFoundLocation = false;
    uint16_t lastFoundLocation = 0;
    uint32_t initResetTimer = 0;
    uint32_t resetLedsTimer = 0;
    uint32_t enrollRequestedTimer = 0;
    uint16_t enrollRequestedLocation = 0;
    uint32_t checkSensorTimer = 0;
    uint32_t searchForFingerDelayTimer = 0;
    uint32_t shutdownSensorTimer = 0;
    inline static bool delayCallbackActive = false;

    inline volatile static bool touched = false;
    bool isLocked = false;

    uint32_t syncIgnoreTimer = 0;

    bool syncSending = false;
    uint32_t syncSendTimer = 0;
    uint8_t syncSendBuffer[SYNC_BUFFER_SIZE];
    uint16_t syncSendBufferLength = 0;
    uint8_t syncSendPacketCount = 0;
    uint8_t syncSendPacketSentCount = 0;
    uint32_t syncRequestedTimer = 0;
    uint16_t syncRequestedFingerId = 0;

    bool syncReceiving = false;
    uint16_t syncReceiveFingerId = 0;
    uint8_t syncReceiveBuffer[SYNC_BUFFER_SIZE];
    uint16_t syncReceiveBufferLength = 0;
    uint16_t syncReceiveBufferChecksum = 0;
    uint8_t syncReceiveLengthPerPacket = 0;
    uint8_t syncReceivePacketCount = 0;
    uint8_t syncReceivePacketReceivedCount = 0;
    bool syncReceivePacketReceived[SYNC_BUFFER_SIZE] = {false};

    uint32_t readRequestDelay = 0;
};

extern FingerprintModule openknxFingerprintModule;