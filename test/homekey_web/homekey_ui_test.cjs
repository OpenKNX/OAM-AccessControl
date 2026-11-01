const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const qrcode = require('../../lib/OFM-HomeKey/web/assets/qrcode.js');
const jsQR = require(process.argv[2] || 'jsqr');

async function main() {
    const elements = new Map();
    const element = id => {
        if (!elements.has(id)) elements.set(id, {
            value: '', disabled: true, hidden: false, required: false, dataset: {}, textContent: '',
            innerHTML: '',
            replaceChildren() { this.innerHTML = ''; },
            addEventListener(event, callback) { this[event] = callback; }
        });
        return elements.get(id);
    };
    const form = element('homekey-form');
    form.dataset.token = 'a'.repeat(32);
    let status = { enabled: true, commissioning: true, pairing: false, canSetCode: true,
        codeConfigured: false, retryAfter: 0, codeState: 0, setupId: 'ABCD' };
    let timer;
    const posts = [];
    let generatedPayload;
    let statusUnavailable = false;
    const windowEvents = {};
    const script = fs.readFileSync(path.join(__dirname, '..', '..', 'lib', 'OFM-HomeKey',
        'web', 'assets', 'homekey.js'), 'utf8');
    vm.runInNewContext(script, {
        document: { getElementById: element },
        window: {
            setTimeout(callback) { timer = callback; },
            addEventListener(event, callback) { windowEvents[event] = callback; }
        },
        qrcode(type, level) {
            const qr = qrcode(type, level);
            return {
                addData(payload, mode) { generatedPayload = payload; qr.addData(payload, mode); },
                make() { qr.make(); },
                createSvgTag(options) { return qr.createSvgTag(options); }
            };
        },
        fetch: async (url, options) => {
            if (url === '/homekey/status') return { ok: !statusUnavailable, json: async () => ({ ...status }) };
            assert.equal(url, '/homekey/code');
            posts.push(options);
            status.codeState = 1;
            return { ok: true, json: async () => ({ queued: true }) };
        }
    });
    await new Promise(resolve => setImmediate(resolve));
    const previous = element('homekey-previous');
    const code = element('homekey-code');
    const confirm = element('homekey-confirm');
    const submit = () => form.submit({ preventDefault() {} });
    const poll = async () => { await timer(); };
    assert.equal(element('homekey-previous-row').hidden, true);
    assert.equal(previous.required, false);
    assert.equal(element('homekey-save').disabled, false);
    code.value = confirm.value = '12345678';
    await submit();
    assert.equal(posts.length, 0);
    code.value = '24681357'; confirm.value = '13572468';
    await submit();
    assert.equal(posts.length, 0);
    code.value = confirm.value = '24681357';
    await submit();
    assert.equal(posts[0].body, `${form.dataset.token}\n\n24681357`);
    assert.equal(Object.hasOwn(posts[0].headers, 'X-HomeKey-Token'), false);
    assert.equal(code.value + confirm.value + previous.value, '');
    assert.equal(element('homekey-save').disabled, true);
    status = { ...status, codeConfigured: true, codeState: 2 };
    await poll();
    assert.equal(previous.required, true);
    assert.equal(element('homekey-previous-row').hidden, false);
    code.value = confirm.value = '13572468';
    await submit();
    assert.equal(posts.length, 1);
    previous.value = '24681357';
    await submit();
    assert.equal(posts[1].body, `${form.dataset.token}\n24681357\n13572468`);
    assert.equal(code.value + confirm.value + previous.value, '');
    status.codeState = 5;
    await poll();
    assert.match(element('homekey-result').textContent, /falsch/);
    assert.equal(element('homekey-save').disabled, false);
    status.codeState = 6; status.retryAfter = 300;
    await poll();
    assert.equal(element('homekey-save').disabled, true);
    assert.match(element('homekey-permission').textContent, /300/);
    status.retryAfter = 0;
    await poll();
    assert.equal(element('homekey-save').disabled, false);
    status.canSetCode = false;
    await poll();
    assert.equal(element('homekey-save').disabled, true);
    assert.equal(element('homekey-qr-show').disabled, true);
    status.pairing = true;
    status.codeState = 0;
    await poll();
    assert.equal(element('homekey-qr-show').disabled, false); // QR does not change the code.
    const qrInput = element('homekey-qr-code');
    const qrSubmit = () => element('homekey-qr-form').submit({ preventDefault() {} });
    const postsBeforeQr = posts.length;
    for (const currentCode of ['24681357', '00001234', '99999998']) {
        qrInput.value = currentCode;
        qrSubmit();
        assert.equal(qrInput.value, '');
        assert.equal(element('homekey-qr').hidden, false);
        assert.match(element('homekey-qr').innerHTML, /^<svg /);
        assert.match(generatedPayload, /^X-HM:\/\/[0-9A-Z]{9}ABCD$/);
        const bits = BigInt(parseInt(generatedPayload.slice(7, 16), 36));
        assert.equal(bits & 0x7ffffffn, BigInt(currentCode));
        assert.equal((bits >> 27n) & 7n, 2n); // IP, not NFC or BLE pairing.
        assert.equal((bits >> 31n) & 255n, 6n); // HomeSpan Category::Locks.
        assert.equal(bits >> 39n, 0n); // Version/reserved.
        const svg = element('homekey-qr').innerHTML;
        const viewBox = svg.match(/viewBox="0 0 (\d+) (\d+)"/);
        assert.ok(viewBox);
        assert.equal(viewBox[1], viewBox[2]);
        const size = Number(viewBox[1]);
        const pixels = new Uint8ClampedArray(size * size * 4);
        pixels.fill(255);
        const rectangles = [...svg.matchAll(/M(\d+),(\d+)l8,0 0,8 -8,0 0,-8z/g)];
        assert.ok(rectangles.length > 0, 'SVG must contain QR modules');
        for (const rectangle of rectangles) {
            const left = Number(rectangle[1]), top = Number(rectangle[2]);
            assert.ok(left >= 32 && top >= 32 && left + 8 <= size - 32 && top + 8 <= size - 32,
                'SVG must retain a four-module quiet zone');
            for (let y = top; y < top + 8; ++y) for (let x = left; x < left + 8; ++x) {
                const offset = (y * size + x) * 4;
                pixels[offset] = pixels[offset + 1] = pixels[offset + 2] = 0;
            }
        }
        const decoded = jsQR(pixels, size, size);
        assert.ok(decoded, 'Rendered QR SVG must be scannable with an independent decoder');
        assert.equal(decoded.data, generatedPayload);
    }
    assert.equal(posts.length, postsBeforeQr); // QR code/setup code never sent to a server.
    element('homekey-qr-hide').click();
    assert.equal(element('homekey-qr').hidden, true);
    assert.equal(element('homekey-qr').innerHTML, '');
    qrInput.value = '12345678'; qrSubmit();
    assert.equal(element('homekey-qr').hidden, true);
    assert.match(element('homekey-qr-result').textContent, /gültigen/);
    qrInput.value = '24681357'; qrSubmit();
    windowEvents.pagehide();
    assert.equal(element('homekey-qr').innerHTML, '');
    qrInput.value = '24681357'; qrSubmit();
    status.pairing = false;
    await poll();
    assert.equal(element('homekey-qr-show').disabled, true);
    assert.equal(element('homekey-qr').innerHTML, '');
    status.pairing = true; status.setupId = '';
    await poll();
    assert.equal(element('homekey-qr-show').disabled, true);
    status.setupId = 'ABCD';
    await poll();
    qrInput.value = '24681357'; qrSubmit();
    status.codeState = 1;
    await poll();
    assert.equal(element('homekey-qr').innerHTML, '');
    status.codeState = 0;
    await poll();
    qrInput.value = '24681357'; qrSubmit();
    status.commissioning = false;
    await poll();
    assert.equal(element('homekey-qr-show').disabled, true);
    assert.equal(element('homekey-qr').innerHTML, '');
    status.commissioning = true;
    await poll();
    qrInput.value = '24681357'; qrSubmit();
    statusUnavailable = true;
    await poll();
    assert.equal(element('homekey-qr-show').disabled, true);
    assert.equal(element('homekey-qr').innerHTML, '');
    console.log('Home Key form, cooldown and locally generated/scannable HomeKit QR checks passed');
}

main().catch(error => { console.error(error); process.exitCode = 1; });
