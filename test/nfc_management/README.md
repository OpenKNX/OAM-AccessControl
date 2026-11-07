# Unified NFC / HomeKey management

The application uses `src/NfcManagement.share.xml` and its JavaScript model to
compose the editor under Allgemein → NFC-Scanner. The shared AccessControl module
retains its standalone UID editor for applications without HomeKey.

Validation:

```sh
node test/nfc_management/ets_test.cjs
node test/nfc_management/jscript_test.cjs
python3 test/nfc_management/backend_test.py
python3 test/nfc_management/commissioning_test.py
python3 test/nfc_management/download_test.py
python3 test/nfc_management/xml_test.py
python3 test/nfc_management/producer_test.py /path/to/OpenKNXproducer
```

`xml_test.py` requires a freshly generated `src/AccessControl.debug.xml`.
`producer_test.py` checks ID uniqueness and function-warning suppression with an
older AccessControl editor lacking the new NFC block-name marker.
HomeKey's native store/online tests and ETS script tests additionally cover the
revocation schema, migration, NVS failure, DDK pointer lifetime, identification
expiry/cancellation and deletion acknowledgement. Existing lifecycle/UID tests
cover suppression of actions during identification.

The UID snapshot command is object 160/property 3 command 113. Request:
`113,cursorBE16,revisionBE32`. Start with cursor/revision zero. Reply:
`result,1,revisionBE32,nextCursorBE16,count`, then up to five records of
`idBE16,uid10,name28`. Cursor 1500 is the end; an additional empty-page request
validates the final generation. Changed revision/unexpected cursor returns
result 8. Every UID storage commit advances the revision. All 1500 slots,
including unnamed tags, are covered. Cached records are stored only in ETS,
not in downloaded ESP parameters.

On-device/ETS acceptance:

1. Generate and sign application 11.7 on the ETS machine and flash the matching
   firmware. Upgrade preserves permission memory offsets and existing keys.
2. Import paired devices under Allgemein → NFC-Scanner → Apple HomeKey,
   Vorgang → anlegen, using Geräte einlesen; new
   rows must remain disabled. Identify an iPhone by scanning with commissioning permission enabled and
   programming mode off; no access action should run.
3. Set its name, unused Tag ID and enabled flag. Check the assignment, configure
   Tag ID → action in the NFC action table, then download. Scan to confirm access.
4. Change a Tag ID/action in ETS: the overview must update immediately. A shared
   active ID must show a conflict; several actions for one credential are valid.
5. Select Vorgang → löschen and delete the selected HomeKey device. Confirm keys/permission are removed, its
   ETS slot is empty, other devices still work and action references remain.
   Reboot and try an old ETS download/reprovisioning: this identity stays denied.
6. Identify that device again, explicitly restore with commissioning permission
   enabled and programming mode off, assign a permission and
   download. Verify restoration and the correct action. Check the Wallet entry
   independently; local deletion cannot remove it.
7. Exercise UID learn/create/edit/delete with HomeKey both enabled and disabled.
   A successful operation refreshes the full UID snapshot. Cancel/offline/failing
   reads retain the previous snapshot and mark it stale.

This release writes credential schema version 2 (reads version 1). Downgrading
HomeKey firmware requires a planned migration; older code cannot read version 2.
Persistent revocations are capped at 128 identities and never silently evicted.

The UID and HomeKey editors share the existing `ACC_NfcEdit` dropdown. HomeKey
shows scanning/restoration for anlernen, import/manual device IDs for anlegen,
permission fields for ändern, and read-only confirmation for löschen. Short
button labels keep commissioning instructions in the relevant German ETS help.
Every visible field and operation, including the overview cells, has a help
context. Only blocked configurations and the bulk-UID deletion warning remain
as inline notices. `xml_test.py` verifies help coverage and topic completeness.

ETS operations work with programming mode off. Scanning and explicit restoration
require NFC commissioning permission; device deletion remains available with
commissioning disabled. `commissioning_test.py` exercises the actual handlers
and credential persistence for those rules. Web setup-code changes continue
to require programming mode.

ETS runs these parameter scripts with Windows Active Scripting JScript. Keep
`src/NfcManagement.script.js` and its HomeKey helpers compatible with ES3;
use plain loops instead of modern array methods. `jscript_test.cjs` executes
the actual assignment check and overview in an isolated realm with ES5+ APIs
removed, including `Array.map` and `Array.indexOf`. It covers action-list
formatting/deduplication, UID collisions, blocked credentials and connection
cleanup. String `indexOf` remains available.

Parameter scripts are embedded in the ETS product, not the ESP firmware.
After changing them, regenerate the knxprod and update the application's scripts
in the ETS project using that product. Flashing the ESP alone cannot replace
the script already stored in the ETS project.

All stored HomeKey policy flags and all 16 permission slots use
`LegacyPatchAlways="true"`. The selected operation, slot and access type are
editor state; they must not decide which permissions reach the device. In
particular, issuer/endpoint controls appear only in `anlegen`, but their values
must also download from `anlernen`, `ändern`, `löschen`, or another selected slot.
`download_test.py` checks coverage and unchanged memory offsets; pass a generated
debug XML path to check that the producer preserves the attributes as well.
After updating the ETS product, download from each operation with two enabled
devices in different slots and verify both still authenticate. Also check that
disabling one slot and downloading from another slot revokes the disabled slot.
The actual ETS download round trip requires hardware testing.

Deleting an incomplete HomeKey slot (missing or malformed issuer/endpoint)
clears and disables only that ETS draft. It sends no reader revocation command
and explicitly requests a parameter download to remove the device-side
permission. Complete identities retain acknowledged persistent revocation before
ETS fields are cleared. Tests cover incomplete slots alongside another device,
cancellation, failed/successful revocation, and execution under ES3 restrictions.

HomeKey identification stages a scan result until the reader has acknowledged
session completion and the online connection has closed. No permission fields
are changed while network operations are pending. The final ETS writes restore
their previous values if a setter, cancellation or progress callback fails;
existing device names, active rights and Tag IDs are preserved. Tests inject
polling/finish/disconnect failures, refused replies, cancelled scans and partial
parameter-write failures. Edits made while waiting are detected before commit.

The bulk-deletion control explicitly says Alle UID-Tags löschen? and appears
only for UID-Tag. It clears UID records, not HomeKey keys, permission slots or
revocations. Individual HomeKey revocation uses Gerät löschen. Each operation
button has a help topic describing its effect and the next commissioning step.
