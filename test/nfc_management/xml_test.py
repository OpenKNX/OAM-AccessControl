"""Check the producer-expanded UI and preservation of downloaded parameter layout."""
from pathlib import Path
import re, subprocess, xml.etree.ElementTree as E
root=Path(__file__).resolve().parents[2]
ns='{http://knx.org/xml/project/20}'
# Guard the entire document, including IDs copied from legacy editor fragments.
from collections import Counter
xml=E.parse(root/'src/AccessControl.debug.xml').getroot()
id_counts=Counter(n.attrib['Id'] for n in xml.iter() if 'Id' in n.attrib)
assert not [id for id,count in id_counts.items() if count>1], 'Duplicate expanded ETS IDs'
parameters={p.attrib['Name']:p for p in xml.iter(ns+'Parameter')}
refs={}
for reference in xml.iter(ns+'ParameterRef'):
 refs.setdefault(reference.attrib['RefId'],reference.attrib['Id'])
blocks=list(xml.iter(ns+'ParameterBlock'))
assert len([b for b in blocks if b.attrib.get('Text')=='NFC-Scanner'])==1
assert not any(re.fullmatch(r'Berechtigung \d+',b.attrib.get('Text','')) for b in blocks)
nfc=next(b for b in blocks if b.attrib.get('Text')=='NFC-Scanner')
buttons={b.attrib.get('EventHandler') for b in nfc.iter(ns+'Button')}
assert {'NFCUI_import','NFCUI_identify','NFCUI_deleteHomeKey','NFCUI_restoreHomeKey','NFCUI_refresh','NFCUI_checkHomeKey'}<=buttons
calc=next(c for c in xml.iter(ns+'ParameterCalculation') if c.attrib.get('Name')=='NFCUI_Overview')
inputs={p.attrib['RefId'] for p in calc.find(ns+'RParameters')}
for r in range(1,17):
 for field in ['Enabled','Issuer','Endpoint','AccessId','Description']:
  name=f'HK_Authorization{r:02}{field}';assert refs[parameters[name].attrib['Id']] in inputs
for i in range(1,311):
 for field in ['ActionId','NfcId']:
  assert refs[parameters[f'NFCACT_Fa{i}{field}'].attrib['Id']] in inputs
for name,p in parameters.items():
 if name.startswith('NFCUI_'):assert p.find(ns+'Memory') is None
# Editor selection must not hide/remove the raw-data/external-enrollment GAs.
parents={child:parent for parent in nfc.iter() for child in parent}
type_ref=refs[parameters['NFCUI_Type'].attrib['Id']]
for obj in nfc.iter(ns+'ComObjectRefRef'):
 node=obj
 while node in parents:
  node=parents[node]
  assert node.attrib.get('ParamRefId')!=type_ref
# No changed parameter offsets or channel allocation in the firmware header.
old=subprocess.check_output(['git','show','HEAD:include/knxprod.h'],cwd=root,text=True)
new=(root/'include/knxprod.h').read_text()
def definitions(text):return {m[0]:m[1].strip() for m in re.findall(r'^#define ((?:HK|ACC|NFCACT)_\w+)\s+([^\n]+)',text,re.M)}
assert definitions(old)==definitions(new)
print('Unified ETS layout, live dependencies, object visibility and unchanged firmware memory offsets passed')

# The shared operation dropdown must expose only the selected credential workflow.
reference_defs={node.attrib['Id']:node for node in xml.iter(ns+'ParameterRef')}
parameter_defs={node.attrib['Id']:node for node in xml.iter(ns+'Parameter')}
def visible(node, values):
 if node.tag==ns+'choose':
  reference=reference_defs[node.attrib['ParamRefId']]
  parameter=parameter_defs[reference.attrib['RefId']]
  value=int(values.get(parameter.attrib['Name'],reference.attrib.get('Value',parameter.attrib.get('Value','0'))))
  selected=[]
  for branch in node:
   test=branch.attrib['test']
   if test.startswith('!='):matches=value!=int(test[2:])
   elif test.startswith('>='):matches=value>=int(test[2:])
   else:matches=str(value) in test.split()
   if matches:selected.extend(item for child in branch for item in visible(child,values))
  return selected
 return [node]+[item for child in node for item in visible(child,values)]

def refnames(nodes):
 return {parameter_defs[reference_defs[n.attrib['RefId']].attrib['RefId']].attrib['Name'] for n in nodes if n.tag==ns+'ParameterRefRef'}
operation_ref=refs[parameters['ACC_NfcEdit'].attrib['Id']]
operation_type=next(t for t in xml.iter(ns+'ParameterType') if t.attrib['Id']==parameters['ACC_NfcEdit'].attrib['ParameterType'])
assert operation_type.find(ns+'TypeRestriction').attrib['UIHint']=='DropDown'
assert all(len(b.attrib['Text'])<=20 for b in nfc.iter(ns+'Button'))
for kind in (0,1):
 for operation in range(4):
  for slot in (1,16):
   nodes=visible(nfc,{'NFCUI_Type':kind,'ACC_NfcEdit':operation,'NFCUI_Slot':slot,'HK_Enabled':1,'HK_EtsAuthorization':1})
   assert sum(n.tag==ns+'ParameterRefRef' and n.attrib['RefId']==operation_ref for n in nodes)==1
   handlers={n.attrib['EventHandler'] for n in nodes if n.tag==ns+'Button'}
   if not kind:
    expected={0:'NFCUI_enrollUid',1:'NFCUI_changeUid',2:'NFCUI_deleteUid',3:'NFCUI_changeUid'}[operation]
    assert expected in handlers and not (handlers & {'NFCUI_identify','NFCUI_import','NFCUI_deleteHomeKey','NFCUI_restoreHomeKey'})
    continue
   expected={0:{'NFCUI_identify','NFCUI_restoreHomeKey','NFCUI_checkHomeKey'},1:{'NFCUI_checkHomeKey'},2:{'NFCUI_deleteHomeKey'},3:{'NFCUI_import','NFCUI_checkHomeKey'}}[operation]
   assert handlers==expected|{'NFCUI_refresh'},(operation,handlers)
   fields=refnames(nodes)
   prefix=f'HK_Authorization{slot:02}'
   assert (prefix+'Enabled' in fields)==(operation!=2)
   assert (prefix+'Issuer' in fields)==(operation==3)
   assert (prefix+'Endpoint' in fields)==(operation==3)
   assert prefix+'Description' in fields and prefix+'AccessId' in fields
   other='HK_Authorization'+('16' if slot==1 else '01')
   assert not any(name.startswith(other) for name in fields)
   if operation==2:
    for node in nodes:
     if node.tag!=ns+'ParameterRefRef':continue
     reference=reference_defs[node.attrib['RefId']]
     name=parameter_defs[reference.attrib['RefId']].attrib['Name']
     if name in (prefix+'Description',prefix+'AccessId'):assert reference.attrib['Access']=='Read'
assert not any('Geräte-IDs können mit der ETS-Schaltfläche importiert werden.' in n.attrib.get('Text','') for n in xml.iter())
print('Compact NFC editor: shared dropdown, operation/slot visibility, read-only deletion and short buttons passed')

# Every visible field and operation has a real help topic, including read-only table cells.
help_topics={path.stem:path for folder in (root/'lib/OFM-HomeKey/src/Baggages/Help_de',root/'lib/OFM-AccessControl/src/Baggages/Help_de') for path in folder.glob('*.md')}
homekey=next(b for b in blocks if b.attrib.get('Text')=='Apple Home Key')
for block in (nfc,homekey):
 for node in block.iter():
  if node.tag not in (ns+'ParameterRefRef',ns+'Button'):continue
  topic=node.attrib.get('HelpContext')
  assert topic in help_topics,(node.attrib,'Missing help topic')
  text=help_topics[topic].read_text(encoding='utf-8-sig')
  assert text.startswith('### ') and len(text)>80,(topic,'Incomplete help topic')
# Only blocked configurations and the destructive bulk-UID reset still need inline notices.
notices=[n for n in nfc.iter(ns+'ParameterSeparator') if n.attrib.get('UIHint')=='Information']
assert len(notices)==2
assert all('deaktiviert' in n.attrib.get('Text','') or 'ETS-Berechtigungstabelle' in n.attrib.get('Text','') for n in notices)
assert not any(n.attrib.get('UIHint')=='Information' for n in homekey.iter(ns+'ParameterSeparator'))
print('ETS help: every field/button linked, complete German topics, routine instruction boxes removed')

# UID bulk deletion never appears as a HomeKey device reset.
assert parameters['ACC_ResetNfc'].attrib['Text']=='Alle UID-Tags löschen?'
for kind in (0,1):
 nodes=visible(nfc,{'NFCUI_Type':kind,'ACC_NfcEdit':2,'HK_Enabled':1,'HK_EtsAuthorization':1,'ACC_ResetNfc':1})
 resets=[node for node in nodes if node.tag==ns+'Button' and node.attrib.get('EventHandler')=='NFCUI_resetUid']
 assert len(resets)==(1 if kind==0 else 0)
 if resets:assert resets[0].attrib['Text']=='UID-Tags löschen'
print('UID-only bulk deletion: explicit label and visibility separate from HomeKey devices passed')
