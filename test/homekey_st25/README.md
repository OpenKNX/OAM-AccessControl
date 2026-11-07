# ST25R3916 integration checks

Run from the application root:

```sh
python3 lib/ST25R3916/test/run.py
c++ -std=c++17 -fsanitize=address,undefined -DARDUINO_ARCH_ESP32 \
  -Ilib/ST25R3916/test/stubs -Ilib/ST25R3916/test/support \
  -Ilib/ST25R3916/src -Ilib/OFM-AccessControl/src \
  test/homekey_st25/adapter_test.cpp lib/ST25R3916/src/ST25R3916.cpp \
  -o /tmp/oam-st25-adapter-test
/tmp/oam-st25-adapter-test
python3 test/homekey_st25/service_test.py
python3 test/homekey_st25/lifecycle_test.py
python3 test/homekey_st25/uid_mode_test.py
python3 test/homekey_st25/ets_visibility_test.py
```

The standalone library owns chip/protocol tests and their device stubs.
The application checks the actual HomeKey adapter's optional ECP transitions
and the actual module loop/event dispatch with standalone UID operation,
HomeKey startup gating and authorization after policy changes.
Physical Elechouse reader and phone tests remain required.

ETS visibility checks cover HomeKey/raw-data combinations, default UID-only operation, mixed/HomeKey-only policy,
physical lock feedback, and actual firmware diagnostic input/output gating.

The service test exercises the actual worker and main-thread queue with no
HomeKey extension, optional authentication consumption and stale epochs/tokens.
The HomeKey tests cover only its optional hooks and HAP lifecycle. Native
transport checks compile `lib/PN7161/src/i2c.cpp` without Arduino or the
HomeKey feature macro (see `lib/PN7161/docs/native-i2c.md`). UID-only firmware builds provide an additional check
that OFM-HomeKey, HomeSpan and DigitalDoorKey are excluded.
