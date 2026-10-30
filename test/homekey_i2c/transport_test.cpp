#include "Pn7161I2cTransport.h"
#include <Arduino.h>
#include <freertos/task.h>
#include <cassert>
#include <iostream>

int main()
{
    TwoWire wire;
    Pn7161I2cTransport reader(wire, 21, 22, 14, 13, 19, 0x28, 400000);
    uint8_t buffer[258] = {};
    assert(reader.read(buffer, 3) == ESP_ERR_INVALID_STATE);
    assert(reader.init() == ESP_OK);
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
    wire.statuses = {2, 0}; // Standby address NACK may be retried.
    assert(reader.write(command, sizeof(command)) == ESP_OK);
    assert(wire.transmissions == 2 && wire.address == 0x28);
    assert(wire.sent.back() == std::vector<uint8_t>(command, command + sizeof(command)));
    wire.statuses = {3, 0}; // Data NACK must never duplicate a partial command.
    assert(reader.write(command, sizeof(command)) == ESP_FAIL);
    assert(wire.transmissions == 3 && wire.statuses.size() == 1);
    wire.statuses.clear();
    wire.shortWrite = 1;
    assert(reader.write(command, sizeof(command)) == ESP_FAIL);
    wire.shortWrite = 0;
    wire.statuses = {2, 2, 2, 0};
    const auto attempts = wire.transmissions;
    assert(reader.write(command, sizeof(command)) == ESP_FAIL);
    assert(wire.transmissions - attempts == 3);

    ticks = UINT32_MAX - 2;
    assert(reader.wait_for_irq(true, 5) == ESP_ERR_TIMEOUT); // Tick wrap.
    levels[14] = HIGH;
    assert(reader.wait_for_irq(true, 0) == ESP_OK);
    reader.deinit(); reader.deinit();
    assert(levels[13] == LOW);
    assert(reader.wait_for_irq(true, 0) == ESP_ERR_INVALID_STATE);
    assert(reader.init() == ESP_OK);

    TwoWire failed;
    failed.capacity = 128;
    Pn7161I2cTransport unavailable(failed, 21, 22, 14, 13, 19, 0x28, 400000);
    assert(unavailable.init() == ESP_ERR_NO_MEM);
    failed.capacity = 258; failed.beginOk = false;
    assert(unavailable.init() == ESP_FAIL);
    std::cout << "Home Key I2C transport checks passed\n";
}
