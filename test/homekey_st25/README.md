# ST25R3916 integration checks

Run from the application root:

```sh
python3 lib/ST25R3916/test/run.py
c++ -std=c++17 -fsanitize=address,undefined -DARDUINO_ARCH_ESP32 \
  -Ilib/ST25R3916/test/stubs -Ilib/ST25R3916/test/support \
  -Ilib/ST25R3916/src -Ilib/OFM-HomeKey/src \
  test/homekey_st25/adapter_test.cpp lib/ST25R3916/src/ST25R3916.cpp \
  -o /tmp/oam-st25-adapter-test
/tmp/oam-st25-adapter-test
python3 test/homekey_st25/lifecycle_test.py
```

The standalone library owns chip/protocol tests and their device stubs.
The application checks the actual HomeKey adapter's optional ECP transitions
and the actual module loop/event dispatch with standalone UID operation,
HomeKey startup gating and authorization after policy changes.
Physical Elechouse reader and phone tests remain required.
