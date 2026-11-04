#pragma once
#include <esp32-hal-i2c.h>
#include <algorithm>
#include <cstring>
inline std::function<void()> afterRead;

using i2c_master_bus_handle_t = TwoWire*;
struct TestI2cDevice { TwoWire* bus; uint16_t address; };
using i2c_master_dev_handle_t = TestI2cDevice*;
inline constexpr int I2C_ADDR_BIT_LEN_7 = 0;
struct i2c_device_config_t {
    int dev_addr_length;
    uint16_t device_address;
    uint32_t scl_speed_hz;
    uint32_t scl_wait_us;
    struct { unsigned disable_ack_check : 1; } flags;
};
inline esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus,
    const i2c_device_config_t* config, i2c_master_dev_handle_t* handle)
{
    assert(bus && bus->busInitialized);
    assert(config->dev_addr_length == I2C_ADDR_BIT_LEN_7 && config->device_address == 0x28);
    assert(config->scl_speed_hz == 400000 && config->scl_wait_us == 0);
    assert(!config->flags.disable_ack_check); // Never hide a missing ACK.
    if (bus->deviceAddStatus != ESP_OK) return bus->deviceAddStatus;
    *handle = new TestI2cDevice{bus, config->device_address};
    ++bus->deviceAdds; ++bus->liveDevices;
    return ESP_OK;
}
inline esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t handle)
{
    assert(handle && handle->bus->liveDevices == 1);
    ++handle->bus->deviceRemoves; --handle->bus->liveDevices;
    delete handle;
    return ESP_OK;
}
inline esp_err_t i2c_master_transmit(i2c_master_dev_handle_t handle,
    const uint8_t* buffer, size_t length, int timeout)
{
    assert(handle && handle->bus->liveDevices == 1);
    return recordI2cWrite(*handle->bus, handle->address, buffer, length, timeout);
}
inline esp_err_t i2c_master_receive(i2c_master_dev_handle_t handle,
    uint8_t* buffer, size_t length, int timeout)
{
    assert(handle && handle->bus->liveDevices == 1 && timeout == 20);
    auto& wire = *handle->bus;
    ++wire.receives;
    if (afterRead) afterRead();
    if (!wire.readStatuses.empty()) {
        const auto status = wire.readStatuses.front(); wire.readStatuses.pop_front();
        if (status != ESP_OK) { buffer[0] = 0xEE; return status; }
    }
    if (wire.incoming.empty()) return ESP_FAIL;
    const auto data = wire.incoming.front(); wire.incoming.pop_front();
    std::memcpy(buffer, data.data(), std::min(length, data.size()));
    return data.size() == length ? ESP_OK : ESP_FAIL;
}
