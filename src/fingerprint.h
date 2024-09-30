#pragma once

//#define CALC_TEMPLATE_CHECKUM
#define TEMPLATE_SIZE 1536

#include "Adafruit_Fingerprint.h"
#include "OpenKNX.h"
#ifdef CALC_TEMPLATE_CHECKUM
    #include "crc16.h"
#endif
#include <string>
#include <functional>

typedef std::function<void(uint32_t)> DelayCalback;

class Fingerprint
{
  public:
    enum State
    {
        None,
        ScanFinger,
        ScanMatch,
        ScanMatchNoAction,
        ScanNoMatch,
        EnrollCreateModel,
        WaitForFinger,
        RemoveFinger,
        DeleteNotFound,
        Success,
        Failed,
        Locked,
        Busy
    };

    struct FindFingerResult
    {
        uint8_t found;
        uint16_t location;
        uint16_t checksum;
    };

    bool scannerReady;

    Fingerprint(uint32_t overridePassword = 0);
    Fingerprint(DelayCalback delayCallback, uint32_t overridePassword = 0);

    bool start();
    void logSystemParameters();
    void close();
    std::string logPrefix();
    bool setLed(State state);
    bool setLed(uint8_t color, uint8_t control, uint8_t speed, uint8_t count);
    bool hasFinger();

    uint16_t getTemplateCount();
    bool hasLocation(uint16_t location);
    uint16_t* getLocations();
    uint16_t getNextFreeLocation();
    FindFingerResult findFingerprint();
    bool createTemplate();
    bool retrieveTemplate(uint8_t templateData[]);
    bool sendTemplate(uint8_t templateData[]);
    bool writeCrc(uint16_t location, uint8_t *templateData, uint32_t secret);
    bool loadTemplate(uint16_t location);
    bool storeTemplate(uint16_t location);
    bool deleteTemplate(uint16_t location);
    bool setPassword(uint32_t newPasswort);
    bool emptyDatabase();
    bool checkSensor();

  private:
    struct GetNotepadPageIndexResult
    {
        uint8_t page;
        uint8_t index;
    };

    Adafruit_Fingerprint _finger;
    DelayCalback _delayMs;

    bool _listTemplates();
    GetNotepadPageIndexResult _getNotepadPageIndex(u_int16_t templateLocation);
    static void _delayCallbackDefault(uint32_t period);
};