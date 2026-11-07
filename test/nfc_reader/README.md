# Shared NFC reader regression checks

The shared service owns presentation suppression, recovery and UID actions.
PN7161 and ST25R3916 use the same ISO-DEP adapter, with PN7161 additionally
applying live ECP changes through NCI. PN7160 retains its cooperative UID engine.

OFM-AccessControl owns the PN7161 reader facade and Wire-to-native bus bridge.
PN7161 I2C always uses the native transport; no Wire transaction fallback remains.
The separate PN7161 library contains only the native
ESP-IDF NCI engine and SPI/I2C transports. Its native I2C tests remain in
`lib/PN7161/test/native_i2c`.

`capabilities()` reports UID, ISO-DEP and ECP support. ETS HomeKey mode requires
an attached authentication extension and ISO-DEP/ECP support. `pollTag()` returns
`NoTag`, `TagFound` or `ReaderError`; transport faults cannot become UID grants.
The boolean polling API remains available for existing integrations.

A held UID can trigger once. Repeated activations refresh its presence and are
suppressed. After one second of polling without that UID, it can trigger again.
A different UID can trigger immediately. ISO-DEP presence monitoring no longer
forces a release after ten seconds. The PN7160 engine also retains its native
repeat suppression and cooperative execution.

The UID buffer is reserved once for ten bytes. Active health probes run after
initialization and at ten-second intervals, including during presence monitoring.
Connection/transport failures bypass that interval and recover with a five-second
backoff. The 8 KB NFC task stack is retained: use `hk memory` stack headroom on
hardware before reducing it.

Run from the application root:

```sh
python3 test/nfc_reader/run.py
python3 test/nfc_reader/cooperative_test.py
python3 test/nfc_reader/ecp_adapter_test.py
python3 test/nfc_reader/pn7161_presence_test.py
python3 test/nfc_reader/st25_adapter_test.py
python3 test/homekey_st25/service_test.py
python3 test/homekey_st25/uid_mode_test.py
python3 test/homekey_st25/lifecycle_test.py
python3 lib/PN7160/test/esp32/run.py
python3 lib/ST25R3916/test/run.py
c++ -std=c++17 -fsanitize=address,undefined -Ilib/OFM-AccessControl/src \
  test/nfc_reader/presentation_test.cpp -o /tmp/nfc-presentation-test
/tmp/nfc-presentation-test
```

Tests exercise real service/adapter/driver code with simulated buses and RF replies.
The PN7161 adapter test uses a driver spy to check initial staging versus live NCI
apply calls, including failed updates. The PN7160 parser tests feed truncated NFC-A
activation packets through its real transport and NCI state machine.

Physical PN7160/PN7161/Elechouse ST25R3916 tests remain necessary for RF timing,
ordinary tags held on the antenna, iPhone/Watch authentication and Express Mode.
Use `debug_REG1_ESP_PN7161_IP` for the routine firmware build.
