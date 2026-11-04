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
inline void* i2cBusHandle(uint8_t bus)
{
    assert(bus == 1);
    return TwoWire::active->busHandleAvailable ? TwoWire::active : nullptr;
}
inline esp_err_t recordI2cWrite(TwoWire& wire, uint16_t address, const uint8_t* data, size_t length, uint32_t timeout)
{
    assert(timeout == 20);
    ++wire.transmissions;
    wire.address = address;
    wire.sent.emplace_back(data, data + length);
    if (afterWrite) afterWrite();
    if (wire.statuses.empty()) return ESP_OK;
    const auto result = wire.statuses.front();
    wire.statuses.pop_front();
    return result;
}
inline esp_err_t i2cWrite(uint8_t bus, uint16_t address, const uint8_t* data, size_t length, uint32_t timeout)
{
    assert(bus == 1);
    ++TwoWire::active->halWrites;
    return recordI2cWrite(*TwoWire::active, address, data, length, timeout);
}
