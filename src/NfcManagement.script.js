// ETS-only model: no overview cache is downloaded to the ESP.
// Keep this script compatible with ETS Windows Active Scripting (JScript/ES3).
function NFCUI_parameter(device, name) {
    var p = device.getParameterByName("NFCUI_" + name);
    if (!p) throw new Error("Passende ETS-Anwendung erforderlich: " + name);
    return p;
}
function NFCUI_value(device, name) { return NFCUI_parameter(device, name).value; }
function NFCUI_status(device, text) { NFCUI_parameter(device, "Status").value = String(text).substring(0, 255); }
function NFCUI_rows(input) {
    var rows = [], cache = "", ids = {}, activeIds = {}, identities = {}, actions = {}, conflicts = 0;
    var actionCount = input.ActionCount === undefined ? 310 : Math.max(0, Math.min(310, Number(input.ActionCount)));
    var visibleActions = input.VisibleActions === undefined ? 150 : Number(input.VisibleActions);
    for (var i = 1; i <= actionCount; i++) {
        var action = Number(input["Action" + i + "Action"]), id = Number(input["Action" + i + "Id"]);
        if (action > 0 && id >= 0 && id < 1500) {
            if (!actions[id]) actions[id] = [];
            var assigned = false;
            for (var a = 0; a < actions[id].length; a++) {
                if (actions[id][a] == action) { assigned = true; break; }
            }
            if (!assigned) actions[id].push(action);
        }
    }
    for (var i = 0; i < 16; i++) cache += String(input["Cache" + i] || "");
    var records = cache ? cache.split(";") : [];
    for (var i = 0; i < records.length; i++) {
        if (!records[i]) continue;
        var fields = records[i].split(",");
        if (fields.length != 3 || !/^\d+$/.test(fields[0]) || !/^[0-9a-f]{20}$/.test(fields[1]) || !/^[0-9a-f]{56}$/.test(fields[2]))
            throw new Error("UID-Snapshot unvollständig; erneut aus Reader lesen");
        var name = "";
        for (var p = 0; p < 56; p += 2) { var c = parseInt(fields[2].substr(p, 2), 16); if (!c) break; name += String.fromCharCode(c); }
        rows.push({ id: Number(fields[0]), type: "UID", description: name, identity: fields[1], active: true, state: "Reader-Snapshot", conflict: false });
    }
    for (var i = 1; i <= Math.min(64, Number(input.RowCount) || 16); i++) {
        var prefix = "Row" + (i < 10 ? "0" : "") + i;
        var issuer = String(input[prefix + "Issuer"] || "").toLowerCase(), endpoint = String(input[prefix + "Endpoint"] || "").toLowerCase();
        var description = String(input[prefix + "Description"] || ""), enabled = Number(input[prefix + "Enabled"]) == 1;
        if (!issuer && !endpoint && !description && !enabled) continue;
        var active = enabled && Number(input.HomeKeyEnabled) == 1 && Number(input.EtsAuthorization) == 1;
        var complete=/^[0-9a-f]{16}$/.test(issuer)&&/^[0-9a-f]{12}$/.test(endpoint);
        rows.push({ id: Number(input[prefix + "AccessId"]), type: "HomeKey", description: description,
            identity: issuer||endpoint ? issuer + "+" + endpoint : "", active: active, validate: input[prefix+"Validate"]===true, state: input[prefix+"State"] || ("ETS Platz " + i + (!complete?": unvollständiger Entwurf":active ? ": aktiv (Entwurf)" : ": deaktiviert (Entwurf)")), conflict: false });
    }
    for (var i = 0; i < rows.length; i++) {
        var row = rows[i], assignedActions = actions[row.id] || [], actionNames = [];
        for (var a = 0; a < assignedActions.length; a++) {
            var action = assignedActions[a];
            actionNames.push(action + (action > visibleActions ? " (nicht konfiguriert)" : ""));
        }
        row.actions = actionNames.join(", ");
        if(row.active)activeIds[row.id]=true;
        if (!row.active && !row.validate) continue;
        if (row.id < 0 || row.id > 1499 || row.id % 1 || (row.type == "HomeKey" && !/^[0-9a-f]{16}\+[0-9a-f]{12}$/.test(row.identity))) {
            row.conflict = true; row.state += "; ungültige ID"; continue;
        }
        if (ids[row.id]) { row.conflict = true; ids[row.id].conflict = true; }
        else ids[row.id] = row;
        var key = row.type + ":" + row.identity;
        if (identities[key]) { row.conflict = true; identities[key].conflict = true; }
        else identities[key] = row;
    }
    for (var i = 0; i < rows.length; i++) {
        if (rows[i].conflict) { rows[i].state += "; KONFLIKT (ID / Gerät doppelt)"; conflicts++; }
        if (!rows[i].actions) rows[i].state += "; keine Aktion";
    }
    var unassigned = [];
    for (var id in actions) if (Object.prototype.hasOwnProperty.call(actions, id) && !activeIds[id]) unassigned.push(id);
    rows.sort(function(a,b) { return a.id - b.id || (a.type < b.type ? -1 : a.type > b.type ? 1 : 0); });
    return { rows: rows, conflicts: conflicts, unassigned: unassigned };
}
// Device view is built exclusively from the last complete readback. Typing into
// the editor changes the draft view/validation, never the cached device rows.
function NFCUI_textFromHex(hex) {
    var text="";
    for(var i=0;i<hex.length;i+=2){var ch=parseInt(hex.substr(i,2),16);if(!ch)break;text+=String.fromCharCode(ch);}
    return text;
}
function NFCUI_deviceRows(input) {
    if(Number(input.DeviceSnapshotReady)!=1)return {rows:[],conflicts:0,unassigned:[]};
    var saved={HomeKeyEnabled:1,EtsAuthorization:1,RowCount:64};
    for(var i=0;i<16;i++)saved["Cache"+i]=input["Cache"+i];
    var assignments=String(input.ActionsCache||"").split(",");
    if(assignments.length!=2||!/^\d+$/.test(assignments[0])||!/^([0-9a-f]{8})*$/.test(assignments[1]))throw new Error("Geräte-Snapshot unvollständig; Zugänge erneut einlesen");
    saved.VisibleActions=Number(assignments[0]);saved.ActionCount=assignments[1].length/8;
    for(var i=0;i<saved.ActionCount;i++){
        saved["Action"+(i+1)+"Action"]=parseInt(assignments[1].substr(i*8,4),16);
        saved["Action"+(i+1)+"Id"]=parseInt(assignments[1].substr(i*8+4,4),16);
    }
    var records=String(input.HomeKeyCache||"").split(";"),row=0,notice="";
    for(var i=0;i<records.length;i++){
        if(!records[i])continue;
        if(!/^[0-9a-f]{116}$/.test(records[i]))throw new Error("HomeKey-Snapshot unvollständig; Zugänge erneut einlesen");
        var h=records[i],index=parseInt(h.substr(4,2),16),policy=parseInt(h.substr(8,2),16),flags=parseInt(h.substr(18,2),16);
        if(!(policy&4))notice="HomeKey-Berechtigungstabelle im Gerät ungültig. ";
        // A name/description alone is a draft, not an identified HomeKey device.
        if(!(flags&8))continue;
        var prefix="Row"+(++row<10?"0":"")+row;
        var active=!!((flags&1)&&(flags&2)&&!(flags&4)&&(policy&1)&&(policy&4));
        saved[prefix+"Issuer"]=h.substr(24,16);saved[prefix+"Endpoint"]=h.substr(40,12);
        saved[prefix+"Description"]=NFCUI_textFromHex(h.substr(52,64));
        saved[prefix+"AccessId"]=parseInt(h.substr(20,4),16);saved[prefix+"Enabled"]=active?1:0;
        saved[prefix+"Validate"]=!!((flags&1)&&(policy&1));
        saved[prefix+"State"]="Gerät "+((policy&2)?"ETS Platz "+(index+1):"USB-Zugang "+(index+1))+": "+(active?"aktiv":"deaktiviert")+
            (!(policy&4)?"; Berechtigungstabelle ungültig":"")+((flags&4)?"; gesperrt":!(flags&2)?"; nicht provisioniert":"");
    }
    var model=NFCUI_rows(saved);model.notice=notice;return model;
}
function NFCUI_readActions(device,online,progress,expected) {
    var cursor=expected?expected.total:0,generation=expected?expected.generation:[0,0,0,0],cache="";
    var total=expected?expected.total:-1,visible=expected?expected.visible:-1,homeKey=expected?expected.homeKey:-1;
    do{
        if(progress.isCanceled())throw new Error("NFC: Abgebrochen");
        var r=BASE_invokeFunctionPropertyWrapper(160,3,[114,cursor>>8,cursor&255].concat(generation),device,online,progress);
        if(!r||r.length<14||r[0]!=0||r[1]!=1)throw new Error("Aktions-Snapshot nicht verfügbar/geändert; passende Firmware erforderlich");
        var next=r[6]*256+r[7],count=Number(r[8]),size=r[9]*256+r[10],shown=r[11]*256+r[12],feature=Number(r[13]);
        if(count>32||size>310||r.length!=14+count*4||next!=cursor+count||next>size||feature>1||shown>150||
            (cursor<size&&!count)||(total>=0&&(total!=size||visible!=shown||homeKey!=feature||HK_hex(r,2,4)!=HK_hex(generation,0,4))))throw new Error("Ungültiger/geänderter Aktions-Snapshot");
        generation=r.slice(2,6);total=size;visible=shown;homeKey=feature;
        cache+=HK_hex(r,14,count*4);
        var finished=cursor==total;cursor=next;
    }while(!finished);
    return {cache:visible+","+cache,homeKey:homeKey,total:total,visible:visible,generation:generation};
}
function NFCUI_readHomeKeys(device,online,progress,expected) {
    var index=expected?expected.total:0,total=expected?expected.total:-1,generation=expected?expected.generation:[0,0,0,0];
    var policy=expected?expected.policy:-1,records=[];
    do{
        if(progress.isCanceled())throw new Error("NFC: Abgebrochen");
        var r,attempt=0;
        do{
            r=HK_call(device,online,progress,[11,index].concat(generation));
            if(r[0]!=1)break;
            if(++attempt>=5)throw new Error("HomeKey-Snapshot: Gerät bleibt beschäftigt");
            HK_wait(progress,200);
        }while(true);
        if(r[0]==2)throw new Error("HomeKey-Gerätestand benötigt aktualisierte Firmware");
        HK_requireOk(r);
        if(r.length<9||r[2]!=index||r[3]>64||r[4]>7||r.length!=(index==r[3]?9:58)||
            (total>=0&&(r[3]!=total||r[4]!=policy||HK_hex(r,5,4)!=HK_hex(generation,0,4))))throw new Error("Ungültiger/geänderter HomeKey-Snapshot; passende Firmware erforderlich");
        total=Number(r[3]);policy=Number(r[4]);generation=r.slice(5,9);
        if(index<total)records.push(HK_hex(r,0,r.length));
    }while(index++<total);
    return {cache:records.join(";"),total:total,policy:policy,generation:generation};
}
function NFCUI_calculate(input, output, context) {
    var fields = ["Id", "Type", "Description", "Identity", "State", "Actions"];
    for (var r = 0; r < 10; r++) for (var f = 0; f < fields.length; f++) output["View" + r + fields[f]] = "";
    try {
        var deviceView=Number(input.Source||0)==0;
        var model = deviceView?NFCUI_deviceRows(input):NFCUI_rows(input), rows = [], query = String(input.Search || "").toLowerCase(), filter = Number(input.Filter);
        for (var i = 0; i < model.rows.length; i++) {
            var row = model.rows[i];
            if ((filter == 1 && row.type != "UID") || (filter == 2 && row.type != "HomeKey") || (filter == 3 && !row.conflict)) continue;
            if (query && (row.id + " " + row.description + " " + row.identity).toLowerCase().indexOf(query) < 0) continue;
            rows.push(row);
        }
        var page = Math.max(1, Number(input.Page) || 1), count = Math.max(1, Math.ceil(rows.length / 10));
        output.Summary = (rows.length + " Einträge; Seite " + page + "/" + count + "; " + model.conflicts + " Konfliktzeilen. " +
            (deviceView ? (Number(input.DeviceSnapshotReady)==1 ? "Gerätestand: letzter vollständiger Snapshot. " : "Noch kein Gerätestand eingelesen. ") : "ETS-Entwurf; noch nicht als Gerätestand bestätigt. ") +
            (model.notice||"")+(model.unassigned.length ? "Aktionen ohne aktiven Zugang bei Tag-ID: " + model.unassigned.join(", ") : "")).substring(0, 255);
        for (var r = 0; r < 10 && (page-1)*10+r < rows.length; r++) {
            var row = rows[(page-1)*10+r], values = [row.id, row.type, row.description, row.identity, row.state, row.actions];
            for (var f = 0; f < fields.length; f++) output["View" + r + fields[f]] = String(values[f]).substring(0, 255);
        }
    } catch (error) { output.Summary = String(error.message).substring(0, 255); }
}
function NFCUI_validate(input, changed, prevValue, context) {
    try { return NFCUI_rows(input).conflicts ? "NFC: Doppelte Tag-ID / Geräte-ID. Konflikte in Allgemein → NFC-Scanner prüfen." : true; }
    catch (error) { return String(error.message); }
}
function NFCUI_input(device) {
    var input = {};
    for (var i = 0; i < 16; i++) input["Cache" + i] = NFCUI_value(device, "Cache" + i);
    input.Snapshot = NFCUI_value(device, "Snapshot");
    var extra=["Source","HomeKeyCache","ActionsCache","DeviceSnapshotReady"];
    for(var i=0;i<extra.length;i++)input[extra[i]]=NFCUI_value(device,extra[i]);
    var count = device.getParameterByName("NFCACT_NfcActionCount"), visible = device.getParameterByName("ACC_VisibleActions");
    input.ActionCount = count ? count.value : 310; input.VisibleActions = visible ? visible.value : 150;
    input.HomeKeyEnabled = device.getParameterByName("HK_Enabled").value;
    input.EtsAuthorization = device.getParameterByName("HK_EtsAuthorization").value;
    for (var i = 1; i <= 16; i++) {
        var f = HK_authorizationFields(device, i), prefix = "Row" + (i < 10 ? "0" : "") + i;
        input[prefix+"Issuer"] = f.issuer.value; input[prefix+"Endpoint"] = f.endpoint.value;
        input[prefix+"Enabled"] = f.enabled.value; input[prefix+"AccessId"] = f.action.value; input[prefix+"Description"] = f.description.value;
    }
    for (var i = 1; i <= 310; i++) {
        var action = device.getParameterByName("NFCACT_Fa"+i+"ActionId"), id = device.getParameterByName("NFCACT_Fa"+i+"NfcId");
        input["Action"+i+"Action"] = action ? action.value : 0; input["Action"+i+"Id"] = id ? id.value : 0;
    }
    return input;
}
function NFCUI_refresh(device, online, progress, context) {
    var connected=false,targets=[],old=[],touched=false;
    try {
        connected=true;online.connect();
        var cursor = 0, generation = [0,0,0,0], records = [], seen = {}, previous = -1;

        do {
            if (progress.isCanceled()) throw new Error("NFC: Abgebrochen");
            var response = BASE_invokeFunctionPropertyWrapper(160,3,[113,cursor>>8,cursor&255].concat(generation),device,online,progress);
            if (!response || response.length < 9 || response[0] != 0 || response[1] != 1)
                throw new Error("UID-Snapshot geändert / Firmware zu alt. Erneut lesen.");
            if (cursor > 0 && HK_hex(response,2,4) != HK_hex(generation,0,4)) throw new Error("UID-Snapshot geändert");
            generation = response.slice(2,6);
            var next = response[6]*256+response[7], count = Number(response[8]);
            if (count > 5 || response.length != 9+count*40 || next < cursor || next > 1500 || (next == cursor && cursor != 1500)) throw new Error("Ungültige UID-Seite");
            for (var i = 0; i < count; i++) {
                var offset = 9+i*40, id = response[offset]*256+response[offset+1];
                if (id < cursor || id >= next || id <= previous || seen[id]) throw new Error("Doppelte/ungültige UID-Snapshot-ID");
                seen[id] = true; previous = id;
                records.push(id+","+HK_hex(response,offset+2,10)+","+HK_hex(response,offset+12,28)+";");
            }
            var finished = cursor == 1500; cursor = next;
            progress.setProgress(Math.floor(cursor*95/1500));
        } while (!finished); // Final empty page validates the snapshot generation.

        var cache=records.join("");
        if(cache.length>16*8191)throw new Error("UID-Snapshot zu groß");
        var actions=NFCUI_readActions(device,online,progress);
        var home=actions.homeKey?NFCUI_readHomeKeys(device,online,progress):{cache:""};
        // Revalidate every generation after reading all subsystems.
        NFCUI_readActions(device,online,progress,actions);
        if(actions.homeKey)NFCUI_readHomeKeys(device,online,progress,home);
        var check=BASE_invokeFunctionPropertyWrapper(160,3,[113,5,220].concat(generation),device,online,progress);
        if(!check||check.length!=9||check[0]!=0||check[1]!=1||check[8]!=0||HK_hex(check,2,4)!=HK_hex(generation,0,4))throw new Error("UID-Snapshot während des Einlesens geändert");
        if(progress.isCanceled())throw new Error("NFC: Abgebrochen");
        online.disconnect();connected=false;
        var names=[],values=[];
        for(var i=0;i<16;i++){names.push("Cache"+i);values.push(cache.substr(i*8191,8191));}
        names=names.concat(["HomeKeyCache","ActionsCache","DeviceSnapshotReady","Snapshot","Page","Source"]);
        values=values.concat([home.cache,actions.cache,1,"Gelesen: "+new Date().toLocaleString()+" ("+records.length+" UID-Tags; HomeKey und Aktionen geprüft)",1,0]);
        for(var i=0;i<names.length;i++){targets.push(NFCUI_parameter(device,names[i]));old.push(targets[i].value);}
        touched=true;
        for(var i=0;i<targets.length;i++)targets[i].value=values[i];
        if(progress.isCanceled())throw new Error("NFC: Abgebrochen");
        progress.setProgress(100);
        NFCUI_status(device,"Gerätestand vollständig eingelesen. ETS-Entwurf separat wählen; Änderungen werden erst nach Download und erneutem Einlesen bestätigt.");
    }catch(error){
        var restored=true;
        if(touched)for(var i=0;i<targets.length;i++){try{targets[i].value=old[i];}catch(ignored){restored=false;}}
        if(!restored){try{NFCUI_parameter(device,"DeviceSnapshotReady").value=0;}catch(ignored){}}
        try{NFCUI_status(device,"Einlesen fehlgeschlagen; "+(restored?"bisheriger Gerätestand unverändert und ggf. veraltet: ":"Snapshot ungültig; erneut einlesen: ")+error.message);}catch(ignored){}
        throw error;
    }finally{if(connected){try{online.disconnect();}catch(ignored){}}}
}
function NFCUI_uidWrite(device, online, progress, context, handler, checkId) {
    if (checkId) {
        var id = Number(device.getParameterByName("ACC_EnrollNfcId").value), model = NFCUI_rows(NFCUI_input(device));
        for (var i=0;i<model.rows.length;i++) if(model.rows[i].type=="HomeKey" && model.rows[i].active && model.rows[i].id==id)
            throw new Error("Tag-ID " + id + " wird im ETS-Entwurf von HomeKey verwendet.");
    }
    NFCUI_parameter(device,"Snapshot").value = "Veraltet: UID-Bearbeitung gestartet";
    try { handler(device,online,progress,context); }
    finally { try { online.disconnect(); } catch(ignored){} }
    NFCUI_refresh(device,online,progress,context);
}
function NFCUI_enrollUid(d,o,p,c){NFCUI_uidWrite(d,o,p,c,ACC_enrollNfc,true);}
function NFCUI_changeUid(d,o,p,c){NFCUI_uidWrite(d,o,p,c,ACC_changeNfc,true);}
function NFCUI_deleteUid(d,o,p,c){NFCUI_uidWrite(d,o,p,c,ACC_deleteNfc,false);}
function NFCUI_resetUid(d,o,p,c){NFCUI_uidWrite(d,o,p,c,ACC_resetNfcScanner,false);}
function NFCUI_import(device,online,progress,context) {
    HK_importEndpoints(device,online,progress,context);
    NFCUI_status(device,device.getParameterByName("HK_OnlineStatus").value);
}
function NFCUI_selected(device) { return HK_authorizationFields(device,Number(NFCUI_value(device,"Slot"))); }
function NFCUI_identity(fields) {
    var hex=String(fields.issuer.value).toLowerCase()+String(fields.endpoint.value).toLowerCase(), data=[];
    if(!/^[0-9a-f]{16}$/.test(String(fields.issuer.value).toLowerCase()) || !/^[0-9a-f]{12}$/.test(String(fields.endpoint.value).toLowerCase())) throw new Error("Vollständige Issuer- und Endpoint-ID erforderlich");
    for(var i=0;i<hex.length;i+=2)data.push(parseInt(hex.substr(i,2),16));return data;
}
function NFCUI_deleteHomeKey(device,online,progress,context) {
    HK_deleteAuthorization(Number(NFCUI_value(device,"Slot")),device,online,progress,context);
    NFCUI_status(device,device.getParameterByName("HK_OnlineStatus").value);
}
function NFCUI_restoreHomeKey(device,online,progress,context) {
    var fields=NFCUI_selected(device), identity=NFCUI_identity(fields);
    HK_run(device,online,progress,function(){
        var status=HK_status(device,online,progress);
        HK_requireOk(HK_call(device,online,progress,[9].concat(status.slice(8,12),identity)));
        fields.enabled.value=0;
        NFCUI_status(device,"Sperre aufgehoben. Gerät ggf. erneut scannen/provisionieren; freie Tag-ID und Aktionen prüfen, Berechtigung aktivieren und herunterladen.");
    });
}
function NFCUI_identify(device,online,progress,context) {
    var token=null,connected=false,plan=null,previous=null,touched=false;
    var selected=NFCUI_parameter(device,"Slot"),oldSlot=selected.value;
    try {
        connected=true;online.connect();
        var response=HK_call(device,online,progress,[5]);HK_requireOk(response);
        if(response.length!=6)throw new Error("Ungültige Identifizierungsantwort");
        token=response.slice(2,6);var until=new Date().getTime()+60000;
        progress.setText("Gekoppeltes iPhone / Watch an den Reader halten. Zutrittsaktionen sind währenddessen gesperrt.");
        do {
            HK_wait(progress,250);response=HK_call(device,online,progress,[6].concat(token));
            if(response[0]==1)continue;HK_requireOk(response);
            if(response.length!=22 || HK_hex(response,4,4)!=HK_hex(token,0,4))throw new Error("Ungültige Identifizierungsantwort");
            if(!response[2])continue;
            var issuer=HK_hex(response,8,8),endpoint=HK_hex(response,16,6),slot=0,free=0;
            for(var i=1;i<=16;i++){
                var f=HK_authorizationFields(device,i);
                if(String(f.issuer.value).toLowerCase()==issuer&&String(f.endpoint.value).toLowerCase()==endpoint)slot=i;
                if(!free&&!String(f.issuer.value)&&!String(f.endpoint.value)&&!String(f.description.value)&&!Number(f.enabled.value)&&!Number(f.action.value))free=i;
            }
            slot=slot||free;if(!slot)throw new Error("Alle 16 HomeKey-Plätze belegt; zuerst ein altes Gerät löschen.");
            // Stage only. A failed finish/disconnect must not consume a slot.
            var f=HK_authorizationFields(device,slot);
            plan={fields:f,slot:slot,issuer:issuer,endpoint:endpoint,revoked:response[3],
                values:[f.enabled.value,f.issuer.value,f.endpoint.value,f.action.value,f.description.value]};
            break;
        }while(new Date().getTime()<until);
        if(!plan)throw new Error("Kein HomeKey-Gerät innerhalb von 60 Sekunden identifiziert");
        if(progress.isCanceled())throw new Error("Home Key: Abgebrochen");
        HK_requireOk(HK_call(device,online,progress,[7].concat(token)));token=null;
        online.disconnect();connected=false;
        if(progress.isCanceled())throw new Error("Home Key: Abgebrochen");

        // All network work has succeeded. Do not overwrite edits made while
        // waiting for the reader. Preserve every field for rollback as well.
        var f=plan.fields,fields=[f.enabled,f.issuer,f.endpoint,f.action,f.description];
        for(var i=0;i<fields.length;i++)if(fields[i].value!==plan.values[i])
            throw new Error("ETS-Platz wurde während des Anlernens geändert; erneut versuchen");
        if(selected.value!==oldSlot)throw new Error("HomeKey-Auswahl wurde während des Anlernens geändert; erneut versuchen");
        previous=[];for(var i=0;i<fields.length;i++)previous.push(fields[i].value);
        touched=true;
        if(!String(f.issuer.value)){
            f.enabled.value=0;f.issuer.value=plan.issuer;f.endpoint.value=plan.endpoint;f.action.value=0;
        }
        if(plan.revoked)f.enabled.value=0;
        selected.value=plan.slot;
        NFCUI_status(device,"Gerät identifiziert, Platz "+plan.slot+(plan.revoked?": GESPERRT. Nur über Erneut zulassen freigeben.":". Bezeichnung und freie Tag-ID vergeben; Berechtigung aktivieren und herunterladen."));
        if(progress.isCanceled())throw new Error("Home Key: Abgebrochen");
        progress.setProgress(100);
    } catch(error) {
        var rollbackFailed=false;
        if(touched){
            for(var i=0;i<fields.length;i++){
                try{fields[i].value=previous[i];}catch(restoreError){rollbackFailed=true;}
            }
            try{selected.value=oldSlot;}catch(restoreError){rollbackFailed=true;}
        }
        var text="Anlernen fehlgeschlagen: "+error.message+(rollbackFailed?"; ETS-Werte konnten nicht vollständig zurückgesetzt werden. Projekt prüfen.":"; keine HomeKey-Zuordnung übernommen.");
        try{HK_setOnlineStatus(device,text);NFCUI_status(device,text);}catch(ignored){}
        throw error;
    } finally {
        if(token){try{HK_requireOk(HK_call(device,online,progress,[7].concat(token)));}catch(ignored){}}
        if(connected){try{online.disconnect();}catch(ignored){}}
    }
}
function NFCUI_checkHomeKey(device,online,progress,context) {
    NFCUI_refresh(device,online,progress,context);
    var fields=NFCUI_selected(device),identity=NFCUI_identity(fields),validation=NFCUI_validate(NFCUI_input(device));
    if(validation!==true)throw new Error(validation);
    HK_run(device,online,progress,function(){
        var status=HK_status(device,online,progress),response=HK_call(device,online,progress,[10].concat(status.slice(8,12),identity));HK_requireOk(response);
        if(response.length!=4)throw new Error("Ungültige Geräteantwort");
        if(response[2])throw new Error("Gerät ist gesperrt. Explizit Erneut zulassen wählen.");
        if(!response[3])throw new Error("Gerät ist im Reader nicht provisioniert. Zuerst koppeln / scannen.");
        NFCUI_status(device,"Zuordnung geprüft. Änderungen in ETS speichern und Parameter herunterladen. Aktionen für diese Tag-ID in der Übersicht prüfen.");
    });
}

// Existing NFC action searches now include HomeKey descriptions and Tag IDs.
function NFCUI_search(device,online,progress,context,byId) {
    device.getParameterByName("NFCACT_NumberSearchResultsToDisplay").value=0;
    device.getParameterByName("NFCACT_NumberSearchResultsOverflow").value=0;
    NFCUI_refresh(device,online,progress,context);
    var rows=NFCUI_deviceRows(NFCUI_input(device)).rows, found=[];
    var query=String(device.getParameterByName(byId?"NFCACT_NfcId":"NFCACT_NfcName").value).toLowerCase();
    for(var i=0;i<rows.length;i++) {
        var row=rows[i];
        if(byId ? row.id==Number(query) : row.description.toLowerCase().indexOf(query)>=0) found.push(row);
    }
    device.getParameterByName("NFCACT_NumberSearchResultsText").value=found.length;
    device.getParameterByName("NFCACT_NumberSearchResultsToDisplay").value=Math.min(found.length,7);
    device.getParameterByName("NFCACT_NumberSearchResultsOverflow").value=found.length>7?1:0;
    for(var i=0;i<Math.min(found.length,7);i++) {
        var row=found[i];
        device.getParameterByName("NFCACTSER_Nfc"+(i+1)+"Id").value=row.id;
        device.getParameterByName("NFCACTSER_Nfc"+(i+1)+"Name").value=(row.type+": "+(row.description||row.identity)+(row.conflict?" [Konflikt]":"")).substring(0,27);
    }
    progress.setText(found.length+" UID/HomeKey-Zuordnungen gefunden; HomeKey zeigt den ETS-Entwurf.");
}
