# ST25R3916/B timing experiment

Build `debug_ESP32S3_ST25R3916_IP_TIMING_TEST` for the XIAO ESP32-S3 IP controller.
This uses the same application/parameter layout and saved credentials as the
normal ST25 firmware. All ST25 builds default to fast polling; only this profile adds timing logs.

The firmware starts in fast mode. With the antenna empty, use the local USB
console command `hk timing baseline` or `hk timing fast`. Fast mode sends ECP
without waiting for a response and caps the WUPA response wait at 10 ms. It keeps
the 5 ms settling delay, 20 ms worker pause, RATS, APDU, FWT and WTX behavior.
Both commands are available in regular ST25 HomeKey builds as well.
Switching mode does not change ETS, keys or permissions and is not persistent;
reboot restores fast mode.

For each mode, present the same paired iPhone three times and the same authorized
UID card three times. Remove the credential for at least three seconds between
presentations. Check successful access, absence of duplicate grants while held,
and reliable removal/re-presentation.

The USB log records:

- Idle `average_probe_us`: ECP, settling and WUPA for unsuccessful probes (five
  second summaries; excludes the worker pause).
- Successful `ecp_us`, `wake_us` (includes settling), `activation_us` (UID
  anticollision/select and optional RATS), and `total_poll_us`.
- `handler_us`: HomeKey SELECT/authentication or UID classification/enqueue.
  It excludes the later KNX-thread permission/action execution.

These are internal durations, not measured time from a person's physical tap to
an action. A phone can arrive between probes or require several ECP polls.
Compare the two phases with equivalent credentials and positioning. No timing
record is emitted in the middle of authentication APDU exchanges.

Standalone driver tests run both the ordinary build and the test build with
fast mode enabled. They exercise UID selection, ECP, APDUs, recovery, B-chip
calibration, health checks, and the no-response timing bounds. The bus model's
microsecond numbers are simulated; hardware figures must come from USB logs.

## Physical trial — 9 November 2026

Elechouse ST25R3916B Mini, XIAO ESP32-S3, 400 kHz I2C. Baseline and
fast modes each accepted all three iPhone and three authorized UID-card taps.
There were exactly six HomeKey and six UID grants across both phases.

| Measurement | Baseline | Fast |
| --- | --- | --- |
| Empty probe (excluding worker pause) | approximately 271–276 ms | approximately 19 ms |
| ECP stage, six detections per mode | 15.10 ms mean | 3.35 ms mean |
| iPhone successful discovery/activation | 31.22 ms mean | 19.28 ms mean |
| UID-card successful discovery/activation | 37.11 ms mean | 25.90 ms mean |
| HomeKey credential handler | 236.8–240.0 ms | 240.8–244.6 ms |

The 20 ms worker pause remains additional. HomeKey authentication time did
not improve in this small sample; the main reduction is the time spent between
activation opportunities. These are internal measurements, not tap-to-unlock
measurements. Longer stability and varied phone placement remain to be tested.
The original trial firmware started in baseline mode. Following the successful
trial, fast became the default for all ST25 builds; baseline remains selectable.
