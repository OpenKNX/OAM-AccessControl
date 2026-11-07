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
    var rows = [], cache = "", ids = {}, identities = {}, actions = {}, conflicts = 0;
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
    for (var i = 1; i <= 16; i++) {
        var prefix = "Row" + (i < 10 ? "0" : "") + i;
        var issuer = String(input[prefix + "Issuer"] || "").toLowerCase(), endpoint = String(input[prefix + "Endpoint"] || "").toLowerCase();
        var description = String(input[prefix + "Description"] || ""), enabled = Number(input[prefix + "Enabled"]) == 1;
        if (!issuer && !endpoint && !description && !enabled) continue;
        var active = enabled && Number(input.HomeKeyEnabled) == 1 && Number(input.EtsAuthorization) == 1;
        rows.push({ id: Number(input[prefix + "AccessId"]), type: "HomeKey", description: description,
            identity: issuer + "+" + endpoint, active: active, state: "ETS Platz " + i + (active ? ": aktiv (Entwurf)" : ": deaktiviert"), conflict: false });
    }
    for (var i = 0; i < rows.length; i++) {
        var row = rows[i], assignedActions = actions[row.id] || [], actionNames = [];
        for (var a = 0; a < assignedActions.length; a++) {
            var action = assignedActions[a];
            actionNames.push(action + (action > visibleActions ? " (nicht konfiguriert)" : ""));
        }
        row.actions = actionNames.join(", ");
        if (!row.active) continue;
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
    for (var id in actions) if (Object.prototype.hasOwnProperty.call(actions, id) && !ids[id]) unassigned.push(id);
    rows.sort(function(a,b) { return a.id - b.id || (a.type < b.type ? -1 : a.type > b.type ? 1 : 0); });
    return { rows: rows, conflicts: conflicts, unassigned: unassigned };
}
function NFCUI_calculate(input, output, context) {
    var fields = ["Id", "Type", "Description", "Identity", "State", "Actions"];
    for (var r = 0; r < 10; r++) for (var f = 0; f < fields.length; f++) output["View" + r + fields[f]] = "";
    try {
        var model = NFCUI_rows(input), rows = [], query = String(input.Search || "").toLowerCase(), filter = Number(input.Filter);
        for (var i = 0; i < model.rows.length; i++) {
            var row = model.rows[i];
            if ((filter == 1 && row.type != "UID") || (filter == 2 && row.type != "HomeKey") || (filter == 3 && !row.conflict)) continue;
            if (query && (row.id + " " + row.description + " " + row.identity).toLowerCase().indexOf(query) < 0) continue;
            rows.push(row);
        }
        var page = Math.max(1, Number(input.Page) || 1), count = Math.max(1, Math.ceil(rows.length / 10));
        output.Summary = (rows.length + " Einträge; Seite " + page + "/" + count + "; " + model.conflicts + " Konfliktzeilen. " +
            (String(input.Snapshot || "").indexOf("Gelesen:") == 0 ? "UID: Snapshot. " : "UID: nicht aktuell geprüft! ") +
            (model.unassigned.length ? "Aktionen ohne aktiven Zugang bei Tag-ID: " + model.unassigned.join(", ") : "")).substring(0, 255);
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
    HK_run(device, online, progress, function() {
        var cursor = 0, generation = [0,0,0,0], records = [], seen = {}, previous = -1;
        NFCUI_parameter(device,"Snapshot").value = "Wird gelesen; bisheriger Snapshot ggf. veraltet";
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
        var cache = records.join("");
        if (cache.length > 16*8191) throw new Error("UID-Snapshot zu groß");
        var old = []; for (var i=0;i<16;i++) old.push(NFCUI_value(device,"Cache"+i));
        try { for (var i=0;i<16;i++) NFCUI_parameter(device,"Cache"+i).value = cache.substr(i*8191,8191); }
        catch(error) { for(var i=0;i<16;i++) NFCUI_parameter(device,"Cache"+i).value=old[i]; throw error; }
        NFCUI_parameter(device,"Snapshot").value = "Gelesen: " + new Date().toLocaleString() + " (" + records.length + " UID-Tags)";
        NFCUI_parameter(device,"Page").value = 1;
        NFCUI_status(device,"UID-Snapshot vollständig aktualisiert; HomeKey zeigt den aktuellen ETS-Entwurf.");
    });
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
    HK_run(device,online,progress,function(){
        var token=null;
        try {
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
                var f=HK_authorizationFields(device,slot);
                if(!String(f.issuer.value)){f.enabled.value=0;f.issuer.value=issuer;f.endpoint.value=endpoint;f.action.value=0;}
                if(response[3])f.enabled.value=0; // A revoked identity is never shown as an enabled permission.
                NFCUI_parameter(device,"Slot").value=slot;
                NFCUI_status(device,"Gerät identifiziert, Platz "+slot+(response[3]?": GESPERRT. Nur über Erneut zulassen freigeben.":". Bezeichnung und freie Tag-ID vergeben; Berechtigung aktivieren und herunterladen."));
                return;
            }while(new Date().getTime()<until);
            throw new Error("Kein HomeKey-Gerät innerhalb von 60 Sekunden identifiziert");
        } finally { if(token) { try{HK_requireOk(HK_call(device,online,progress,[7].concat(token)));}catch(ignored){} } }
    });
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
    var rows=NFCUI_rows(NFCUI_input(device)).rows, found=[];
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
