"""Exercise the actual ST25 driver through the shared adapter interface."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2];driver=root/'lib/ST25R3916'
with tempfile.TemporaryDirectory(prefix='st25-adapter-') as temp:
 binary=str(Path(temp)/'test')
 subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined','-DARDUINO_ARCH_ESP32',
  '-I'+str(driver/'test/stubs'),'-I'+str(driver/'test/support'),'-I'+str(driver/'src'),
  '-I'+str(root/'lib/OFM-AccessControl/src'),str(root/'test/homekey_st25/adapter_test.cpp'),str(driver/'src/ST25R3916.cpp'),'-o',binary],check=True)
 subprocess.run([binary],check=True)
