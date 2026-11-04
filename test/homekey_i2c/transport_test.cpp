#include "Pn7161I2cTransport.h"
#include <Arduino.h>
#include <freertos/task.h>
#include <esp32-hal-i2c.h>
#include <esp_idf_version.h>
#include <esp_log.h>
#include <cassert>
#include <iostream>

int main()
{
    TwoWire wire;
    Pn7161I2cTransport reader(wire, 21, 22, 14, 13, 19, 0x28, 400000);
    uint8_t buffer[258] = {};
    assert(reader.read(buffer, 3) == ESP_ERR_INVALID_STATE);
    assert(reader.init() == ESP_OK);
    assert(reader.init() == ESP_OK); // Idempotent: no second device handle.
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)
    assert(wire.deviceAdds == 1 && wire.liveDevices == 1);
#endif
    assert(wire.sda == 21 && wire.scl == 22 && wire.clock == 400000);
    assert(wire.timeout == 20 && levels[19] == LOW && levels[13] == LOW);
    reader.set_ven(true);
    assert(levels[13] == HIGH);

    // Exercise the actual NCI header/payload split and maximum-size frame.
    wire.incoming = {{0x60, 0x05, 255}, std::vector<uint8_t>(255, 0xA5)};
    assert(reader.read(buffer, 3) == ESP_OK && buffer[2] == 255);
    assert(reader.read(buffer + 3, 255) == ESP_OK && buffer[257] == 0xA5);
    wire.incoming = {{0x60}};
    assert(reader.read(buffer, 3) == ESP_FAIL && buffer[0] == 0);
    assert(reader.read(nullptr, 3) == ESP_ERR_INVALID_ARG);
    assert(reader.read(buffer, 259) == ESP_ERR_INVALID_ARG);

    const uint8_t command[] = {0x20, 0, 1, 1};
    levels[14] = HIGH;
    const auto before = ticks;
    assert(reader.write(command, sizeof(command)) == ESP_ERR_TIMEOUT);
    assert(wire.transmissions == 0 && ticks - before == 20);
    levels[14] = LOW;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
    constexpr auto nack = ESP_ERR_INVALID_RESPONSE;
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)
    constexpr auto nack = ESP_ERR_INVALID_STATE;
#else
    constexpr auto nack = ESP_FAIL;
#endif
    wire.statuses = {nack, ESP_OK}; // Standby NACK resends the complete frame.
    assert(reader.write(command, sizeof(command)) == ESP_OK);
    assert(wire.transmissions == 2 && wire.address == 0x28);
    assert(wire.sent[0] == std::vector<uint8_t>(command, command + sizeof(command)));
    assert(wire.sent[1] == wire.sent[0]);
    assert(ticks - before == 25); // Initial IRQ timeout plus one 5 ms wake-up delay.
    assert(errorLogs.empty()); // Recovered standby NACKs are not errors.
    assert(debugLogs.size() == 1 && debugLogs.back().find("recovered") != std::string::npos);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)
    assert(wire.halWrites == 0); // No premature Arduino i2cWrite error log.
#endif

    // A timeout could follow an accepted packet: never replay it or other errors.
    for (auto error : {ESP_ERR_TIMEOUT, ESP_ERR_INVALID_ARG, ESP_ERR_NO_MEM})
    {
        wire.statuses = {error, ESP_OK};
        const auto attempts = wire.transmissions;
        const auto errors = errorLogs.size();
        assert(reader.write(command, sizeof(command)) == error);
        assert(wire.transmissions == attempts + 1 && wire.statuses.size() == 1);
        assert(errorLogs.size() == errors + 1); // Real failures remain visible.
    }
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
    wire.statuses = {ESP_ERR_INVALID_STATE, ESP_OK};
    const auto invalidStateAttempts = wire.transmissions;
    assert(reader.write(command, sizeof(command)) == ESP_ERR_INVALID_STATE);
    assert(wire.transmissions == invalidStateAttempts + 1); // No longer a NACK on IDF 6.
#endif
    wire.statuses = {nack, nack, nack, ESP_OK};
    const auto attempts = wire.transmissions;
    const auto errors = errorLogs.size();
    assert(reader.write(command, sizeof(command)) == nack);
    assert(wire.transmissions - attempts == 3 && wire.statuses.size() == 1);
    assert(errorLogs.size() == errors + 1 && errorLogs.back().find("3 attempt") != std::string::npos);

    wire.statuses.clear();
    wire.busInitialized = false;
    assert(reader.write(command, sizeof(command)) == ESP_ERR_INVALID_STATE);
    assert(wire.transmissions - attempts == 3); // An uninitialized bus is not a NACK.
    wire.busInitialized = true;

    // NXP allows IRQ to rise during a successful write (a response is waiting).
    afterWrite = [] { levels[14] = HIGH; };
    assert(reader.write(command, sizeof(command)) == ESP_OK);
    levels[14] = LOW;
    wire.statuses = {nack, ESP_OK};
    const auto blocked = wire.transmissions;
    assert(reader.write(command, sizeof(command)) == ESP_ERR_TIMEOUT);
    assert(wire.transmissions == blocked + 1); // Pending inbound data blocks retries.
    afterWrite = {};
    levels[14] = LOW;
    wire.statuses.clear();

    ticks = UINT32_MAX - 2;
    assert(reader.wait_for_irq(true, 5) == ESP_ERR_TIMEOUT); // Tick wrap.
    levels[14] = HIGH;
    assert(reader.wait_for_irq(true, 0) == ESP_OK);
    reader.deinit(); reader.deinit();
    assert(levels[13] == LOW);
    assert(wire.busInitialized); // Removing our handle must leave Wire alive.
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)
    assert(wire.deviceRemoves == 1 && wire.liveDevices == 0);
#endif
    assert(reader.wait_for_irq(true, 0) == ESP_ERR_INVALID_STATE);
    assert(reader.init() == ESP_OK);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)
    assert(wire.deviceAdds == 2 && wire.liveDevices == 1);
#endif
    reader.deinit();

    TwoWire failed;
    failed.capacity = 128;
    Pn7161I2cTransport unavailable(failed, 21, 22, 14, 13, 19, 0x28, 400000);
    assert(unavailable.init() == ESP_ERR_NO_MEM);
    failed.capacity = 258; failed.beginOk = false;
    assert(unavailable.init() == ESP_FAIL);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)
    failed.beginOk = true; failed.busHandleAvailable = false;
    assert(unavailable.init() == ESP_ERR_INVALID_STATE);
    failed.busHandleAvailable = true; failed.deviceAddStatus = ESP_ERR_NO_MEM;
    assert(unavailable.init() == ESP_ERR_NO_MEM);
    assert(unavailable.write(command, sizeof(command)) == ESP_ERR_INVALID_STATE);
    assert(failed.liveDevices == 0 && failed.deviceAdds == 0);
    failed.deviceAddStatus = ESP_OK;
    assert(unavailable.init() == ESP_OK);
    unavailable.deinit();
    assert(failed.liveDevices == 0 && failed.deviceAdds == 1 && failed.deviceRemoves == 1);
#endif
    std::cout << "Home Key I2C transport checks passed\n";
}
