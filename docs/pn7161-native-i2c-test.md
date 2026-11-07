# Native PN7161 I²C transport: local test

This implements the native transport direction requested in the closed
[rednblkx/pn7160 PR #1](https://github.com/rednblkx/pn7160/pull/1#issuecomment-6707485693).
The changes are maintained in the private repositories for hardware testing;
the public PRs remain unchanged.

## Changes

- **PN7161:** `i2c.hpp` / `i2c.cpp` add `PN7160_I2C`, using only ESP-IDF APIs.
  CMake adds the native source/dependency on IDF 5.4+. Native tests and a
  standalone ESP-IDF reset/init example accompany it.
- **OFM-AccessControl:** owns the `Pn7161Reader` facade and retains its donor
  notice alongside the unchanged original module license. An optional caller-owned
  transport allows the existing NCI/discovery/UID/APDU/ECP path to use the new
  native transport. SPI profiles retain the native SPI transport. No Wire transaction fallback remains.
  An application-only bridge obtains the actual Wire
  controller's existing native bus. If no GPIO owner started Wire, the bridge
  starts that owner once during setup. It never starts another native bus.
  It registers the NFC device before workers start and retains it until reboot,
  including while NFC is disabled or recovering. Recovery stops NCI and lowers
  VEN but does not remove/re-register the device or restart Wire.
- **OAM-AccessControl:** all PN7161 I2C profiles now select the native transport
  automatically; there is no fallback-selection build flag. The configured ESP32
  targets pin a compatible SDK. RP2040 and other reader backends are unchanged.

Only the native transport, CMake changes, native tests/example and its independent
documentation belong in a future upstream candidate. The application reader facade
and the application bridge remain separate.

## Build and wiring

```sh
~/.platformio/penv/bin/pio run -e debug_REG1_ESP_PN7161_IP
```

Artifact: `.pio/build/debug_REG1_ESP_PN7161_IP/firmware.bin`.
Use the same upload method as for your previous firmware. No board was flashed
as part of implementation. No ETS parameter/application version changes are
required by this transport change, and credential storage is unchanged.

REG1 ESP v00.11 wiring remains:

| PN7161 | ESP32 |
|---|---|
| SDA | GPIO21 |
| SCL | GPIO22 |
| IRQ | GPIO14 |
| VEN | GPIO13 |
| DWL | GPIO19 |
| VDD, VANT | 3.3 V |
| GND | GND |

Address is unshifted `0x28`, speed 400 kHz. Existing Wi-Fi networking remains;
the listed GPIOs overlap Ethernet RMII. Use suitable external I²C pull-ups.

## Hardware acceptance checklist

1. Boot with your existing ETS configuration. Expect `NFC: Native I2C ready:`
   with IDF version, controller, SDA/SCL and address, followed by the existing
   reader/discovery readiness message. Save the full startup log, especially any
   NCI reset/init failures and firmware identity. The standalone example can
   provide an isolated reset/init check without discovery or enrollment.
2. Scan a registered UID tag and an authorized iPhone/Watch. Compare action,
   held-tag suppression and subsequent scans with the previous firmware.
   Check the web page and normal KNX functions while scanning repeatedly.
3. Disable/re-enable NFC through the existing configuration or use the existing
   reader test/recovery workflow. Confirm subsequent scans recover and the
   `Native I2C ready` registration message is not repeated for reader recovery.
   Compare free heap over repeated restarts. Do not short pins to force failures.
4. For shared-bus acceptance, connect a known second peripheral at a different
   address on GPIO21/22. Initialize its Wire device/driver before NFC workers
   start, then exercise its documented reads alongside repeated NFC traffic and
   reader restarts. Record failures, latency and free heap. A mock device count
   is not evidence of this physical test.

The shared bus must keep its pins, controller and device registrations stable.
All other Wire devices must be initialized before background NFC traffic starts;
runtime device add/remove, `Wire.end()`, `begin()`/`setPins()` reconfiguration,
clock changes and bus-wide scans are outside this trial's ownership contract.
Never access/probe NFC address `0x28` through Wire. Synchronous transfers on
already-registered devices use ESP-IDF's bus operation semaphore; lifecycle APIs
do not have the same thread-safety guarantee. See the
[ESP-IDF 5.5.4 thread-safety documentation](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32/api-reference/peripherals/i2c.html#thread-safety).

The Gira keypad changes the clock and recreates device handles during operation.
The native trial therefore disables NFC with an explicit error if that keypad
is selected in ETS. Use no keypad or another supported selection for this test.
Broader shared-peripheral support requires coordinating all lifecycle operations.

## Validation and remaining gates

Reference build: classic ESP32, pioarduino `55.03.38-1`, Arduino-ESP32 3.3.8,
ESP-IDF 5.5.4, C++23. Starting commits: application `7510dda`, PN7161 `d028288`,
OFM-AccessControl `51e4c8e`, OGM-Common `73bd379`. This local tree already contained
ESP32 environments and the common NFC service; the attached plan's older public
repository assumptions do not describe these private branches.

- REG1 IP firmware builds, including the unchanged SPI and NCI sources.
- Native-only host tests exercise resource cleanup, packet framing, IRQ timing,
  NACK retry limits and ambiguous errors without Arduino headers.
- Native simulated API-version configurations 5.4/5.5/6.0 are exercised with
  stubs; the installed SDK build is 5.5.4. The obsolete Wire transport tests
  were removed with that transport. Shared-service checks cover missing native
  bus and unsupported keypad configuration without starting an NFC worker.
- Standalone `idf.py` build is pending: a standalone ESP-IDF installation is not
  available here. Compiling native sources in the actual firmware SDK is useful
  evidence, but does not replace this independent build.
- Physical reset/init, shared-bus parallel stress, UID/HomeKey scans and recovery
  are pending. No real-device behavior or runtime memory improvement is claimed.
- RP2040/other firmware targets were not built; repository instructions restrict
  firmware validation to REG1 IP unless another target is explicitly requested.

The test firmware deliberately includes the bridge before hardware acceptance
so you can run those checks. A replacement upstream PR should wait for successful
hardware results and an independent native-IDF build.
