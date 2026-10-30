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
header. The snapshot now records the currently linked local repositories,
including KNX 2.4.0, Common 1.9.1, LogicModule 4.4.1, ConfigTransfer 0.6,
SwitchActuator 0.3, VirtualButton 0.7 and BinaryInput 0.4. The XML verifies these
module versions. Do not bypass those checks or edit `knxprod.h` to hide a mismatch.

The release ETS application version is 1.0 (encoded as 16, following 0.15), and
the development version is 10.15. SwitchActuator global objects now use numbers
8–10; its channel objects remain at 91–99. Regenerate/import the matching ETS
application when using this firmware. Normal builds use `/tmp/oam-accesscontrol-libdeps`; isolated builds use a
separate project-specific cache under `/tmp`. The R503Pro XML includes the keypad and
feedback definitions referenced by the shared AccessControl description.

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
| `debug_ESP32S3_AppleHomeKey` | ESP32 HomeSpan provisioning, DigitalDoorKey and PN7161 SPI prototype |

Despite its development name, the ESP32 HomeKey target currently uses the legacy
PN7160 protocol and UID-based credentials. Apple Home Key provisioning and
authentication use the separate `debug_ESP32S3_AppleHomeKey` target.

The ESP32 backend uses the same PN7160 NCI engine, tag detection/removal, UID
enrollment and lookup, ETS action mappings and KNX synchronization as RP2040.
It adds explicit I2C bus/pin selection, 258-byte packet buffers, bounded timeouts
and rejection of incomplete transfers. Invalid UID lengths are rejected on ESP32.
`OPENKNX_ACC_DISABLE_LEGACY_NFC` is no longer set in these targets. The old
RP2040 transport body and platform-specific peripheral operations are retained.

The application links the separately committed local `PN7160` checkout. Its
OGM-Common dependency uses an isolated worktree under `.homekey-repos/OGM-Common`
at the unmodified upstream `v1dev` revision `f099eb9` (1.9.1), which already
includes ESP32 I2C initialization. The shared OGM-Common development checkout
remains on its original branch. The ETS configuration uses Common 1.9 and
requires OpenKNXproducer 4.3.12 or newer. The checked-in R503 header was
regenerated from `src/AccessControl-ReleaseR503.xml`.

### REG1-ipTpControllerESP-Devboard v00.11 with PN7161

Choose a target with `pio run -e <environment>`:

| Environment | KNX transport |
| --- | --- |
| `debug_REG1_ESP_PN7161_TP` | TP, mask 0x07B0 |
| `debug_REG1_ESP_PN7161_IP` | KNX IP over Wi-Fi, mask 0x57B0 |
| `debug_REG1_ESP_PN7161_TPIP` | KNX exclusively over TP, mask 0x07B0; Wi-Fi/IP for Apple Home Key |

`debug_REG1_ESP_PN7161` remains an alias for the TP environment.
TPIP inherits the TP configuration. HomeSpan manages its Wi-Fi connection for
HomeKit/Home Key; KNXnet/IP tunneling and routing are disabled. Wi-Fi connection
changes do not restart the KNX TP stack. Since all REG1 targets enable Home Key,
the TP and TPIP environments currently have the same transport configuration.
IP firmware requires a matching
ETS application for mask 0x57B0; the existing TP ETS application cannot be used
unchanged for that target.

Apple Home Key is compiled into all three REG1 environments (and the TP alias).
They select `include/homekey/knxprod.h` and exclusively use OFM-HomeKey's
PN7161 I2C backend for ECP, provisioning, authentication and optional UID tags.
Use the Home Key ETS application and commissioning steps below; Home Key must
still be enabled and its endpoints explicitly authorized in ETS.

The Wi-Fi targets connect as a station using credentials stored in NVS.
For initial setup, enter `wifi <SSID> <password>` in the local serial console
(`hk wifi <SSID> <password>` also works). This is available immediately, before
ETS configuration or Home Key enablement, and saves the network for KNX IP
startup and HomeSpan. After Home Key starts, network changes run on its worker.
Diagnostic KNX commands cannot change Wi-Fi credentials. The command is listed
in console help. An open network can be selected with `wifi <SSID>`.

Alternatively, provide quoted string build flags `OPENKNX_WIFI_SSID` and
`OPENKNX_WIFI_PASSWORD` in a local environment override, for example:

```ini
[env:local_REG1_IP]
extends = env:debug_REG1_ESP_PN7161_IP
build_flags =
  ${env:debug_REG1_ESP_PN7161_IP.build_flags}
  -D OPENKNX_WIFI_SSID=\"your-network\"
  -D OPENKNX_WIFI_PASSWORD=\"your-password\"
```

Keep credentials out of version control. Wi-Fi connection proceeds without
blocking KNX TP startup. HomeSpan uses the same network for Home Key.
On the TP and TPIP environments, Home Key also needs Wi-Fi for HomeKit
provisioning; use the same console command or a local build override.

These environments use the
REG1 v00.11 hardware definition (KNX UART1 TX GPIO5, RX GPIO37), the RGB
front panel, and a 4 MB flash layout with a 3 MB application, credential storage
and a separate 128 KB `homekey` NVS partition. Flash the factory image to install
the new partition table. Data from the earlier REG1 partition layout is not
migrated; these 4 MB targets have one application slot.
Fingerprint, keypad, relay and binary-input wiring is not configured.

| PN7161 signal | Connection |
| --- | --- |
| DWL | GPIO19 (held low for normal NCI operation) |
| SDA | GPIO21 |
| SCL | GPIO22 |
| IRQ | GPIO14 |
| VEN | GPIO13 |
| VDD | 3.3 V |
| GND | GND |
| VANT | 3.3 V |

The I2C address is 0x28 and the bus runs at 400 kHz. GPIO19/21/22 are shared
with Ethernet RMII, so Ethernet must remain disconnected and disabled with
this wiring. The Home Key reader applies the fixed 3.3 V PMU/TXLDO preset for
VANT=3.3 V, following the [module vendor's supply configuration](https://www.elechouse.com/docs/pn7161/).
Reader operation and phone/watch provisioning require hardware validation.

Native I2C transport checks (header/payload framing, transfer failures, standby
NACK retry, IRQ timeout/tick wrap and reset):

```sh
c++ -std=c++17 -DOPENKNX_HOMEKEY_ENABLED -DARDUINO_ARCH_ESP32 \
  -Itest/homekey_i2c/stubs -Ilib/OFM-HomeKey/src -Ilib/PN7161/include \
  test/homekey_i2c/transport_test.cpp lib/OFM-HomeKey/src/Pn7161I2cTransport.cpp \
  -o /tmp/oam-homekey-i2c-test
/tmp/oam-homekey-i2c-test
```

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
The separate Apple Home Key target implements credential storage; encryption
is not enabled in this development layout. The OpenKNX
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

The default PlatformIO environment on this machine uses Python 3.14 with working
LZMA support. Build the current workspace directly:

```sh
~/.platformio/penv/bin/pio run -e debug_ESP32S3_HomeKey
```

To build a separate snapshot of committed sources:

```sh
python3 scripts/Build-HomeKey-Local.py \
  --build-dir /tmp/oam-homekey-current-validation \
  --pio ~/.platformio/penv/bin/pio
```


## Apple Home Key implementation (ESP32 prototype)

`debug_ESP32S3_AppleHomeKey` and the REG1 environments enable the separately
versioned OFM-HomeKey module, HomeSpan NFC Access provisioning, DigitalDoorKey
authentication, and persistent explicit issuer/endpoint-to-NFC-ID mappings.
REG1 uses PN7161 I2C; XIAO uses SPI. The existing RP2040 and XIAO ESP32 PN7160
I2C environments retain their reader paths. The Home Key targets exclude the
legacy reader so only one module owns NFC. HomeKit pairing and UID
identification do not authorize a Home Key endpoint; authenticated mapped IDs
reuse the existing NFC action engine on the OpenKNX thread.

```sh
~/.platformio/penv/bin/pio run -e debug_ESP32S3_AppleHomeKey
```

Use `src/AccessControl-HomeKey.xml` (development application A6/02, version 0.3)
and `include/homekey/knxprod.h` together. The build script selects that header
before all module compilations; existing application XML/header selection is
retained for the other targets. Generate it with OpenKNXproducer 4.3.12:

```sh
/path/to/OpenKNXproducer create src/AccessControl-HomeKey.xml \
  -h include/homekey/knxprod.h -o /tmp/AccessControl-HomeKey.knxprod -d
```

On this Mac the producer generates the header and checks the expanded XML;
ETS product signing is skipped and an XSD is not installed. Generate/sign the
ETS import product with the appropriate producer setup on Windows.

Configure Wi-Fi from the local serial console with `wifi <SSID> <password>`
before ETS setup if needed. Enable Home Key and commissioning in its ETS
parameters, then set a private HomeKit code locally using `hk code <8digits>`.
Use the ETS parameter-page buttons to open/close the
five-minute pairing window and refresh its status. The buttons use the device's
individual address through the ETS KNX interface. Pair in Apple Home with the
locally configured code; KO 12/13/64 remain available for Group Monitor control.

Enable the ETS authorization table, then press `Provisionierte Geräte-IDs
importieren`. The script adds full public issuer/endpoint IDs to completely
empty rows, preserves existing rows and rights, and leaves new rows disabled.
Assign unused NFC action IDs 0..1499, configure the corresponding NFCACT actions,
enable the rows and download. Importing IDs alone grants no access. The script
checks snapshot consistency and available capacity before writing. KO 14 reports
table validity, pending endpoints and UID collisions. Local `hk map` /
`hk unmap` is available when ETS authorization is off. An invalid/empty ETS
policy cannot use a saved local mapping as fallback.

The Home Key application/header is now version 0.3; regenerate and use the
matching ETS product for this firmware. All existing RP2040/legacy applications
retain their versions, parameters and object numbers. Full object/status tables,
commissioning, retention and revocation details are in
[OFM-HomeKey/Readme.md](lib/OFM-HomeKey/Readme.md).

PN7161 SPI prototype pins: CS GPIO1 (D0), SCLK GPIO7 (D8), MISO GPIO8 (D9),
MOSI GPIO9 (D10), IRQ GPIO3 (D2), VEN GPIO4 (D3). They conflict with the optional
fingerprint/relay profile; the build rejects that combination. Lock feedback
uses KO 11 (1=physically locked). Home app remote unlock is not enabled.

The three XIAO environments (`debug_ESP32S3_HomeKey`,
`debug_ESP32S3_PN7160_Peripherals`, `debug_ESP32S3_AppleHomeKey`) locally pin
PioArduino 55.03.38-1 (Arduino 3.3.8 / ESP-IDF 5.5.4) through their shared
HomeKey environment. The OGM-Common ESP32 platform selection and RP2040
configuration are unchanged. HomeSpan is pinned to 2.1.8. All three
profiles compiled and linked successfully with this platform pin on 2026-10-09.

The Arduino-only integration builds without an IDF project conversion. Exact
local wrapper revisions are recorded in `dependencies.txt`; HomeSpan 2.1.8 is
pinned in the environment. Modified dependencies, including OFM-HomeKey, are
checked out under `.homekey-repos/private` and linked from `lib`.

Home Key upstream dependencies were checked on 2026-10-30:

| Dependency | Selected upstream revision | Status |
| --- | --- | --- |
| HomeSpan | [2.1.8 / 107ffc0](https://github.com/HomeSpan/HomeSpan/releases/tag/2.1.8) | Latest stable release; existing pin retained |
| DigitalDoorKey | [80e4a1e](https://github.com/rednblkx/DigitalDoorKey/commit/80e4a1e14eab3646b62ab3bdad6d2ed01d399375) | Updated Arduino wrapper; crypto shim moved to public includes |
| PN7161 driver (`rednblkx/pn7160`) | [db5cb56](https://github.com/rednblkx/pn7160/commit/db5cb56b2ccfbaa4c30879043a3597bd6c7d53c8) | Current upstream; existing wrapper retained |

The updated DigitalDoorKey wrapper lives in
`.homekey-repos/private/DigitalDoorKey` and is linked through `lib/DigitalDoorKey`.
Its exact local revision is pinned in `dependencies.txt`.
The local OFM-HomeKey implementation retains its PN7161 I2C, Wi-Fi console and
commissioning changes. Other OpenKNX dependencies were not updated.

The Home Key implementation and its ESP32 integration changed these repositories:

| Repository | Changes |
| --- | --- |
| OAM-AccessControl | Build environments, board wiring, flash partitions, ETS integration, startup and regression checks |
| OFM-HomeKey | HomeKit provisioning, authentication, authorization mappings, ETS commissioning, PN7161 I2C transport and Wi-Fi console/help |
| OFM-AccessControl | ESP32 credential storage/peripherals and guarded Home Key/externally owned NFC adapter |
| DigitalDoorKey | Arduino/PlatformIO wrapper and SDK crypto/header integration |
| PN7161 (`rednblkx/pn7160`) | Distinct Arduino/PlatformIO SPI/NCI driver wrapper; the I2C transport is in OFM-HomeKey |
| PN7160 (`Strooom/PN7160`) | ESP32 I2C port for the legacy reader environments, preserving RP2040 behavior |
| OFM-SwitchActuator | Console diagnostic API compatibility fix used by the integration |
| OFM-FileTransferModule | Conditional gzip compiler-warning fix in the earlier Home Key integration; current checkout uses the upstream `ec/v1dev` branch |

HomeSpan is used without source modifications. OGM-Common, OGM-HardwareConfig,
LogicModule, BinaryInput, VirtualButton and ConfigTransfer use their existing
upstream checkouts. `dependencies.txt` records the actual local revisions used
by the verified builds, including the current KNX stack checkout.

### Private Home Key development repositories

The application and modified dependencies are maintained as independent private
repositories under `u20p17`:

| Repository | Purpose |
| --- | --- |
| [OAM-AccessControl](https://github.com/u20p17/OAM-AccessControl) | Application and build configuration |
| [OFM-HomeKey](https://github.com/u20p17/OFM-HomeKey) | Home Key implementation |
| [OFM-AccessControl](https://github.com/u20p17/OFM-AccessControl) | Access-control integration |
| [DigitalDoorKey](https://github.com/u20p17/DigitalDoorKey) | Arduino authentication wrapper |
| [PN7161](https://github.com/u20p17/PN7161) | PN7161 NCI driver wrapper |
| [PN7160](https://github.com/u20p17/PN7160) | Legacy ESP32 I2C reader port |
| [OFM-SwitchActuator](https://github.com/u20p17/OFM-SwitchActuator) | Compatibility changes |
| [OFM-FileTransferModule](https://github.com/u20p17/OFM-FileTransferModule) | Firmware/file-transfer dependency |
| [knx](https://github.com/u20p17/knx) | KNX stack branch used by this application |

Each private repository uses `v1dev_HomeKey` as its development branch. The
local `origin` points to the private repository and is the default push remote.
Where a public source exists, it is retained as `upstream` for fetching, with
its push URL disabled (`no_push://public-upstream-disabled`). Existing shared
OpenKNX checkouts retain their original remotes. GitHub Actions is disabled on
the private repositories; no collaborators were added. `dependencies.txt`
records the private clone URLs and exact revisions of modified dependencies.

The dependency checkouts are linked from `lib`. Do not replace those links with public upstream
clones: the committed local wrappers are required. The isolated build helper
copies all recorded local dependencies and now builds all four targets.
Generated package caches use a project-specific `/tmp` directory because files
in the synced project cache acquired duplicate source copies during builds.

This is the first software prototype. Physical PN7161 operation, Home Key
provisioning/taps on iPhone or Watch, RF timing and concurrent KNX traffic have
not been verified. NVS encryption, secure boot and production commissioning
hardening remain for the next hardware/security stage.

Validation on 2026-10-09: all four environments compiled and linked; the new
ETS header passed producer consistency checks; native Home Key persistence,
revocation, policy and online protocol tests passed. ETS JavaScript tests cover
safe imports, commissioning and the actual Common wrapper with small APDUs.
Live ETS buttons and physical KNX/Home Key operation still require validation.
Home Key firmware occupies about 2.79 MB of its 3 MB OTA slot (88.8%); static
RAM is about 74 KB (22.6%). The SDK size-report helper emits a nonfatal `--ng`
option warning; PlatformIO's firmware size check and image generation succeed.

The Apple Home Key target also compiled and linked with HomeSpan 2.1.8 and
PioArduino 55.03.38-1 on 2026-10-09; no integration code changes were needed.
Pairing/provisioning on physical hardware remains unverified.
