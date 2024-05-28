#define _GNU_SOURCE

#include "OpenKNX.h"
#include "hardware.h"
#include "Fingerprint.h"
#include "ActionChannel.h"
#include "CRC32.h"
#include "CRC16.h"
#include "lz4.h"

#define SCANNER_TOUCH_PIN 2

#define TOUCH_LEFT_PIN 26
#define TOUCH_RIGHT_PIN 27
#define LED_GREEN_PIN 24
#define LED_RED_PIN 25

#define EXT0 14
#define EXT1 15
#define EXT2 28 // ADC
#define EXT3 18
#define EXT4 29 // ADC
#define EXT5 19

#define INIT_RESET_TIMEOUT 1000
#define LED_RESET_TIMEOUT 1000
#define ENROLL_REQUEST_DELAY 100
#define CAPTURE_RETRIES_TOUCH_TIMEOUT 500
#define CAPTURE_RETRIES_LOCK_TIMEOUT 3000

#define MAX_FINGERS 1500

#define FLASH_MAGIC_WORD 2912744758
#define FINGER_DATA_SIZE 29
#define FIN_CaclStorageOffset(fingerId) fingerId * FINGER_DATA_SIZE + 4096 + 1 // first byte free for finger info storage format version

#define FLASH_SCANNER_PASSWORD_OFFSET 5

#define SYNC_BUFFER_SIZE TEMPLATE_SIZE + FINGER_DATA_SIZE
#define SYNC_SEND_PACKET_DATA_LENGTH 13
#define SYNC_AFTER_ENROLL_DELAY 500

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

    const std::string name() override;
    const std::string version() override;
    // void writeFlash() override;
    // void readFlash(const uint8_t* data, const uint16_t size) override;
    // uint16_t flashSize() override;

  private:
    static void interruptDisplayTouched();
    static void interruptTouchLeft();
    static void interruptTouchRight();
    void initFingerprintScanner();
    void initFlash();
    void processScanSuccess(uint16_t location, bool external = false);
    bool enrollFinger(uint16_t location);
    bool deleteFinger(uint16_t location);
    void setLedDefault();
    void startSyncSend(uint16_t fingerId, bool loadModel = true);
    void processSyncSend();
    void processSyncReceive(uint8_t* data);
    void handleFunctionPropertyEnrollFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySyncFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertyDeleteFinger(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertyResetScanner(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySetPassword(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySearchPersonByFingerId(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    void handleFunctionPropertySearchFingerIdByPerson(uint8_t *data, uint8_t *resultData, uint8_t &resultLength);
    static void delayCallback(uint32_t period);

    OpenKNX::Flash::Driver _fingerprintStorage;
    ActionChannel *_channels[FIN_ChannelCount];

    Fingerprint finger;
    uint32_t initResetTimer = 0;
    uint32_t resetLedsTimer = 0;
    uint32_t enrollRequestedTimer = 0;
    uint16_t enrollRequestedLocation = 0;
    inline static bool delayCallbackActive = false;

    inline volatile static bool touched = false;

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
};

extern FingerprintModule openknxFingerprintModule;