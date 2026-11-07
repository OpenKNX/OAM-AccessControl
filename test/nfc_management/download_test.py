"""Keep the stored HomeKey table independent of the editor's visibility state."""
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[2]
namespace = {'k': 'http://knx.org/xml/project/20'}
source = ET.parse(root / 'lib/OFM-HomeKey/src/HomeKey.share.xml')
parameters = {p.attrib['Name']: p for p in source.findall('.//k:Parameters//k:Parameter', namespace)}
policy = ['Enabled', 'PairingPermitted', 'AllowUid', 'EtsAuthorization']
fields = ['Enabled', 'Issuer', 'Endpoint', 'AccessId', 'Description']
expected = policy + [f'Authorization{slot:02d}{field}' for slot in range(1, 17) for field in fields]
assert len(expected) == 84
for name in expected:
    assert parameters[name].attrib.get('LegacyPatchAlways') == 'true', name

# The firmware's unchanged parser expects contiguous 33-byte rows. In particular,
# fixing visibility must not move existing credentials or policy bytes in NVM.
for slot in range(1, 17):
    offset = 1 + (slot - 1) * 33
    for field, relative in [('Enabled', 0), ('Issuer', 1), ('Endpoint', 18), ('AccessId', 31)]:
        memory = parameters[f'Authorization{slot:02d}{field}'].find('k:Memory', namespace)
        assert int(memory.attrib['Offset']) == offset + relative

if len(sys.argv) > 1:
    # Check producer output as well: source attributes must survive expansion.
    generated = ET.parse(sys.argv[1])
    stored = {p.attrib['Name']: p for p in generated.findall('.//k:Parameters//k:Parameter', namespace)}
    for name in expected:
        assert stored['HK_' + name].attrib.get('LegacyPatchAlways') == 'true', name
    first = int(stored['HK_Authorization01Enabled'].find('k:Memory', namespace).attrib['Offset'])
    for slot in range(1, 17):
        for field, relative in [('Enabled', 0), ('Issuer', 1), ('Endpoint', 18), ('AccessId', 31)]:
            memory = stored[f'HK_Authorization{slot:02d}{field}'].find('k:Memory', namespace)
            assert int(memory.attrib['Offset']) == first + (slot - 1) * 33 + relative

print('All 16 HomeKey slots and policy flags download independently of UI visibility; NVM offsets unchanged')
