# External dependency audit and upstream PR plan

Audit date: 2026-11-07. Scope: non-OpenKNX dependencies used by OAM-AccessControl,
with emphasis on the NFC/HomeKey changes. This is a source/history audit and PR
plan; no upstream PRs, publication, pushes or implementation changes were made.
Inactive duplicate library directories ending in ` 2` or ` 3` are excluded.
OpenKNX modules, `knx` and OpenKNX `TPUart` are outside this audit's PR scope.

Follow-up implementation on 2026-11-07: PN7161's stale porting notes were
corrected. DigitalDoorKey now advertises both frameworks, selects the Arduino
protocol explicitly (Aliro by default), preserves native/hybrid ESP-IDF control,
uses actual platform definitions instead of forcing `CONFIG_IDF_CMAKE`, and
limits vendored CBOR headers to Arduino. OpenKNX explicitly selects HomeKey and
its existing library-local logging level. The findings below describe the
original audited revisions; these two preparation items have now been addressed.
The REG1 IP firmware and six packaging checks passed. The host suite built;
12/13 test executables passed, with the same two BLE-flow assertions also failing
on the prior DigitalDoorKey revision. A full native ESP-IDF build remains pending.

## Inventory and provenance

| Local dependency | Actual upstream / comparison base | Local revision | Findings |
| --- | --- | --- | --- |
| DigitalDoorKey | [rednblkx/DigitalDoorKey](https://github.com/rednblkx/DigitalDoorKey/tree/80e4a1e14eab3646b62ab3bdad6d2ed01d399375), `main` at `80e4a1e` | `c74c30c`, version 1.0.1 | Two local commits; Arduino packaging and TLV symbol isolation. 24 files, +1565/-48; most additions are vendored headers. |
| PN7160 | [Strooom/PN7160](https://github.com/Strooom/PN7160/tree/caf9d22604bea176f0effce685b2f384218da111), `RP2040` at `caf9d22` | `1358ad7`, version 1.2.1 | Four local commits; ESP32 transport, cooperative reader facade, activation-parser validation, version bump. 13 files, +555/-6 against the correct branch. |
| PN7161 | [rednblkx/pn7160](https://github.com/rednblkx/pn7160/tree/db5cb56b2ccfbaa4c30879043a3597bd6c7d53c8), `main` at `db5cb56` | `1a1492f`, version 1.1.1 | Five local commits; seven added files, +807 lines. Existing upstream SPI/NCI implementation is unchanged. Adds packaging, I2C transport and a reader facade adapted from HomeKey-ESP32. |
| ST25R3916 | Reader extracted from [rednblkx/HomeKey-ESP32](https://github.com/rednblkx/HomeKey-ESP32/tree/f167cba1950aa9e37afb9cb06163a78299599a67), `f167cba` | `b6f3634`, version 0.1.1 | New standalone repository, not a Git fork with shared ancestry. Three commits: extraction/adaptation, latched transport failure, version bump. |
| HomeSpan | [HomeSpan/HomeSpan](https://github.com/HomeSpan/HomeSpan/tree/107ffc07f4455754ea89068d8cf2e992de3583e6), `107ffc07` | Same pinned revision | Downloaded Git checkout is clean; no HomeKey-specific library patch. No PR needed. |

The four modified libraries are in private `u20p17` repositories under
`..`, linked into this application's
`lib` directory. Their tracked working trees were clean during the audit.

Public GitHub heads were checked during this audit. DigitalDoorKey, rednblkx's
PN7160, HomeSpan and the HomeKey-ESP32 donor match the bases above. Strooom's
default `main` is `ac06362`, but its **RP2040 branch is `caf9d22`**, exactly our
pre-change base. [PR #10](https://github.com/Strooom/PN7160/pull/10) was merged
into `RP2040`, not `main`. Its inherited RP2040/build/logging changes must not be
presented as our new work. A comparison only against `main` overstates our diff.

Other installed external dependencies checked through package metadata and Git
status where available: ANSI 0.2.1, FastCRC 1.41, lz4 1.10.0, TimerInterrupt_Generic
1.13.0, BL0942 1.0.1, BS811X_I2C 1.0.0, NXP PCA9633 1.1.1 and RTTStream 1.4.0.
No local modifications attributable to this HomeKey work were identified there.
Some are inherited `ab-tools` forks rather than original upstream packages:
lz4 (`bdb19a2`), BL0942 (`31546b6`) and BS811X_I2C (`29e2e1a`). Those inherited
fork differences are not new HomeKey patches. Presence in the dependency cache
does not mean every library is linked into the selected firmware.

TinyCBOR 0.6.0 headers were vendored into DigitalDoorKey from Intel TinyCBOR
revision `d393c16f3eb30d0c47e6f9d92db62272f0ec4dc7`; this is a packaging change,
not a TinyCBOR algorithm fix. The ESP32 core supplies mbedTLS/libsodium; no local
cryptographic algorithm changes were identified. No PR is proposed to those
projects. Registry packages without Git history were inspected as installed
packages; they were not independently reconstructed from fresh registry archives.

## What we changed and what should go upstream

### DigitalDoorKey

Commits: `6489a51` (Arduino wrapper), `c74c30c` (TLV isolation).

* Renamed its global `TLV8` implementation to `DdkTlv8`, updating both HomeKey
  and Aliro references. HomeSpan also defines a global `TLV8`; overlapping C++
  symbols can bind methods to an incompatible class layout. This belongs in
  DigitalDoorKey, even though the crash appears during HomeSpan provisioning.
  The patch preserves TLV bytes and adds coexistence/round-trip regression tests.
* Added PlatformIO packaging, private include paths, HomeKey-only source filters
  and Arduino build definitions. TinyCBOR headers and four include rewrites
  bridge the Arduino dependency layout. This is separate from the TLV bug fix.
* Before submitting the packaging change, preserve upstream's ESP-IDF/host and
  Aliro support. Our manifest currently advertises Arduino-only support, forces
  HomeKey mode, defines `CONFIG_IDF_CMAKE`, and forces logging off. Make protocol
  selection/logging configurable and avoid pretending an Arduino build is CMake.
* The unconditional TinyCBOR include rewrites can also affect IDF/host builds.
  An unchanged CMake file does not prove those configurations are unaffected.
  Verify header/library compatibility, or scope the workaround to Arduino.
* Discuss a proper namespace with the maintainer. Do not introduce a global
  `TLV8` compatibility alias that recreates the collision. Update stale build
  versions/claims in `PORTING.md` before including any of that document upstream.

### Strooom PN7160

Commits: `0c3dfe5` (ESP32 I2C), `0cd4c15` (facade), `da135cf` (parser),
`1358ad7` (version).

* Added an ESP32 `TwoWire` transport: explicit pins/address, 258-byte buffer,
  bounded timeouts, complete transfer checks, error handling and shared-bus
  ownership. Other platform branches remain present.
* Added cooperative `PN7160Reader` initialization/discovery/polling, presence,
  health and stop operations. It uses the existing static NCI engine, so it
  still supports one reader instance per MCU. This is UID functionality;
  the facade does not add APDU, HomeKey or ECP support.
* Hardened activation parsing against truncated packets, invalid technology
  offsets and invalid UID lengths. Failure cannot leave a previous UID exposed
  as a newly read credential. The test feeds malformed packets through the real
  NCI engine and transport, including every truncated prefix.
* Target `RP2040` for the additive port/facade, or agree a consolidation branch
  with the maintainer. A parser fix can be adapted to `main` separately if wanted;
  do not drag the entire inherited RP2040 branch into a bug-fix PR.
* Preserve its existing CC BY-NC-SA 4.0 license and attribution. This dependency
  is not MIT like the rednblkx drivers.

### rednblkx PN7160 / our PN7161 package

Commits: `4543486`, `683a72c`, `cbf1039`, `1b877d7`, `1a1492f`.

* Added an Arduino I2C transport implementing the upstream transport interface.
  It borrows the initialized bus, owns only its device handle, handles IRQ/VEN/DWL,
  validates transfer sizes, retries address-NACK conditions within bounds, and
  leaves timeouts/non-retryable failures visible. Native IDF I2C operations avoid
  Arduino logging an error for every recoverable standby NACK.
* Added a reusable reader facade for SPI or I2C, discovery, UID/APDU, configurable
  VANT and optional copied ECP data. ECP is disabled by default; the controller
  supplies the CRC for its 16-byte NCI ECP payload. No OpenKNX/HomeSpan dependency.
* Removed application-style UID cooldown from the extracted facade so the caller
  owns presentation suppression. NCI polling failures now mark the reader
  disconnected; a failed presence-probe write ends cached presence immediately.
* The facade came from HomeKey-ESP32's `main/Pn7160Reader.cpp`, not from the
  upstream driver repository itself. Consequently, the small connection/presence
  fixes can also be ported directly back to HomeKey-ESP32 without asking it to
  adopt our new facade or packaging.
* `PORTING.md` still starts with obsolete SPI/XIAO/OFM-HomeKey ownership notes.
  Correct those before submission. Keep upstream naming compatible: the private
  package name `PN7161` avoids our local library collision, but is not a reason to
  rename upstream's existing PN7160 APIs. Retain donor MIT attribution.

### ST25R3916 / HomeKey-ESP32 donor

The source was compared directly with the donor's `main/St25r3916Reader.cpp`.
Changes extend beyond extracting it into a library:

* Shared `TwoWire` bus ownership replaces creation/deletion of a private bus.
* Optional, caller-owned/copied ECP configuration replaces mandatory application
  data. Driver headers no longer depend on the HomeKey application.
* Transport failures latch disconnection; initialization and RF configuration
  check failures instead of proceeding with a partly configured reader.
* Oversized FIFO replies and malformed ATQA/ATS are rejected, including absent
  optional ATS fields. Failed APDU exchanges clear partial response data.
* ISO-DEP frame-wait calculation was corrected, with bounded total exchange
  time, response size, WTX multiplier and retry limits. These are distinct from
  the Arduino packaging changes and are candidates for donor bug-fix PRs.
* Polling yields to other tasks. The library targets Elechouse ST25R3916 over
  I2C; it does not add SPI, ST25R3916B verification or NFC-B/F/V.

The standalone repository has no shared Git ancestry with HomeKey-ESP32.
Do not propose merging that whole repository into the donor. Port generic fixes
to its existing reader first; offer standalone-library adoption separately.
[HomeKey-ESP32 PR #308](https://github.com/rednblkx/HomeKey-ESP32/pull/308) is an
open migration away from Arduino/HomeSpan, so keep those fixes usable without
the Arduino bus adapter and coordinate any later integration with that work.

## Proposed PR sequence

Each numbered item is a separate reviewable PR, with tests included in its own
repository. The local commits are starting points, not always clean cherry-picks.

| Order | Target | Proposed scope | Preparation / dependency |
| --- | --- | --- | --- |
| 1 | DigitalDoorKey | **Avoid TLV8 symbol collisions with HomeSpan** | Extract `c74c30c`; keep packaging out. Add a two-translation-unit/link regression alongside the current byte/coexistence test; verify HomeKey and Aliro. |
| 2 | Strooom/PN7160 | **Reject malformed NFC-A activation notifications** | Extract `da135cf` and its NCI regression tests; decouple test support from the ESP32 feature commit. Agree `RP2040` versus an adapted `main` fix. |
| 3 | HomeKey-ESP32 | **Invalidate PN7160 connection/presence after NCI failures** | Port `1b877d7`/`1a1492f` behavior into the existing donor reader. Test idle timeout versus real connection failure; preserve the donor's presentation policy. |
| 4 | HomeKey-ESP32 | **Validate ST25R3916 replies and bound ISO-DEP exchanges** | Port protocol/error fixes without the Arduino extraction. Include timing vectors, malformed ATS/FIFO replies, WTX/deadline and fault tests. Split timing from parser/fault handling if the maintainer prefers smaller reviews. |
| 5 | Strooom/PN7160 | **Support ESP32 over shared Arduino I2C** | Extract `0c3dfe5`; include transport tests and example. Verify an actual ESP32 and existing RP2040 hardware. |
| 6 | Strooom/PN7160 | **Expose a cooperative UID reader facade** | Extract `0cd4c15`; retain old API. Depends on transport work for the ESP32 example; document single-instance and UID-only limits. |
| 7 | rednblkx/pn7160 | **Add Arduino ESP32 I2C transport** | Transport and minimal packaging from `4543486`/`683a72c`, with standalone tests. Preserve existing SPI/ESP-IDF build and public names. |
| 8 | rednblkx/pn7160 | **Add a reusable SPI/I2C reader facade** | Remaining `683a72c` plus `cbf1039`/`1b877d7`/`1a1492f`; depends on #7. Agree whether the maintainer wants this higher-level API in the driver repo. Keep ECP optional and suppression caller-owned. |
| 9 | DigitalDoorKey | **Support Arduino ESP32/PlatformIO builds** | Rework `6489a51` as configurable packaging. Preserve IDF, host and Aliro; document TinyCBOR provenance/compatibility. Independent of the TLV fix in principle. |

After hardware validation, prepare a standalone ST25R3916 release and an optional
HomeKey-ESP32 integration proposal. That is a separate architectural decision,
not a prerequisite for sending back its generic bug fixes.

## Validation performed and remaining work

Passed during this audit:

* DigitalDoorKey TLV coexistence and round trips, compiled with AddressSanitizer
  and UndefinedBehaviorSanitizer.
* PN7160 transport and malformed activation tests using the actual engine.
* Actual PN7160 adapter/NCI tests for simulated ESP32 and RP2040 branches.
* Standalone ST25R3916 driver tests with sanitizers: UID, ECP, ISO-DEP chaining,
  CRC/WTX/bounds and shared-bus lifecycle.
* PN7161 presence-method regression with sanitizers. This extracts the real
  method into a harness; it is not a full driver build or hardware test.
* Actual PN7161 I2C transport with sanitizers under simulated IDF 5.3, 5.4, 5.5
  and 6.0 version branches. These are stubbed API tests, not real SDK builds.

Before submitting: move application-owned tests into the relevant library PRs;
run each upstream project's own builds/tests; add real IDF/SPI build coverage
for the rednblkx driver and full IDF/host/Aliro coverage for DigitalDoorKey.
For firmware integration, use only `debug_REG1_ESP_PN7161_IP` per `AGENTS.md`.
No new full firmware build was required or run for this source-only audit.

Hardware checks still matter: cold boot/recovery and shared I2C peripherals;
4/7/10-byte UID presentation/removal; iPhone/Watch pairing/authentication and
Express Mode; ECP toggling; NACK/disconnect recovery. Physical Elechouse/iPhone
validation remains explicitly pending in the ST25R3916 README. Simulated tests
do not establish RF compatibility or product readiness.

## Preparing review branches while retaining privacy

Create fresh per-PR branches from the intended current upstream branch. Port only
the relevant hunks, tests and attribution; omit local version-only churn,
OpenKNX-specific documentation and stale porting history. Recheck upstream and
open PRs at submission time, particularly the HomeKey-ESP32 refactor.

Keep the existing private integration branches intact. Public PRs expose their
patches, and unrelated private repository copies are not ordinary public
cross-fork PR sources. Publication would therefore require approved public
review branches/forks or another maintainer-supported submission route. The
current audit does not authorize that publication; all implementation stays in
the private repositories for now.
