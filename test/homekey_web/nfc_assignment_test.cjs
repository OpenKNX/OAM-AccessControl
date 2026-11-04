const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const root = path.resolve(__dirname, '../..');
let count = 0;
function test(name, run) { run(); count++; console.log('PASS: ' + name); }

function fixture(homekey = true) {
    const parameters = {};
    const set = (name, value) => parameters[name] = { value };
    set('NFCACT_Fa1ActionId', 1); set('NFCACT_Fa1NfcId', 100);
    set('NFCACT_Fa1NfcActionInfo', 'Old successful result');
    set('ACC_VisibleActions', 2); set('ACC_Act1Description', 'Haustür');
    if (homekey) {
        set('HK_Enabled', 1); set('HK_EtsAuthorization', 1);
        for (let row = 1; row <= 16; row++) {
            const prefix = 'HK_Authorization' + String(row).padStart(2, '0');
            for (const [suffix, value] of Object.entries({ Enabled: 0, AccessId: 0,
                Issuer: '', Endpoint: '', Description: '' })) set(prefix + suffix, value);
        }
        for (const [suffix, value] of Object.entries({ Enabled: 1, AccessId: 100,
            Issuer: '001122334455aabb', Endpoint: '0123456789ab', Description: 'Julias iPhone' }))
            set('HK_Authorization01' + suffix, value);
    }
    const f = { parameters, calls: [], connects: 0, disconnects: 0, uid: null,
        provisioned: true, failure: null, texts: [] };
    f.device = { getParameterByName: name => parameters[name] };
    f.online = { connect() { f.connects++; }, disconnect() { f.disconnects++; } };
    f.progress = { setText(text) { f.texts.push(text); }, setProgress() {}, isCanceled() { return false; } };
    const context = vm.createContext({ BASE_invokeFunctionPropertyWrapper(object, property, data) {
        f.calls.push({ object, property, data: Array.from(data) });
        if (f.failure) throw new Error(f.failure);
        if (object === 160) {
            assert.deepEqual(Array.from(data), [111, 0, 100]);
            return f.uid === null ? [1] : [0, ...Array(10).fill(1), ...Buffer.from(f.uid), 0];
        }
        assert.equal(object, 165); assert.equal(property, 3);
        if (f.homeKeyFailure) throw new Error(f.homeKeyFailure);
        if (data[0] === 0) return [0, 1, 1, 1, 0, 2, 2, f.provisioned ? 1 : 0, 0, 0, 0, 1];
        assert.deepEqual(Array.from(data), [3, 0, 0, 0, 0, 1]);
        return [0, 1, 0, 1, 0, 0, 0, 1, 0, 17, 34, 51, 68, 85, 170, 187, 1, 35, 69, 103, 137, 171];
    } });
    for (const file of ['OFM-AccessControl/src/AccessControl.script.js',
        ...(homekey ? ['OFM-HomeKey/src/HomeKey.script.js'] : [])])
        vm.runInContext(fs.readFileSync(path.join(root, 'lib', file), 'utf8'), context);
    f.check = (channel = 1) => context.ACC_checkNfcAction(f.device, f.online, f.progress, { Channel: channel });
    f.info = () => parameters.NFCACT_Fa1NfcActionInfo.value;
    f.validationInput = () => {
        const input = { HomeKeyEnabled: parameters.HK_Enabled.value, EtsAuthorization: parameters.HK_EtsAuthorization.value };
        for (let row = 1; row <= 16; row++) for (const suffix of ['Enabled', 'AccessId', 'Issuer', 'Endpoint']) {
            const number = String(row).padStart(2, '0');
            input['Row' + number + suffix] = parameters['HK_Authorization' + number + suffix].value;
        }
        return input;
    };
    f.validate = input => context.HK_validateAssignments(input);
    return f;
}

test('Home Key description and provisioning are resolved without changing permissions', () => {
    const f = fixture(), before = JSON.stringify(f.parameters);
    f.check();
    assert.match(f.info(), /Provisioniert; ETS: Home Key Julias iPhone/);
    assert.match(f.texts.at(-1), /Zuordnung nach ETS-Download wirksam; Aktion 1: Haustür/);
    const after = JSON.parse(before); after.NFCACT_Fa1NfcActionInfo.value = f.info();
    assert.deepEqual(f.parameters, after);
    assert.equal(f.connects, 1); assert.equal(f.disconnects, 1);
    assert(f.calls.every(c => c.object === 160 || [0, 3].includes(c.data[0])));
});
test('An unprovisioned Home Key is clearly marked', () => {
    const f = fixture(); f.provisioned = false; f.check();
    assert.match(f.info(), /^Nicht provisioniert/);
});
test('An occupied UID slot is a visible conflict, not a successful Home Key lookup', () => {
    const f = fixture(); f.uid = 'Normal tag'; f.check();
    assert.match(f.info(), /^KONFLIKT: UID-Tag und Home Key nutzen ID 100/);
});
test('Duplicate active IDs and identities identify conflicting permission rows', () => {
    const f = fixture(), input = f.validationInput();
    assert.equal(f.validate(input), true);
    input.Row02Enabled = 1; input.Row02AccessId = 100;
    assert.match(f.validate(input), /Berechtigung 1 und 2 doppelt/);
    input.Row02AccessId = 101;
    input.Row02Issuer = input.Row01Issuer.toUpperCase(); input.Row02Endpoint = input.Row01Endpoint.toUpperCase();
    assert.match(f.validate(input), /Dasselbe Gerät.*1 und 2/);
    input.Row02Enabled = 0; assert.equal(f.validate(input), true);
    input.Row02Enabled = 1; input.EtsAuthorization = 0; assert.equal(f.validate(input), true);
});
test('Info holen exposes duplicate IDs and preserves actionable warnings in 80 characters', () => {
    const f = fixture(); f.parameters.HK_Authorization02Enabled.value = 1;
    f.parameters.HK_Authorization02AccessId.value = 100; f.check();
    assert.match(f.info(), /Berechtigung 1 und 2 doppelt/);
    assert(f.info().length <= 80);
});
test('Local USB mode and disabled Home Key are not reported as active ETS grants', () => {
    const f = fixture(); f.parameters.HK_EtsAuthorization.value = 0; f.check();
    assert.match(f.info(), /USB-Berechtigungen aktiv/);
    f.parameters.HK_Enabled.value = 0; f.check(); assert.match(f.info(), /Home Key deaktiviert/);
    assert(f.calls.every(c => c.object === 160));
});
test('Ordinary UID lookup works in legacy applications without the Home Key script', () => {
    const f = fixture(false); f.uid = 'Key fob'; f.check();
    assert.match(f.info(), /UID-Tag: Key fob; Aktion 1: Haustür/);
    assert.equal(f.disconnects, 1);
});
test('Ordinary UID lookup still works with disabled Home Key draft rows', () => {
    const f = fixture(); f.uid = 'Key fob'; f.parameters.HK_Authorization01Enabled.value = 0;
    f.check(); assert.match(f.info(), /UID-Tag: Key fob/);
});
test('The same credential ID can appear in several NFC action assignment rows', () => {
    const f = fixture();
    f.parameters.NFCACT_Fa2ActionId = { value: 2 };
    f.parameters.NFCACT_Fa2NfcId = { value: 100 };
    f.parameters.NFCACT_Fa2NfcActionInfo = { value: '' };
    f.parameters.ACC_Act2Description = { value: 'Licht' };
    f.check();
    // A second action assignment is unrelated to unique Home Key identities.
    f.check(2);
    assert.match(f.texts.at(-1), /Aktion 2: Licht/);
    assert.match(f.info(), /Provisioniert/);
    assert.match(f.parameters.NFCACT_Fa2NfcActionInfo.value, /Provisioniert/);
    assert.equal(f.validate(f.validationInput()), true);
});
test('Not-found and communication errors replace stale successful information', () => {
    const f = fixture(false); f.check(); assert.match(f.info(), /^Kein UID-Tag/);
    f.failure = 'timeout'; assert.throws(f.check, /timeout/);
    assert.match(f.info(), /Abfrage fehlgeschlagen: timeout/); assert.equal(f.disconnects, 2);
});
test('Home Key reader failures close the connection and clear a previous good result', () => {
    const f = fixture(); f.check(); assert.match(f.info(), /^Provisioniert/);
    f.homeKeyFailure = 'reader unavailable'; assert.throws(f.check, /reader unavailable/);
    assert.match(f.info(), /^Abfrage fehlgeschlagen: reader unavailable/);
    assert.equal(f.connects, 2); assert.equal(f.disconnects, 2);
});
test('All active Home Key IDs must fit the shared slot range; drafts stay editable', () => {
    const f = fixture(), input = f.validationInput();
    for (const id of [-1, 1500, 1.5, NaN]) {
        input.Row01AccessId = id; assert.match(f.validate(input), /0 bis 1499/);
    }
    input.Row01AccessId = 0; input.Row01Issuer = ''; input.Row01Endpoint = '';
    assert.equal(f.validate(input), true);
    f.parameters.HK_Authorization01Issuer.value = ''; f.check();
    assert.match(f.info(), /^Ungültige Geräte-ID/);
});
test('Invalid action numbers do not open an online connection', () => {
    for (const id of [0, 3]) {
        const f = fixture(); f.parameters.NFCACT_Fa1ActionId.value = id; f.check();
        assert.match(f.info(), /Aktion ist nicht definiert/); assert.equal(f.connects, 0);
    }
});
console.log(`${count} NFC/Home Key assignment checks passed`);
