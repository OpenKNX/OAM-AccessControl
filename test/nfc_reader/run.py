"""Test PN7160's actual adapter, transport and NCI engine on both platform branches."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
pn=root/'lib/PN7160';acc=root/'lib/OFM-AccessControl'
sources=[pn/'lib'/p for p in ['reader/pn7160reader.cpp','pn7160interface/pn7160interface.cpp',
    'nci/nci.cpp','nci/ncistate.cpp','nci/ncipacket.cpp','tag/tag.cpp','logging/logging.cpp',
    'timer/singletimer.cpp','pn7160config/pn7160config.cpp','pn7160config/pn7160configcollection.cpp']]
with tempfile.TemporaryDirectory(prefix='pn7160-common-reader-') as temp:
    for platform in ['ARDUINO_ARCH_ESP32','PN7160_RP2040']:
        binary=str(Path(temp)/platform)
        subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined','-DunitTesting','-D'+platform,
            '-I'+str(pn/'test/esp32/stubs'),'-I'+str(pn/'lib'),'-I'+str(acc/'src'),
            str(root/'test/nfc_reader/pn7160_test.cpp'),*map(str,sources),'-o',binary],check=True)
        subprocess.run([binary],check=True)
        print(platform+': passed',flush=True)
