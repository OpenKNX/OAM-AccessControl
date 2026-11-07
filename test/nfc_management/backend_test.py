"""Exercise the real function-property boundary and snapshot implementation."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'lib/OFM-AccessControl/src/AccessControl.cpp').read_text()
def method(name):
 a=s.index(name);b=s.index('{',a);depth=1;e=b+1
 while depth:
  depth+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[a:e]
process=method('bool AccessControl::processFunctionProperty(')
listing=method('void AccessControl::handleFunctionPropertyListNfc(')
change=method('void AccessControl::handleFunctionPropertyChangeNfc(')
names=sorted(set(re.findall(r'\b(handleFunctionProperty\w+)\(',process))-{'handleFunctionPropertyListNfc','handleFunctionPropertyChangeNfc'})
cpp=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#include <array>
#include <algorithm>
#define OPENKNX_ACC_NFC_READER
#define MAX_NFCS 1500
#define ACC_CalcNfcStorageOffset(id) (4097+(id)*38)
#define logInfoP(...)
#define logDebugP(...)
#define logHexDebugP(...)
#define logIndentUp(...)
#define logIndentDown(...)
unsigned delayTimerInit(){return 1;}
struct { bool configured(){return true;} } knx;
struct {bool reserve=false;bool idReserved(uint16_t id){return reserve&&id==42;}} openknxNfc;
struct Storage {
 std::array<uint8_t,4097+1500*38> bytes{};unsigned writes=0;
 void read(unsigned offset,uint8_t* out,unsigned size){assert(offset+size<=bytes.size());memcpy(out,bytes.data()+offset,size);}
 void write(unsigned offset,uint8_t* data,unsigned size){assert(offset+size<=bytes.size());memcpy(bytes.data()+offset,data,size);writes++;}
 void commit(){}
};
struct AccessControl {
 Storage _nfcStorage;uint32_t nfcRevision=1;uint16_t nfcListCursor=0xffff;unsigned calls=0,syncRequestedNfcId=0,syncRequestedNfcTimer=0;
 bool processFunctionProperty(uint8_t,uint8_t,uint8_t,uint8_t*,uint8_t*,uint8_t&);
 void handleFunctionPropertyListNfc(uint8_t*,uint8_t*,uint8_t&);
 void handleFunctionPropertyChangeNfc(uint8_t*,uint8_t*,uint8_t&);
'''
cpp+='\n'.join(' template<typename... T> void '+n+'(T...){calls++;}' for n in names)+'\n};\n'+process+listing+change
cpp+=r'''
int main(){
 AccessControl a;uint8_t out[256]={},length=0;
 auto request=[&](std::vector<uint8_t> data){length=0;return a.processFunctionProperty(160,3,data.size(),data.data(),out,length);};
 assert(!request({}));
 for(auto data:std::vector<std::vector<uint8_t>>{{101},{101,0,1},{104,0,1},{112},{113,0,0},{111,5,220},{104,255,255,0}}){assert(request(data)&&out[0]==2);}
 assert(a.calls==0&&a._nfcStorage.writes==0);
 assert(request({101,0,42,'A',0})&&a.calls==1); // Legacy variable-length names remain compatible.
 openknxNfc.reserve=true;assert(request({101,0,42,'A',0})&&out[0]==3&&a.calls==1);
 std::vector<uint8_t> edit={104,0,42,1,2,3,4,0,0,0,0,0,0,0};
 assert(request(edit)&&out[0]==3);openknxNfc.reserve=false;
 assert(request(edit)&&out[0]==0); // Empty description must not suppress the UID write.
 assert(a._nfcStorage.bytes[ACC_CalcNfcStorageOffset(42)]==1);
 edit[2]=43;assert(request(edit)&&out[0]==4); // Duplicate UID is rejected without mutation.
 unsigned writes=a._nfcStorage.writes;assert(writes==1);
 a._nfcStorage.bytes.fill(0);
 for(unsigned id=0;id<1500;id++){auto offset=ACC_CalcNfcStorageOffset(id);a._nfcStorage.bytes[offset]=1;a._nfcStorage.bytes[offset+1]=id>>8;a._nfcStorage.bytes[offset+2]=id;}
 unsigned cursor=0,count=0;uint32_t revision=0;
 do {
  std::vector<uint8_t> packet={113,uint8_t(cursor>>8),uint8_t(cursor),uint8_t(revision>>24),uint8_t(revision>>16),uint8_t(revision>>8),uint8_t(revision)};
  assert(request(packet)&&out[0]==0&&out[1]==1&&length==9+out[8]*40);
  revision=(uint32_t(out[2])<<24)|(uint32_t(out[3])<<16)|(uint32_t(out[4])<<8)|out[5];
  for(unsigned i=0;i<out[8];i++){assert((unsigned(out[9+i*40])<<8|out[10+i*40])==count);count++;}
  cursor=unsigned(out[6])*256+out[7];
 }while(cursor<1500);
 assert(count==1500);
 assert(request({113,5,220,0,0,0,uint8_t(revision)})&&out[0]==0&&length==9);
 ++a.nfcRevision;assert(request({113,5,220,0,0,0,uint8_t(revision)})&&out[0]==8);
 a.nfcRevision=revision;a.nfcListCursor=0xffff;assert(request({113,5,220,0,0,0,uint8_t(revision)})&&out[0]==8); // Reboot cannot continue a prior snapshot.
 assert(request({113,0,0,0,0,0,0})&&out[0]==0); // Fresh snapshot still works.
 assert(a._nfcStorage.writes==writes);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 path=Path(tmp);(path/'test.cpp').write_text(cpp)
 subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined','-g',str(path/'test.cpp'),'-o',str(path/'test')],check=True)
 subprocess.run([str(path/'test')],check=True)
print('NFC function-property validation, UID edits/collisions and complete snapshots passed')
