#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>
#include <esp_err.h>
class TwoWire
{
  public:
    inline static TwoWire* active = nullptr;
    bool busInitialized = false;
    size_t capacity = 258;
    bool beginOk = true;
    int sda = -1, scl = -1, timeout = 0;
    uint32_t clock = 0;
    unsigned transmissions = 0;
    unsigned halWrites = 0, deviceAdds = 0, deviceRemoves = 0, liveDevices = 0;
    unsigned receives = 0;
    std::deque<esp_err_t> readStatuses;
    bool busHandleAvailable = true;
    esp_err_t deviceAddStatus = ESP_OK;
    size_t shortWrite = 0;
    uint8_t address = 0;
    std::deque<esp_err_t> statuses;
    std::deque<std::vector<uint8_t>> incoming;
    std::deque<uint8_t> rx;
    std::vector<std::vector<uint8_t>> sent;
    size_t setBufferSize(size_t) { return capacity; }
    bool begin(int a, int b, uint32_t c) { active = this; sda = a; scl = b; clock = c; busInitialized = beginOk; return beginOk; }
    uint8_t getBusNum() { return 1; }
    uint16_t getTimeOut() { return timeout; }
    void setTimeOut(int t) { timeout = t; }
    size_t requestFrom(uint8_t a, size_t)
    {
        address = a;
        if (incoming.empty()) return 0;
        auto packet = incoming.front(); incoming.pop_front();
        rx.assign(packet.begin(), packet.end());
        return packet.size();
    }
    int read() { if (rx.empty()) return -1; int b = rx.front(); rx.pop_front(); return b; }
    void beginTransmission(uint8_t a) { address = a; ++transmissions; }
    size_t write(const uint8_t* data, size_t length)
    {
        sent.emplace_back(data, data + length);
        return shortWrite ? shortWrite : length;
    }
    uint8_t endTransmission()
    {
        if (statuses.empty()) return 0;
        auto result = statuses.front(); statuses.pop_front(); return result;
    }
};
