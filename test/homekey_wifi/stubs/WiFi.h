#pragma once
#include <string>
inline constexpr int WIFI_STA = 1;
inline constexpr int WL_CONNECT_FAILED = 4;
struct WifiStub
{
    bool modeOk = true, persistentStorage = false, reconnect = false;
    unsigned attempts = 0, modes = 0;
    int beginResult = 0;
    std::string ssid, password;
    void persistent(bool enabled) { persistentStorage = enabled; }
    bool mode(int mode) { ++modes; return modeOk && mode == WIFI_STA; }
    void setAutoReconnect(bool enabled) { reconnect = enabled; }
    int begin(const char* network, const char* secret) { ++attempts; ssid = network; password = secret; return beginResult; }
};
inline WifiStub WiFi;
