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

# XKNX resolves downloads from active refs, even for legacy-patched parameters.
# An Access=None block hides its controls but still evaluates those same refs.
refs = {p.attrib['RefId']: p.attrib['Id'] for p in source.findall('.//k:ParameterRefs/k:ParameterRef', namespace)}
blocks = source.findall('.//k:ParameterBlock[@Name="AuthorizationDownload"]', namespace)
assert len(blocks) == 1 and blocks[0].attrib.get('Access') == 'None'
assert {n.attrib['RefId'] for n in blocks[0].findall('k:ParameterRefRef', namespace)} == {
    refs[parameters[name].attrib['Id']] for name in expected
}

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
    download_blocks = [b for b in generated.findall('.//k:Dynamic//k:ParameterBlock', namespace)
                       if b.attrib.get('Name', '').endswith('AuthorizationDownload')]
    assert len(download_blocks) == 1 and download_blocks[0].attrib.get('Access') == 'None'
    parents = {child: parent for parent in generated.iter() for child in parent}
    parent = parents[download_blocks[0]]
    while parent.tag != '{' + namespace['k'] + '}Dynamic':
        if parent.tag == '{' + namespace['k'] + '}choose':
            # Producer may wrap the whole AccessControl module in its enable flag.
            assert '_UP-00998' in parent.attrib.get('ParamRefId', ''), 'Download block must not depend on NFC/GUI selectors'
        parent = parents[parent]
    generated_refs = {p.attrib['Id']: p.attrib['RefId'] for p in generated.findall('.//k:ParameterRefs/k:ParameterRef', namespace)}
    assert {generated_refs[n.attrib['RefId']] for n in download_blocks[0].findall('k:ParameterRefRef', namespace)} == {
        stored['HK_' + name].attrib['Id'] for name in expected
    }
    first = int(stored['HK_Authorization01Enabled'].find('k:Memory', namespace).attrib['Offset'])
    for slot in range(1, 17):
        for field, relative in [('Enabled', 0), ('Issuer', 1), ('Endpoint', 18), ('AccessId', 31)]:
            memory = stored[f'HK_Authorization{slot:02d}{field}'].find('k:Memory', namespace)
            assert int(memory.attrib['Offset']) == first + (slot - 1) * 33 + relative

print('All 16 HomeKey slots and policy flags download independently of UI visibility; NVM offsets unchanged')
