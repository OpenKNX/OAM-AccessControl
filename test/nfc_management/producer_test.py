"""Run the producer with a legacy ACC editor lacking its new block-name marker.

Usage: python3 test/nfc_management/producer_test.py /path/to/OpenKNXproducer
"""
from collections import Counter
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as E

root = Path(__file__).resolve().parents[2]
producer = Path(sys.argv[1]).resolve()
module = (root / 'lib/OFM-AccessControl').resolve()
ns = '{http://knx.org/xml/project/20}'

with tempfile.TemporaryDirectory(prefix='nfc-producer-compat-') as temporary:
    temporary = Path(temporary)
    (temporary / 'lib').mkdir()
    for entry in (root / 'lib').iterdir():
        if entry.name != 'OFM-AccessControl':
            (temporary / 'lib' / entry.name).symlink_to(entry.resolve(), target_is_directory=entry.is_dir())
    snapshot = temporary / 'lib/OFM-AccessControl'
    snapshot.mkdir()
    for entry in module.iterdir():
        if entry.name in ('.git', 'src'):
            continue
        (snapshot / entry.name).symlink_to(entry, target_is_directory=entry.is_dir())
    (snapshot / 'src').mkdir()
    for entry in (module / 'src').iterdir():
        if entry.name == 'AccessControl.share.xml':
            source = entry.read_text().replace('Name="NfcScanner" Icon=', 'Icon=')
            (snapshot / 'src' / entry.name).write_text(source)
        else:
            shutil.copytree(entry, snapshot / 'src' / entry.name) if entry.is_dir() else shutil.copy2(entry, snapshot / 'src' / entry.name)
    (temporary / 'src').mkdir()
    application = temporary / 'src/AccessControl.xml'
    for entry in (root / 'src').iterdir():
        if entry.name.endswith('.debug.xml'):
            continue
        if entry.name == 'AccessControl.xml':
            application.write_text(entry.read_text())
        else:
            shutil.copytree(entry, temporary / 'src' / entry.name) if entry.is_dir() else shutil.copy2(entry, temporary / 'src' / entry.name)
    result = subprocess.run([str(producer), 'create', 'src/AccessControl.xml', '-h',
                             str(temporary / 'knxprod.h'), '-o', str(temporary / 'test.knxprod'), '-d'],
                            cwd=temporary, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    assert result.returncode == 0, result.stdout
    assert '- Id-Uniqueness... OK' in result.stdout, result.stdout
    assert '- Integrity of script methods... OK' in result.stdout, result.stdout
    assert 'due to check errors' not in result.stdout, result.stdout
    xml = E.parse(temporary / 'src/AccessControl.debug.xml').getroot()
    subprocess.run([sys.executable, str(root / 'test/nfc_management/download_test.py'),
                    str(temporary / 'src/AccessControl.debug.xml')], check=True)
    ids = Counter(node.attrib['Id'] for node in xml.iter() if 'Id' in node.attrib)
    assert not [id for id, count in ids.items() if count > 1]
    blocks = [node for node in xml.iter(ns + 'ParameterBlock') if node.attrib.get('Text') == 'NFC-Scanner']
    assert len(blocks) == 1
    assert not any(button.attrib.get('EventHandler') == 'ACC_enrollNfc' for button in xml.iter(ns + 'Button'))
    print('Producer compatibility: legacy UID editor excluded, all IDs unique, function warnings suppressed')
