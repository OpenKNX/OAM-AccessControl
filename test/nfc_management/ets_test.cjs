const fs=require('fs'),vm=require('vm'),assert=require('assert');
vm.runInThisContext(fs.readFileSync('lib/OFM-HomeKey/src/HomeKey.script.js','utf8'));
vm.runInThisContext(fs.readFileSync('src/NfcManagement.script.js','utf8'));
let tests=0;
function test(name,run){run();tests++;console.log('PASS:',name);}
function input(){let v={HomeKeyEnabled:1,EtsAuthorization:1,Snapshot:'Gelesen: Test',Filter:0,Page:1};for(let i=0;i<16;i++)v['Cache'+i]='';return v;}
function hk(v,row,id,enabled=1){let p='Row'+String(row).padStart(2,'0');Object.assign(v,{[p+'Issuer']:'0011223344556677',[p+'Endpoint']:'1122334455'+String(row).padStart(2,'0'),[p+'Enabled']:enabled,[p+'AccessId']:id,[p+'Description']:'Phone '+row});}
function uid(id,name=''){return id+',11223344000000000000,'+Buffer.from(name,'latin1').toString('hex').padEnd(56,'0')+';';}
function fixture(){let p={HK_Enabled:{value:1},HK_EtsAuthorization:{value:1},HK_OnlineStatus:{value:''}};
 for(let i=1;i<=16;i++){let pre='HK_Authorization'+String(i).padStart(2,'0');for(let f of ['Issuer','Endpoint','Description','Enabled','AccessId'])p[pre+f]={value:['Enabled','AccessId'].includes(f)?0:''};}
 for(let name of ['Type','Slot','Page','Status','Snapshot',...Array.from({length:16},(_,i)=>'Cache'+i)])p['NFCUI_'+name]={value:name==='Slot'?1:''};
 let f={p,closed:0,canceled:false,requests:[],generation:[0,0,0,1],records:[],token:[0,0,0,77],revoked:false};
 f.device={getParameterByName:name=>p[name]};f.online={connect(){},disconnect(){f.closed++;}};f.progress={isCanceled:()=>f.canceled,setText(){},setProgress(){}};
 global.BASE_invokeFunctionPropertyWrapper=(object,property,data)=>{f.requests.push([object,...data]);if(f.override){let r=f.override(object,data);if(r)return r;}
 if(object===160){let cursor=data[1]*256+data[2],rec=f.records.filter(r=>r.id>=cursor).slice(0,5),next=rec.length===5?rec.at(-1).id+1:1500;
 return [0,1,...f.generation,next>>8,next&255,rec.length,...rec.flatMap(r=>[r.id>>8,r.id&255,...Buffer.from(r.uid||'11223344000000000000','hex'),...Buffer.from((r.name||'').padEnd(28,'\0'),'latin1')])];}
 if(data[0]===0)return [0,1,1,1,0,0,2,0,...f.generation];
 if(data[0]===5)return [0,1,...f.token];
 if(data[0]===6)return [0,1,1,f.revoked?1:0,...f.token,...Buffer.from('0011223344556677112233445501','hex')];
 if(data[0]===7||data[0]===9)return [0,1];
 if(data[0]===10)return [0,1,f.revoked?1:0,1];
 throw Error('Unhandled '+data);};return f;}
function refresh(f){NFCUI_refresh(f.device,f.online,f.progress,{});}
test('Live ETS edits, disabled drafts, duplicate IDs and identities',()=>{let v=input();hk(v,1,0);hk(v,2,0,0);v.Cache0=uid(0);assert.equal(NFCUI_rows(v).conflicts,2);v.Row01AccessId=42;assert.equal(NFCUI_rows(v).conflicts,0);v.Row02Enabled=1;v.Row02AccessId=42;assert.equal(NFCUI_rows(v).conflicts,2);v.Row02AccessId=43;v.Row02Endpoint=v.Row01Endpoint;assert.equal(NFCUI_rows(v).conflicts,2);v.HomeKeyEnabled=0;assert.equal(NFCUI_rows(v).conflicts,0);});
test('Action edits update usage without renaming credentials; deletion reveals orphan actions',()=>{let v=input();hk(v,1,42);v.Action1Action=1;v.Action1Id=42;v.Action2Action=2;v.Action2Id=42;let m=NFCUI_rows(v);assert.equal(m.rows[0].actions,'1, 2');assert.equal(m.conflicts,0);v.Action1Id=43;m=NFCUI_rows(v);assert.equal(m.rows[0].id,42);assert.deepEqual(m.unassigned,['43']);v.Row01Enabled=0;assert.deepEqual(NFCUI_rows(v).unassigned,['42','43']);});
test('Hidden action assignments do not appear as usable routes',()=>{let v=input();hk(v,1,42);v.Action1Action=1;v.Action1Id=42;v.Action2Action=2;v.Action2Id=42;v.ActionCount=1;v.VisibleActions=0;assert.equal(NFCUI_rows(v).rows[0].actions,'1 (nicht konfiguriert)');});
test('Overview search, conflict filter and pagination recalculate',()=>{let v=input();for(let i=1;i<=16;i++)hk(v,i,i);v.Page=2;let o={};NFCUI_calculate(v,o);assert.equal(o.View0Id,'11');v.Search='Phone 16';v.Page=1;NFCUI_calculate(v,o);assert.equal(o.View0Id,'16');assert.equal(o.View1Id,'');v.Search='';v.Row02AccessId=1;v.Filter=3;NFCUI_calculate(v,o);assert.equal(o.View0Id,'1');assert.equal(o.View1Id,'1');assert.equal(o.View2Id,'');});
test('Full 1500-record UID snapshot includes unnamed first/last records',()=>{let f=fixture();f.records=Array.from({length:1500},(_,id)=>({id,name:id%2?'Tag '+id:''}));refresh(f);let cache=Array.from({length:16},(_,i)=>f.p['NFCUI_Cache'+i].value).join('');assert.equal(cache.split(';').length-1,1500);assert(cache.startsWith('0,'));assert(cache.includes(';1499,'));assert(f.p.NFCUI_Snapshot.value.startsWith('Gelesen:'));assert.equal(f.requests.at(-1)[2]*256+f.requests.at(-1)[3],1500);assert.equal(f.closed,1);});
test('Empty UID snapshot clears previous cache',()=>{let f=fixture();f.p.NFCUI_Cache0.value=uid(3);refresh(f);assert.equal(f.p.NFCUI_Cache0.value,'');});
test('Stale, malformed, duplicate and canceled snapshots preserve previous cache',()=>{for(let failure of ['stale','malformed','duplicate','cancel']){let f=fixture();f.p.NFCUI_Cache0.value=uid(42);f.records=[{id:1},{id:2}];f.override=(object,data)=>{if(object!==160)return;if(failure==='stale')return [8,1];if(failure==='malformed')return [0,1,0,0,0,1,5,220,3];if(failure==='duplicate')return [0,1,0,0,0,1,5,220,2,...[1,1].flatMap(id=>[0,id,...Array(38).fill(1)])];};f.canceled=failure==='cancel';assert.throws(()=>refresh(f));assert.equal(f.p.NFCUI_Cache0.value,uid(42));assert(!f.p.NFCUI_Snapshot.value.startsWith('Gelesen:'));assert.equal(f.closed,1);}});
test('Identify a revoked phone without granting it; existing names preserved',()=>{let f=fixture(),wait=HK_wait;HK_wait=()=>{};try{f.revoked=true;NFCUI_identify(f.device,f.online,f.progress,{});assert.equal(f.p.HK_Authorization01Enabled.value,0);assert.equal(f.p.HK_Authorization01Endpoint.value,'112233445501');assert(f.p.NFCUI_Status.value.includes('GESPERRT'));assert(f.requests.some(r=>r[1]===7));f.p.HK_Authorization01Description.value='My phone';f.p.HK_Authorization01AccessId.value=123;NFCUI_identify(f.device,f.online,f.progress,{});assert.equal(f.p.HK_Authorization01Description.value,'My phone');assert.equal(f.p.HK_Authorization01AccessId.value,123);}finally{HK_wait=wait;}});
test('Canceled identification sends token cancellation and leaves slots empty',()=>{let f=fixture(),wait=HK_wait;HK_wait=()=>{throw Error('Canceled');};try{assert.throws(()=>NFCUI_identify(f.device,f.online,f.progress,{}));assert(f.requests.some(r=>r[1]===7));assert.equal(f.p.HK_Authorization01Issuer.value,'');assert.equal(f.closed,1);}finally{HK_wait=wait;}});
test('UID writes reject a Tag-ID reserved in the current ETS draft before calling backend',()=>{let f=fixture();f.p.ACC_EnrollNfcId={value:42};f.p.HK_Authorization01Enabled.value=1;f.p.HK_Authorization01Issuer.value='0011223344556677';f.p.HK_Authorization01Endpoint.value='112233445501';f.p.HK_Authorization01AccessId.value=42;let called=false;assert.throws(()=>NFCUI_uidWrite(f.device,f.online,f.progress,{},()=>called=true,true));assert(!called);assert.equal(f.requests.length,0);});
test('Deleting an incomplete slot clears only the ETS draft and never revokes another device',()=>{
 for(let identity of [['',''],['0011223344556677',''],['bad','112233445501']]){
  let f=fixture();f.p.HK_Authorization01Enabled.value=1;f.p.HK_Authorization01Description.value='iphone Franz';
  f.p.HK_Authorization01Issuer.value=identity[0];f.p.HK_Authorization01Endpoint.value=identity[1];
  f.p.HK_Authorization02Issuer.value='0011223344556677';f.p.HK_Authorization02Endpoint.value='112233445501';
  let other=JSON.stringify(Object.fromEntries(Object.entries(f.p).filter(([k])=>k.startsWith('HK_Authorization02'))));
  f.online.connect=()=>{throw Error('Incomplete-slot cleanup must not connect');};
  NFCUI_deleteHomeKey(f.device,f.online,f.progress,{});
  assert.equal(f.requests.length,0);assert.equal(f.p.HK_Authorization01Enabled.value,0);
  for(let field of ['Issuer','Endpoint','Description'])assert.equal(f.p['HK_Authorization01'+field].value,'');
  assert.equal(f.p.HK_Authorization01AccessId.value,0);
  assert.equal(JSON.stringify(Object.fromEntries(Object.entries(f.p).filter(([k])=>k.startsWith('HK_Authorization02')))),other);
  assert(f.p.NFCUI_Status.value.includes('Kein Gerät im Reader gelöscht oder gesperrt'));
  assert(f.p.NFCUI_Status.value.includes('Parameter herunterladen'));
 }
});
test('Canceled incomplete-slot deletion leaves all fields untouched',()=>{
 let f=fixture();f.p.HK_Authorization01Enabled.value=1;f.p.HK_Authorization01Description.value='draft';
 f.canceled=true;let before=JSON.stringify(f.p);assert.throws(()=>NFCUI_deleteHomeKey(f.device,f.online,f.progress,{}));
 assert.equal(JSON.stringify(f.p),before);assert.equal(f.requests.length,0);
});
test('Complete device deletion requires acknowledged revocation before clearing ETS',()=>{
 for(let fail of [true,false]){
  let f=fixture();f.p.HK_Authorization01Enabled.value=1;f.p.HK_Authorization01Issuer.value='0011223344556677';
  f.p.HK_Authorization01Endpoint.value='112233445501';f.override=(object,data)=>data[0]===8?(fail?[7,1]:[0,1]):undefined;
  if(fail){assert.throws(()=>NFCUI_deleteHomeKey(f.device,f.online,f.progress,{}));assert.equal(f.p.HK_Authorization01Enabled.value,1);assert.equal(f.p.HK_Authorization01Endpoint.value,'112233445501');}
  else{NFCUI_deleteHomeKey(f.device,f.online,f.progress,{});assert.equal(f.p.HK_Authorization01Issuer.value,'');assert(f.p.NFCUI_Status.value.includes('Gerät gelöscht und gesperrt'));}
  assert(f.requests.some(r=>r[1]===8));assert.equal(f.closed,1);
 }
});
function homeKeyDraft(f){return JSON.stringify(Object.fromEntries(Object.entries(f.p).filter(([name])=>name.startsWith('HK_Authorization')||name==='NFCUI_Slot')));}
test('Identification never commits a slot before reader acknowledgement and disconnect',()=>{
 let f=fixture(),wait=HK_wait;HK_wait=()=>{};
 try{
  let before=homeKeyDraft(f);f.override=(object,data)=>{if(data[0]===7)assert.equal(homeKeyDraft(f),before);};
  f.online.disconnect=()=>{assert.equal(homeKeyDraft(f),before);f.closed++;};
  NFCUI_identify(f.device,f.online,f.progress,{});
  assert.equal(f.p.HK_Authorization01Issuer.value,'0011223344556677');
  assert.equal(f.p.HK_Authorization01Enabled.value,0);assert.equal(f.closed,1);
 }finally{HK_wait=wait;}
});
test('Identification failures preserve every slot and selection, including late network failures',()=>{
 let wait=HK_wait;HK_wait=()=>{};
 try{for(let failure of ['poll','malformed','finish','finish-refused','disconnect','cancel-after-result','progress']){
  let f=fixture();f.p.NFCUI_Slot.value=2;
  let before=homeKeyDraft(f);
  f.override=(object,data)=>{
   if(data[0]===6){if(failure==='poll')throw Error('Network poll failed');if(failure==='malformed')return [0,1,1];if(failure==='cancel-after-result')f.canceled=true;}
   if(data[0]===7){if(failure==='finish')throw Error('Network finish failed');if(failure==='finish-refused')return [7,1];}
  };
  if(failure==='disconnect')f.online.disconnect=()=>{f.closed++;throw Error('Network disconnect failed');};
  if(failure==='progress')f.progress.setProgress=()=>{throw Error('Progress callback failed');};
  assert.throws(()=>NFCUI_identify(f.device,f.online,f.progress,{}),failure);
  assert.equal(homeKeyDraft(f),before,failure);assert(f.p.NFCUI_Status.value.includes('Anlernen fehlgeschlagen'));
 }}finally{HK_wait=wait;}
});
test('Failed parameter setters roll back partial identification writes and the slot selection',()=>{
 let wait=HK_wait;HK_wait=()=>{};
 try{for(let key of ['HK_Authorization01Enabled','HK_Authorization01Issuer','HK_Authorization01Endpoint','HK_Authorization01AccessId','NFCUI_Slot']){
  let f=fixture();f.p.NFCUI_Slot.value=2;let before=homeKeyDraft(f),value=f.p[key].value,fail=true;
  Object.defineProperty(f.p[key],'value',{enumerable:true,get(){return value;},set(next){if(fail){fail=false;throw Error('Setter failed');}value=next;}});
  assert.throws(()=>NFCUI_identify(f.device,f.online,f.progress,{}));assert.equal(homeKeyDraft(f),before,key);
 }}finally{HK_wait=wait;}
});
test('A failed completion restores existing names, Tag IDs and active rights',()=>{
 let f=fixture(),wait=HK_wait;HK_wait=()=>{};
 try{
  Object.assign(f.p.HK_Authorization01Enabled,{value:1});f.p.HK_Authorization01Issuer.value='0011223344556677';
  f.p.HK_Authorization01Endpoint.value='112233445501';f.p.HK_Authorization01AccessId.value=42;
  f.p.HK_Authorization01Description.value='My phone';f.revoked=true;let before=homeKeyDraft(f);
  f.progress.setProgress=()=>{throw Error('Progress callback failed');};
  assert.throws(()=>NFCUI_identify(f.device,f.online,f.progress,{}));assert.equal(homeKeyDraft(f),before);
 }finally{HK_wait=wait;}
});
test('Identification does not overwrite edits made during the online phase',()=>{
 let f=fixture(),wait=HK_wait;HK_wait=()=>{};
 try{
  f.override=(object,data)=>{if(data[0]===7)f.p.HK_Authorization01Description.value='Edited while waiting';};
  assert.throws(()=>NFCUI_identify(f.device,f.online,f.progress,{}),/während des Anlernens geändert/);
  assert.equal(f.p.HK_Authorization01Description.value,'Edited while waiting');
  assert.equal(f.p.HK_Authorization01Issuer.value,'');assert.equal(f.p.HK_Authorization01Enabled.value,0);
 }finally{HK_wait=wait;}
});
console.log(tests+' unified NFC ETS tests passed');
