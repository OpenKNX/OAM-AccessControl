#pragma once
#include <Wire.h>
#include <cassert>
#include <functional>
inline std::function<void()> afterWrite;
inline bool i2cIsInit(uint8_t bus)
{
    assert(bus == 1);
    return TwoWire::active && TwoWire::active->busInitialized;
}
inline esp_err_t i2cWrite(uint8_t bus, uint16_t address, const uint8_t* data, size_t length, uint32_t timeout)
{
    assert(bus == 1 && timeout == 20);
    auto& wire = *TwoWire::active;
    ++wire.transmissions;
    wire.address = address;
    wire.sent.emplace_back(data, data + length);
    if (afterWrite) afterWrite();
    if (wire.statuses.empty()) return ESP_OK;
    const auto result = wire.statuses.front();
    wire.statuses.pop_front();
    return result;
}
