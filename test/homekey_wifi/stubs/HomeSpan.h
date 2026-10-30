#pragma once
#include <string>
struct HomeSpanStub
{
    unsigned saves = 0;
    std::string ssid, password;
    void setWifiCredentials(const char* network, const char* secret) { ++saves; ssid = network; password = secret; }
};
inline HomeSpanStub homeSpan;
