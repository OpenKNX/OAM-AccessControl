# NFC overview: device readback and ETS drafts

The overview defaults to **Datenquelle → Gerätestand**. **Zugänge einlesen**
reads UID tags, downloaded HomeKey permissions/descriptions, and downloaded NFC
action assignments. All parts are staged and generation-checked before the ETS
cache is replaced. Failed or cancelled refreshes retain the previous complete
snapshot; failed cache rollback marks the snapshot invalid.

A name without a complete issuer/endpoint identity is an ETS draft, not another
device. Such placeholders are excluded from the device view. **ETS-Entwurf**
remains available for editing and checking pending changes. Editing a free slot,
description, Tag-ID or action does not change device-view values. The snapshot
is not a live connection: read again after programming the device.

## HomeKey downloads independent of editor visibility (11.8)

All 84 stored HomeKey policy/permission fields are referenced by an unconditional
`AuthorizationDownload` parameter block with `Access="None"`. The block reuses
the original ParameterRefs, so the editor, scripts and download encode the same
values. It stays outside the NFC scanner selector; the existing memory layout
and `LegacyPatchAlways` attributes are retained.

This matters for XKNX, which builds the download image from active references.
Previously, selecting **anlernen** hid the issuer/endpoint references and left
their bytes empty in the download, even though the ETS draft displayed the IDs.
**Zugänge einlesen** correctly excluded that incomplete device row. Showing the
fields with **anlegen** made the same image include the missing 28 ID bytes.

Hardware validation on the ESP32-S3 IP controller (2026-11-09): application-only
USB firmware flash, XKNX product update preserving parameters/group links, full
11.8 download while **anlernen** was selected, and successful device readback of
both the active Home Key (Tag-ID 0) and the UID tag (Tag-ID 1) without
conflicts. Parameter previews with another HomeKey slot selected and with UID
mode selected both reported zero differences. No key or UID storage was erased.

## Updating

This change needs **both** the updated firmware and a regenerated ETS product.
The added read-only APIs are absent from older firmware. All existing downloaded
HomeKey/NFC parameter offsets and credential storage remain unchanged; the new
source selector and caches exist only inside ETS. No credential reset is needed.

For the user's XIAO ESP32-S3 IP board:

```sh
pio run -e debug_ESP32S3_AppleHomeKey_IP
```

Application image: `.pio/build/debug_ESP32S3_AppleHomeKey_IP/firmware.bin`.
Generate/sign `src/AccessControl.xml` with the usual ETS product build process,
update the project to that product, and download the parameters.

## Acceptance checks

1. With one learned/programmed phone and another slot containing only a name,
   press **Zugänge einlesen**. The default device view must show one HomeKey
   entry, with its stored slot number, description and downloaded Tag-ID.
2. Change a free slot's description or rename the phone in ETS. Device view
   remains unchanged. Switch to **ETS-Entwurf** to see the pending changes and
   incomplete drafts. Download, then read again to confirm stored changes.
3. Change an action assignment without downloading. Device view must retain the
   downloaded action until programming and a new readback complete.
4. Interrupt a refresh. Existing snapshots must remain intact, with a failure
   message. Readback must not enable, delete or revoke any credential.
5. Test UID-only firmware: action readback reports that HomeKey is not compiled;
   the refresh must still read UID records without querying that module.

Validation completed: REG1 IP and XIAO ESP32-S3 IP firmware builds; native
read-only snapshot framing, bounds and stale-generation tests; the full
1500-record UID snapshot and 310-action frontend tests; screenshot/placeholder
regression; failed refresh/rollback cases; ES3/JScript checks; producer-expanded
UI, German help coverage and unchanged downloaded parameter offsets.
On-device validation of the new readback workflow remains pending.

The S3 image was built in an isolated PlatformIO core because the shared SDK
package versions were changing during compilation. The isolated core used the
pinned Arduino-ESP32 3.3.8 / ESP-IDF 5.5.4 packages and existing verified tool
archives; no application networking settings were changed for that build.
