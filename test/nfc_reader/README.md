# PN7160 shared-service regression checks

From the application root:

```sh
python3 lib/PN7160/test/esp32/run.py
python3 test/nfc_reader/run.py
python3 test/nfc_reader/cooperative_test.py
python3 test/homekey_st25/service_test.py
```

The adapter tests compile the actual PN7160 facade, common adapter, transport
and NCI engine for both ESP32 and RP2040 branches under ASan/UBSan. They cover
initialization, asynchronous readiness, 4/7/10-byte UIDs, repeated/replacement
tags, removal, invalid pins and reset recovery. Unsupported APDU/ECP operations
fail explicitly. The cooperative service test compiles without FreeRTOS and
checks nonblocking execution, diagnostic isolation, retry timing and stale
results. The existing shared-worker suite covers the ESP32 execution path.
Hardware and full RP2040 firmware validation remain separate checks.
