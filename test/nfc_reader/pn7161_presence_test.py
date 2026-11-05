"""Exercise the actual PN7161 presence method with asynchronous NCI replies."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
source=(root/'lib/PN7161/src/Pn7161Reader.cpp').read_text()
method=source[source.index('bool Pn7161Reader::isTagStillPresent()'):source.index('void Pn7161Reader::releaseTag()')]
cpp=r'''
#include <cstdint>
#include <cassert>
using TickType_t=uint32_t;constexpr int ESP_OK=0,portTICK_PERIOD_MS=1;
namespace nci{constexpr int PROT_ISODEP=4;}
uint32_t ticks=0;uint32_t xTaskGetTickCount(){return ticks;}
struct Nci {bool present=true;int error=0;unsigned probes=0;
 int rf_iso_dep_presence_check(){++probes;return error;}
 bool tag_in_field(){return present;}
};
struct Pn7161Reader {Nci* m_nci=nullptr;bool m_connected=true;
 uint8_t m_currentProtocol=nci::PROT_ISODEP;uint32_t m_lastPresenceCheck=0;
 static constexpr uint32_t kPresenceCheckIntervalMs=500;
 bool isTagStillPresent();
};
'''+method+r'''
int main(){
 Nci nci;Pn7161Reader reader;reader.m_nci=&nci;
 assert(reader.isTagStillPresent()&&nci.probes==0);
 ticks=500;assert(reader.isTagStillPresent()&&nci.probes==1);
 nci.present=false;ticks=600;assert(!reader.isTagStillPresent());
 nci.present=true;nci.error=-1;ticks=1000;
 assert(!reader.isTagStillPresent()&&!reader.m_connected&&nci.probes==2);
 reader.m_nci=nullptr;assert(!reader.isTagStillPresent());
}
'''
with tempfile.TemporaryDirectory(prefix='pn7161-presence-') as temp:
 p=Path(temp);(p/'test.cpp').write_text(cpp)
 subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PN7161 presence: asynchronous removal, probe interval and transport-fault exit passed')
