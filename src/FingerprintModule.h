#define _GNU_SOURCE

#include "OpenKNX.h"
#include "hardware.h"
#include "Fingerprint.h"
#include "ActionChannel.h"
#include "FastCRC.h"
#include "lz4.h"
#include "pn7160interface/pn7160interface.hpp"
#include "logging/logging.hpp"
#include "nci/nci.hpp"


#define NFC_WIRE Wire
#define NFC_PN7160_ADDR 0x28
#define NFC_SDA_PIN 20
#define NFC_SCL_PIN 21
#define NFC_IRQ_PIN 16
#define NFC_VEN_PIN 17
#define NFC_DWL_REQ_PIN 23


#define INIT_RESET_TIMEOUT 1000
#define LED_RESET_TIMEOUT 1000
#define LED_RESET_FAST_TIMEOUT 250
#define ENROLL_REQUEST_DELAY 100
#define CAPTURE_RETRIES_TOUCH_TIMEOUT 500
#define CAPTURE_RETRIES_LOCK_TIMEOUT 3000
#define CHECK_SENSOR_DELAY 1000
#define SHUTDOWN_SENSOR_DELAY 3000

#define MAX_FINGERS 1500
#define OPENKNX_FIN_FLASH_FINGER_MAGIC_WORD 2912744758
#define OPENKNX_FIN_FLASH_FINGER_DATA_SIZE 29 // 1 byte: which finger, 28 bytes: person name
#define FIN_CalcFingerStorageOffset(fingerId) fingerId * OPENKNX_FIN_FLASH_FINGER_DATA_SIZE + 4096 + 1 // first byte free for finger info storage format version
#define FLASH_FINGER_SCANNER_PASSWORD_OFFSET 5

#define MAX_NFCS 1500
#define OPENKNX_FIN_FLASH_NFC_MAGIC_WORD 1983749238
#define OPENKNX_FIN_FLASH_NFC_DATA_SIZE 38 // 10 byte: NFC tag UID, 28 bytes: person name
#define FIN_CalcNfcStorageOffset(nfcId) nfcId * OPENKNX_FIN_FLASH_NFC_DATA_SIZE + 4096 + 1 // first byte free for NFC info storage format version
#define NFC_ENROLL_TIMEOUT 10000
#define NFC_ENROLL_LED_BLINK_INTERVAL 250

#define SYNC_BUFFER_SIZE TEMPLATE_SIZE + OPENKNX_FIN_FLASH_FINGER_DATA_SIZE
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
    enum SyncType : uint8_t
    {
        FINGER,
        NFC
    };

    static void interruptDisplayTouched();
    static void interruptTouchLeft();
    static void interruptTouchRight();
    bool switchFingerprintPower(bool on, bool testMode = false);
    void initFingerprintScanner(bool testMode = false);
    void initFlashFingerprint();
    void initFlashNfc();
    void initNfc();
    void loopNfc();
    void processFingerScanSuccess(uint16_t location, bool external = false);
    void processNfcScanSuccess(uint16_t nfcId, bool external = false);
    bool enrollFinger(uint16_t location);
    bool deleteFinger(uint16_t location, bool sync = true);
    bool deleteNfc(uint16_t nfcId, bool sync = true);
    bool searchForFinger();
    void resetRingLed();
    void startSyncDelete(SyncType syncType, uint16_t deleteId);
    void startSyncSend(SyncType syncType, uint16_t syncId, bool loadModel = true);
    void processSyncSend();
    void processSyncReceive(uint8_t* data);
    void processInputKoLock(GroupObject &ko);
    void processInputKoTouchPcbLed(GroupObject &ko);
    void processInputKoEnrollFinger(GroupObject &ko);
    void processInputKoEnrollNfc(GroupObject &ko);
    void handleFunctionPropertyEnrollFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertyChangeFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySyncFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertyDeleteFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertyResetFingerScanner(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySearchPersonByFingerId(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySearchFingerIdByPerson(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySetFingerPassword(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertyEnrollNfc(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertyChangeNfc(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySyncNfc(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertyDeleteNfc(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertyResetNfcScanner(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySearchTagByNfcId(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySearchNfcIdByTag(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    static void delayCallback(uint32_t period);
    void runTestMode();

    FastCRC16 crc16;
    FastCRC32 crc32;

    OpenKNX::Flash::Driver _fingerprintStorage;
    OpenKNX::Flash::Driver _nfcStorage;
    ActionChannel *_channels[FIN_ChannelCount];

    Fingerprint *finger = nullptr;
    bool hasLastFoundLocation = false;
    uint16_t lastFoundLocation = 0;
    uint32_t initResetTimer = 0;
    uint32_t resetFingerLedTimer = 0;
    uint32_t resetTouchPcbLedTimer = 0;
    uint32_t resetTouchPcbLedTimerFast = 0;
    uint32_t enrollRequestedFingerTimer = 0;
    uint16_t enrollRequestedFingerLocation = 0;
    uint32_t enrollNfcStarted = 0;
    uint16_t enrollNfcId = 0;
    bool enrollNfcLedOn = false;
    uint32_t enrollNfcLedLastChanged = 0;
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
    uint32_t syncRequestedFingerTimer = 0;
    uint16_t syncRequestedFingerId = 0;

    uint32_t syncRequestedNfcTimer = 0;
    uint16_t syncRequestedNfcId = 0;

    bool syncReceiving = false;
    SyncType syncReceiveType;
    uint16_t syncReceiveSyncId = 0;
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