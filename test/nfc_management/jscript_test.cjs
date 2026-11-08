// Execute the actual ETS scripts in an isolated realm without ES5+ library APIs.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

function fixture() {
    const parameters = {
        HK_Enabled: { value: 1 }, HK_EtsAuthorization: { value: 1 }, HK_OnlineStatus: { value: '' },
        NFCUI_Slot: { value: 1 }, NFCUI_Status: { value: '' }, NFCUI_Snapshot: { value: '' },
        NFCUI_Source:{value:1},NFCUI_HomeKeyCache:{value:''},NFCUI_ActionsCache:{value:''},NFCUI_DeviceSnapshotReady:{value:0},NFCUI_Page: { value: 1 }, NFCACT_NfcActionCount: { value: 3 }, ACC_VisibleActions: { value: 2 }
    };
    for (let i=0;i<16;i++) parameters['NFCUI_Cache'+i] = { value: '' };
    for (let row=1;row<=16;row++) for (const [name,value] of Object.entries({
        Enabled: 0, Issuer: '', Endpoint: '', AccessId: 0, Description: ''
    })) parameters['HK_Authorization'+String(row).padStart(2,'0')+name] = { value };
    for (let i=1;i<=3;i++) {
        parameters['NFCACT_Fa'+i+'ActionId'] = { value: i===2 ? 1 : i };
        parameters['NFCACT_Fa'+i+'NfcId'] = { value: 42 };
    }
    for (const [field,value] of Object.entries({ Enabled: 1, Issuer: '0011223344556677',
        Endpoint: '112233445501', AccessId: 42, Description: 'Mein iPhone' }))
        parameters['HK_Authorization01'+field].value=value;
    const state = { connects: 0, disconnects: 0, calls: [], occupied: false, revoked: false };
    const device = { getParameterByName: name => parameters[name] };
    const online = { connect() { state.connects++; }, disconnect() { state.disconnects++; } };
    const progress = { isCanceled() { return false; }, setProgress() {}, setText() {} };
    const context = vm.createContext({
        BASE_invokeFunctionPropertyWrapper(object,property,data) {
            assert.equal(property,3); state.calls.push([object,...data]);
            if(object===160&&data[0]===114){
                const cursor=data[1]*256+data[2],count=3,next=cursor===count?count:3;
                let rows=[];if(cursor!==count)for(let i=1;i<=3;i++){let action=parameters['NFCACT_Fa'+i+'ActionId'].value,id=parameters['NFCACT_Fa'+i+'NfcId'].value;rows.push(action>>8,action&255,id>>8,id&255);}
                return [0,1,0,0,0,1,0,next,rows.length/4,0,count,0,2,1,...rows];
            }
            if(object===165&&data[0]===11){
                const index=data[1],header=[0,1,index,16,7,0,0,0,1];if(index===16)return header;
                const pre='HK_Authorization'+String(index+1).padStart(2,'0'),issuer=parameters[pre+'Issuer'].value,endpoint=parameters[pre+'Endpoint'].value;
                const valid=/^[0-9a-f]{16}$/.test(issuer)&&/^[0-9a-f]{12}$/.test(endpoint),id=parameters[pre+'AccessId'].value;
                return [...header,(valid?10:0)|(parameters[pre+'Enabled'].value?1:0),id>>8,id&255,...(valid?Buffer.from(issuer+endpoint,'hex'):Buffer.alloc(14)),...Buffer.from(parameters[pre+'Description'].value.padEnd(32,'\0'),'latin1')];
            }
            if (object===160) {
                assert.equal(data[0],113);
                const cursor=data[1]*256+data[2];
                const records=state.occupied&&cursor===0 ? [0,42,...Array(10).fill(1),...Array(28).fill(0)] : [];
                return [0,1,0,0,0,1,5,220,records.length?1:0,...records];
            }
            assert.equal(object,165);
            if(data[0]===0)return [0,1,1,1,0,0,2,1,0,0,0,1];
            if(data[0]===5)return [0,1,0,0,0,77];
            if(data[0]===6)return [0,1,1,0,0,0,0,77,...Buffer.from('0011223344556677112233445501','hex')];
            if(data[0]===7){if(state.finishFailure)throw Error('Network finish failed');return [0,1];}
            assert.equal(data[0],10);
            return [0,1,state.revoked?1:0,1];
        }
    });
    vm.runInContext(`
        Array.prototype.map = Array.prototype.indexOf = Array.prototype.lastIndexOf = undefined;
        Array.prototype.filter = Array.prototype.forEach = Array.prototype.every = Array.prototype.some = undefined;
        Array.prototype.reduce = Array.prototype.reduceRight = Array.prototype.find = Array.prototype.findIndex = undefined;
        Array.prototype.includes = Array.prototype.flat = Array.prototype.flatMap = Array.prototype.at = undefined;
        Array.isArray = undefined;
        Object.keys = Object.values = Object.entries = Object.assign = Object.create = undefined;
        String.prototype.trim = String.prototype.startsWith = String.prototype.endsWith = undefined;
        String.prototype.includes = String.prototype.repeat = String.prototype.padStart = String.prototype.padEnd = undefined;
        Date.now = Number.isInteger = JSON = undefined;
    `,context);
    for (const file of ['lib/OFM-HomeKey/src/HomeKey.script.js','src/NfcManagement.script.js'])
        vm.runInContext(fs.readFileSync(file,'utf8'),context,{filename:file});
    return { context, state, parameters, device, online, progress };
}

{
    const f=fixture();
    f.context.NFCUI_checkHomeKey(f.device,f.online,f.progress,{});
    assert.match(f.parameters.NFCUI_Status.value,/Zuordnung geprüft/);
    assert.equal(f.state.connects,f.state.disconnects);
    assert(f.state.calls.some(call=>call[0]===165&&call[1]===10));
    const input=f.context.NFCUI_input(f.device);
    const model=f.context.NFCUI_rows(input);
    assert.equal(model.rows[0].actions,'1, 3 (nicht konfiguriert)');
    const output={};
    f.context.NFCUI_calculate(input,output,{});
    assert.equal(output.View0Id,'42');
    assert.equal(output.View0Actions,'1, 3 (nicht konfiguriert)');
    assert.equal(output.View0Description,'Mein iPhone');
    input.ActionCount=0;
    assert.equal(f.context.NFCUI_rows(input).rows[0].actions,'');
    f.parameters.HK_Authorization01Enabled.value=0;
    assert.equal(f.context.NFCUI_rows(f.context.NFCUI_input(f.device)).unassigned[0],'42');
}
{
    const f=fixture();f.state.occupied=true;
    assert.throws(()=>f.context.NFCUI_checkHomeKey(f.device,f.online,f.progress,{}),/Doppelte Tag-ID/);
    assert.equal(f.state.connects,f.state.disconnects);
    assert(!f.state.calls.some(call=>call[0]===165&&call[1]===10));
    const output={};f.context.NFCUI_calculate(f.context.NFCUI_input(f.device),output,{});
    assert.match(output.Summary,/2 Konfliktzeilen/);
}
{
    const f=fixture();f.state.revoked=true;
    assert.throws(()=>f.context.NFCUI_checkHomeKey(f.device,f.online,f.progress,{}),/Gerät ist gesperrt/);
    assert.equal(f.state.connects,f.state.disconnects);
}
{
    const f=fixture();
    f.parameters.HK_Authorization01Issuer.value='';f.parameters.HK_Authorization01Endpoint.value='';
    f.context.NFCUI_deleteHomeKey(f.device,f.online,f.progress,{});
    assert.equal(f.parameters.HK_Authorization01Enabled.value,0);
    assert.equal(f.parameters.HK_Authorization01Description.value,'');
    assert.equal(f.state.connects,0);assert.equal(f.state.calls.length,0);
    assert.match(f.parameters.NFCUI_Status.value,/Kein Gerät im Reader gelöscht oder gesperrt/);
    assert.match(f.parameters.NFCUI_Status.value,/Parameter herunterladen/);
    assert(f.parameters.NFCUI_Status.value.length<=199);
}
for(const failure of [false,true]){
    const f=fixture();f.state.finishFailure=failure;f.context.HK_wait=function(){};
    for(const [name,value] of Object.entries({Enabled:0,Issuer:'',Endpoint:'',AccessId:0,Description:''}))
        f.parameters['HK_Authorization01'+name].value=value;
    if(failure){
        assert.throws(()=>f.context.NFCUI_identify(f.device,f.online,f.progress,{}),/Network finish failed/);
        assert.equal(f.parameters.HK_Authorization01Issuer.value,'');
        assert.equal(f.parameters.HK_Authorization01Endpoint.value,'');
    }else{
        f.context.NFCUI_identify(f.device,f.online,f.progress,{});
        assert.equal(f.parameters.HK_Authorization01Issuer.value,'0011223344556677');
        assert.equal(f.parameters.HK_Authorization01Endpoint.value,'112233445501');
    }
    assert.equal(f.parameters.HK_Authorization01Enabled.value,0);
    assert.equal(f.state.connects,f.state.disconnects);
}
console.log('ETS scripts with ES5+ APIs disabled: assignments, deletion and transactional identification passed');
