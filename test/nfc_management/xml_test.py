"""Check the producer-expanded UI and preservation of downloaded parameter layout."""
from pathlib import Path
import re, subprocess, xml.etree.ElementTree as E
root=Path(__file__).resolve().parents[2]
ns='{http://knx.org/xml/project/20}'
xml=E.parse(root/'src/AccessControl.debug.xml').getroot()
parameters={p.attrib['Name']:p for p in xml.iter(ns+'Parameter')}
refs={p.attrib['RefId']:p.attrib['Id'] for p in xml.iter(ns+'ParameterRef')}
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
