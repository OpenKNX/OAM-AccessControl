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

## Local builds and experimental Home Key target

Use the dependency revisions in `dependencies.txt` for this application's existing
ETS header. The `lib/` entries are links to separate repositories; newer shared
checkouts can fail the module version checks. Do not bypass those checks or edit
the generated `knxprod.h` to hide a mismatch.

```sh
pio run -e debug_RP2040
pio run -e debug_ESP32S3_HomeKey
```

`debug_ESP32S3_HomeKey` selects **Seeed Studio XIAO ESP32-S3**, the shared
OGM-Common PioArduino/Arduino configuration, USB serial, and an experimental 8 MB
partition table. It reserves two 3 MB OTA slots, the existing KNX/OpenKNX data,
separate fingerprint/NFC/keypad partitions, and a future Home Key NVS partition.
The Home Key partition is only reserved space; credential storage and encryption
are not implemented. `PN7160_RP2040` remains enabled only in RP2040 environments.

The provisional KNX connection is UART1, TX GPIO43 (D6), RX GPIO44 (D7), requiring
an external KNX TP transceiver. Peripheral pins and physical wiring still need
confirmation. This target retains the application's existing module registrations;
it does not yet implement Home Key, HomeKit provisioning, or an ESP32 NFC backend.
Scanner, keypad, touch, LED and actuator mappings are intentionally not inherited
from the RP2040 boards. Do not upload this experimental target before completing
the platform port and validating its wiring.

Local validation on 2026-10-09 (application base `810e212`):

| Build | Result |
| --- | --- |
| Unchanged RP2040 with installed shared libraries | Failed: LogicModule 4.4 versus ETS 4.3 |
| Unchanged RP2040 with exact `dependencies.txt` revisions | Compile and link passed; UF2 generated |
| RP2040 after adding the ESP32 environment, same pinned revisions | Compile and link passed; RAM 67,140 bytes, flash 696,272 bytes, unchanged from baseline |
| XIAO ESP32-S3 with exact `dependencies.txt` revisions | Failed in legacy PN7160: missing Teensy `i2c_t3.h` |

Tests used isolated local dependency clones in `/tmp/oam-homekey-build`, each on
`v1dev_HomeKey`, preserving the shared repositories. The successful baseline used
OGM-Common `f6dbb3c`, Arduino-Pico 5.4.4 and its pinned Raspberry Pi platform.
The ESP32 attempt used PioArduino `54.03.21-2`, Arduino-ESP32 3.2.1 and GCC 14.2.
The partition table was checked for alignment, overlaps and the 8 MB boundary.
No hardware boot, KNX traffic, NFC or HomeKit tests have been performed.

The next port must disable/replace the RP2040 PN7160 path without introducing a
second reader owner, adapt AccessControl's flash initialization to ESP32 named
partitions, and handle absent fingerprint/peripheral devices safely. These are
source changes in OFM-AccessControl, separate from adding a build environment.

On this machine, the existing PlatformIO Python could not load `liblzma`. A
temporary Python environment allowed the ESP32 toolchain to install:

```sh
python3 -m venv /tmp/oam-homekey-pio-venv
/tmp/oam-homekey-pio-venv/bin/python -m pip install platformio
PLATFORMIO_CORE_DIR=/tmp/oam-homekey-platformio \
  /tmp/oam-homekey-pio-venv/bin/pio run \
  -d /tmp/oam-homekey-build -e debug_ESP32S3_HomeKey
```
