#include "HomeKeyWifi.h"
#include <HomeSpan.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <cassert>
#include <iostream>

int main()
{
    using HomeKeyWifi::Result;
    unsigned queued = 0;
    std::string queuedSsid, queuedPassword;
    auto queue = [&](const std::string& ssid, const std::string& password)
    { ++queued; queuedSsid = ssid; queuedPassword = password; return true; };
    auto run = [&](const std::string& command, bool remote = false, bool started = false)
    { return HomeKeyWifi::command(command, remote, started, queue); };

    // Fresh IP firmware: no ETS configuration and no HAP worker exist.
    assert(run("wifi first-network secret with spaces") == Result::Connecting);
    assert(queued == 0 && WiFi.attempts == 1 && homeSpan.saves == 1);
    assert(WiFi.ssid == "first-network" && WiFi.password == "secret with spaces");
    assert(homeSpan.ssid == WiFi.ssid && homeSpan.password == WiFi.password);
    assert(WiFi.persistentStorage && WiFi.reconnect && selectedStorage == WIFI_STORAGE_FLASH);
    assert(run("hk wifi replacement updated-secret") == Result::Connecting);
    assert(WiFi.attempts == 2 && homeSpan.ssid == "replacement");

    // Once HAP runs, the console must not modify HomeSpan from another task.
    assert(run("wifi live-network live-secret", false, true) == Result::Queued);
    assert(queued == 1 && queuedSsid == "live-network" && queuedPassword == "live-secret");
    assert(WiFi.attempts == 2 && homeSpan.saves == 2);
    assert(HomeKeyWifi::command("hk wifi network secret", false, true,
        [](const auto&, const auto&) { return false; }) == Result::Busy);

    // Diagnostic KOs cannot change network configuration, before or after HAP.
    for (bool started : {false, true})
        for (const auto* text : {"wifi network secret", "hk wifi network secret"})
            assert(run(text, true, started) == Result::LocalOnly);
    assert(WiFi.attempts == 2 && homeSpan.saves == 2 && queued == 1);

    for (const auto& text : {std::string("wifi"), std::string("hk wifi"),
            "wifi " + std::string(33, 's') + " password",
            "wifi network " + std::string(65, 'p'), std::string("wifi bad\0name password", 22)})
        assert(run(text) == Result::Invalid);
    for (const auto* text : {"hk pair 12345678", "hk reset", "wifi-status", "help"})
        assert(run(text) == Result::NotHandled);
    assert(WiFi.attempts == 2 && queued == 1);

    WiFi.modeOk = false;
    assert(run("wifi unavailable secret") == Result::Failed);
    WiFi.modeOk = true; storageResult = -1;
    assert(run("wifi unavailable secret") == Result::Failed);
    assert(homeSpan.saves == 2 && WiFi.attempts == 2);
    storageResult = ESP_OK;
    assert(run("wifi open-network") == Result::Connecting);
    assert(WiFi.password.empty());
    WiFi.beginResult = WL_CONNECT_FAILED;
    assert(run("wifi unavailable secret") == Result::Failed);
    std::cout << "Home Key WiFi console checks passed\n";
}
