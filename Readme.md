# OAM-AccessControl

OpenKNX Firmware und Anwendung zur OpenKNX-kompatiblen Einbindung von Fingerabdruck- und NFC-Lesern.

## Inbetriebnahme

Die Inbetriebnahme geht am einfachsten mit der [OpenKNX Toolbox](https://openknx.atlassian.net/wiki/spaces/OpenKNX/pages/66295/OpenKNX-Toolbox).

## Kompatible Fingerabdruckleser

Aktuell werden folgende Fingerabdruckleser unterstützt:
- R503 (Speicherkapazität: 200 Finger)
- R503S (Speicherkapazität: 150 Finger)
- R503Pro (Speicherkapazität: 1500 Finger)

Weitere Fingerabdruckleser können bei Bedarf angebunden werden.

## Kompatible NFC-Leser

Aktuell werden NFC-Leser basierend auf dem NXP PN7160 unterstützt.

## Kompatible Hardware

Die kompakte Steuerplatine kann optional vollständig in ein Standard-Schalterprogramm integriert werden. Hier als Beispiel mit Gira-Schalterprogramm in matt schwarz und Edelstahlrahmen:

<kbd>![Gira vorne](hardware/AB-SmartHouse-AccessControlWithGiraFront.jpg)</kbd>

Eine Funktionsbeschreibung sowie kompatible Hardware findet sich [hier](https://openknx.atlassian.net/wiki/spaces/OpenKNX/pages/6553602/Zutrittskontrolle).

## Local builds and ESP32 PN7160 support

Use the dependency revisions in `dependencies.txt` for this application's ETS
header. The `lib/` entries link separate local repositories; newer shared
checkouts can fail module version checks. Do not bypass those checks or edit
`knxprod.h` to hide a mismatch.

The local build helper clones committed local source and the recorded revisions
onto `v1dev_HomeKey`, without switching shared checkouts or pushing. It isolates
PlatformIO's library directory to avoid collisions with other local builds.
PlatformIO may download toolchains and third-party libraries. Choose a new build
directory for each snapshot:

```sh
python3 scripts/Build-HomeKey-Local.py --build-dir /tmp/oam-homekey-parity-validation
```

The helper builds these targets by default; use `-e` to select individual ones:

| Target | Configuration |
| --- | --- |
| `debug_RP2040` | Existing RP2040 target and PN7160 implementation |
| `debug_ESP32S3_HomeKey` | XIAO ESP32-S3 with PN7160 I2C and existing access-control features |
| `debug_ESP32S3_PN7160_Peripherals` | ESP32 PN7160 plus fingerprint UART, Gira/matrix keypad, touch/LED expanders, four binary inputs and one bistable relay |

Despite its development name, the ESP32 HomeKey target currently uses the legacy
PN7160 protocol and UID-based credentials. PN7161 and Apple Home Key
provisioning/authentication belong to a later implementation stage.

The ESP32 backend uses the same PN7160 NCI engine, tag detection/removal, UID
enrollment and lookup, ETS action mappings and KNX synchronization as RP2040.
It adds explicit I2C bus/pin selection, 258-byte packet buffers, bounded timeouts
and rejection of incomplete transfers. Invalid UID lengths are rejected on ESP32.
`OPENKNX_ACC_DISABLE_LEGACY_NFC` is no longer set in these targets. The old
RP2040 transport body and platform-specific peripheral operations are retained.

The application links the separately committed local `PN7160` checkout. Its
OGM-Common dependency uses an isolated worktree under `.homekey-repos/OGM-Common`
at the compatible 1.8 branch with an ESP32 I2C initialization backport; the shared
OGM-Common development checkout remains on its original branch.

### XIAO ESP32-S3 prototype wiring

These assignments are a prototype configuration, not a validated carrier board.
Confirm physical connections before flashing. KNX requires an external TP
transceiver, and the peripheral profile requires the listed extra components.

| Function | GPIO / connection |
| --- | --- |
| KNX UART1 | TX GPIO43 (D6), RX GPIO44 (D7) |
| PN7160 and shared I2C | SDA GPIO5 (D4), SCL GPIO6 (D5), PN7160 address 0x28 |
| PN7160 control | IRQ GPIO3 (D2), VEN GPIO4 (D3) |
| Onboard programming LED | GPIO21, active low |
| Optional fingerprint UART2 | RX GPIO1 (D0), TX GPIO2 (D1), touch GPIO7 (D8) |
| Optional bistable relay | SET GPIO8 (D9), RESET GPIO9 (D10), active low; external driver required |
| Direct touch/LED and binary-input expander | PCA9554 address 0x21, GPIO namespace 0x01xx |
| External touch/LED expander | PCA9554 address 0x20, GPIO namespace 0x02xx |
| Matrix keypad expander | PCA9554 address 0x22, GPIO namespace 0x03xx |
| Gira keypad | BS8116 address 0x50 and PCA9633 LED driver address 0x62 |

ESP32 expander touch inputs are polled instead of being registered as MCU GPIO
interrupts. Keypad reads temporarily use 100 kHz; the shared bus otherwise uses
400 kHz. The Gira keypad uses ESP32's bounded Wire timeout and backs off while a
device does not ACK. RP2040 retains its existing timeout recovery and interrupts.
The main ESP32 target safely handles an absent fingerprint sensor. Use the
peripheral target for the prototype that connects those devices.

Both ESP32 profiles use the experimental 8 MB partition table with two 3 MB OTA
slots, a 32 KB OpenKNX partition, separate fingerprint/NFC/keypad partitions and
reserved Home Key NVS space. Credential records retain their existing format.
Home Key credential storage and encryption are not implemented. The OpenKNX
partition is kept below 64 KB because the shared persistence code uses 16-bit
slot sizes.

### Validation

Local compile/link checks on 2026-10-09 passed for RP2040, ESP32 PN7160 and the
ESP32 peripheral profile. Native tests passed for initialization, bus selection,
IRQ/VEN, I2C failures, 258-byte packets and the shared NCI engine's reset,
4/7/10-byte UIDs, repeat suppression and tag removal. Scanner/GPIO checks verify
safe operation when hardware is absent and preservation of real pin IDs.

The unchanged RP2040 baseline was built first with the recorded dependency
snapshot. An initial ESP32 attempt failed on the RP2040 driver's Teensy fallback;
the dedicated ESP32 transport resolves that failure. Build libraries are now
isolated because the old shared `/tmp/libdeps` could be replaced by other builds.

No physical reader, fingerprint sensor, KNX traffic or HomeKit tests have been
performed. On-device validation is still needed to confirm timing and wiring.

```sh
python3 lib/PN7160/test/esp32/run.py
c++ -std=c++17 -DARDUINO_ARCH_ESP32 \
  -Ilib/OFM-AccessControl/test/native/stubs -Ilib/OFM-AccessControl/src \
  lib/OFM-AccessControl/test/native/port_test.cpp \
  lib/OFM-AccessControl/src/Fingerprint.cpp -o /tmp/accesscontrol-parity-test
/tmp/accesscontrol-parity-test
```

On this machine, the existing PlatformIO Python could not load `liblzma`; use a
temporary Python environment and the isolated toolchain cache:

```sh
python3 -m venv /tmp/oam-homekey-pio-venv
/tmp/oam-homekey-pio-venv/bin/python -m pip install platformio
PLATFORMIO_CORE_DIR=/tmp/oam-homekey-platformio \
  python3 scripts/Build-HomeKey-Local.py \
  --build-dir /tmp/oam-homekey-parity-validation \
  --pio /tmp/oam-homekey-pio-venv/bin/pio
```
