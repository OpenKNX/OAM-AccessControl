#include "AccessControl.h"
#include "SwitchActuatorModule.h"
#include "Logic.h"
#include "GpioBinaryInputModule.h"
#include "VirtualButtonModule.h"
#include "FileTransferModule.h"
#include "Feedback.h"
#if defined(DEVICE_DEV_REG1_LAN_TP_Base_V00_11) && defined(KNX_IP_WIFI)
#include <WiFi.h>
#endif
#if defined(OPENKNX_HOMEKEY_ENABLED) && defined(ARDUINO_ARCH_ESP32)
#include "HomeKeyModule.h"
#ifdef OPENKNX_HOMEKEY_WEB
#include "NetworkModule.h"
#include "HomeKeyWifi.h"
#endif
#endif

void setup()
{
#ifdef DEVICE_DEV_REG1_LAN_TP_Base_V00_11
    // Keep the PN7161 in normal NCI mode before reader initialization.
    digitalWrite(NFC_DWL_REQ_PIN, LOW);
    pinMode(NFC_DWL_REQ_PIN, OUTPUT);
    #if defined(KNX_IP_WIFI) && !defined(OPENKNX_HOMEKEY_WEB)
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    #ifdef OPENKNX_WIFI_SSID
        #ifndef OPENKNX_WIFI_PASSWORD
            #define OPENKNX_WIFI_PASSWORD ""
        #endif
    WiFi.begin(OPENKNX_WIFI_SSID, OPENKNX_WIFI_PASSWORD);
    #else
    // Reuse Wi-Fi credentials already stored in NVS.
    WiFi.begin();
    #endif
    #endif
#endif
    openknx.init();
#if defined(OPENKNX_HOMEKEY_WEB) && defined(OPENKNX_WIFI_SSID)
    #ifndef OPENKNX_WIFI_PASSWORD
        #define OPENKNX_WIFI_PASSWORD ""
    #endif
    if (!HomeKeyWifi::configure(OPENKNX_WIFI_SSID, OPENKNX_WIFI_PASSWORD))
        logError("HomeKey", "Build-time WiFi configuration could not be saved or started");
#endif
    openknx.addModule(1, openknxLogic);
    openknx.addModule(2, openknxAccessControl);
    openknx.addModule(3, openknxVirtualButtonModule);
    openknx.addModule(4, openknxSwitchActuatorModule);
#if defined(OPENKNX_BI_GPIO_PINS) && OPENKNX_BI_GPIO_COUNT > 0 && BI_ChannelCount > 0
    openknx.addModule(5, openknxGpioBinaryInputModule);
#endif
    openknx.addModule(6, openknxFeedback);
#if defined(OPENKNX_HOMEKEY_ENABLED) && defined(ARDUINO_ARCH_ESP32)
    openknx.addModule(7, openknxHomeKey);
#ifdef OPENKNX_HOMEKEY_WEB
    openknx.addModule(8, openknxNetwork);
#endif
#endif
    openknx.addModule(9, openknxFileTransferModule);
    openknx.setup();
}

void loop()
{
#if defined(DEVICE_DEV_REG1_LAN_TP_Base_V00_11) && defined(KNX_IP_WIFI) && !defined(OPENKNX_HOMEKEY_WEB)
    // Wi-Fi can acquire/change its address after knx.start(). Reopen the IP
    // endpoint on that transition, and retry a failed multicast join.
    static uint32_t previousIp = 0;
    static uint32_t lastJoinAttempt = 0;
    const uint32_t currentIp = WiFi.status() == WL_CONNECTED ? uint32_t(WiFi.localIP()) : 0;
    if (currentIp != 0 && currentIp != previousIp)
    {
        knx.enabled(false);
        knx.enabled(true);
        lastJoinAttempt = millis();
    }
    else if (currentIp != 0 && !knx.enabled() && millis() - lastJoinAttempt >= 1000)
    {
        knx.enabled(true);
        lastJoinAttempt = millis();
    }
    previousIp = currentIp;
#endif
    openknx.loop();
}
