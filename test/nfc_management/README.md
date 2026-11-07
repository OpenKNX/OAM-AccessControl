# Unified NFC / HomeKey management

The application uses `src/NfcManagement.share.xml` and its JavaScript model to
compose the editor under Allgemein → NFC-Scanner. The shared AccessControl module
retains its standalone UID editor for applications without HomeKey.

Validation:

```sh
node test/nfc_management/ets_test.cjs
python3 test/nfc_management/backend_test.py
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
2. Import paired devices under Allgemein → NFC-Scanner → Apple HomeKey; new
   rows must remain disabled. Identify an iPhone by scanning with programming
   mode and commissioning permission enabled; no access action should run.
3. Set its name, unused Tag ID and enabled flag. Check the assignment, configure
   Tag ID → action in the NFC action table, then download. Scan to confirm access.
4. Change a Tag ID/action in ETS: the overview must update immediately. A shared
   active ID must show a conflict; several actions for one credential are valid.
5. Delete the selected HomeKey device. Confirm keys/permission are removed, its
   ETS slot is empty, other devices still work and action references remain.
   Reboot and try an old ETS download/reprovisioning: this identity stays denied.
6. Identify that device again, explicitly restore, assign a permission and
   download. Verify restoration and the correct action. Check the Wallet entry
   independently; local deletion cannot remove it.
7. Exercise UID learn/create/edit/delete with HomeKey both enabled and disabled.
   A successful operation refreshes the full UID snapshot. Cancel/offline/failing
   reads retain the previous snapshot and mark it stale.

This release writes credential schema version 2 (reads version 1). Downgrading
HomeKey firmware requires a planned migration; older code cannot read version 2.
Persistent revocations are capped at 128 identities and never silently evicted.
