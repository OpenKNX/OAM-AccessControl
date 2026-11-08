"""Exercise actual ETS HomeKey handlers with programming mode off and real persistence."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
source=(root/'lib/OFM-HomeKey/src/HomeKeyModule.cpp').read_text()
def method(signature):
 start=source.index(signature);end=source.index('{',start)+1;depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
cpp=r'''
#include "CredentialStore.h"
#include "HomeKeyOnline.h"
#include "HomeKeyIdentification.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <iostream>
std::vector<uint8_t> flash,staged;
int nvs_flash_init_partition(const char*){return ESP_OK;}
int nvs_open_from_partition(const char*,const char*,int,nvs_handle_t* handle){*handle=1;return ESP_OK;}
int nvs_get_blob(nvs_handle_t,const char*,void* data,size_t* size){if(flash.empty())return ESP_ERR_NVS_NOT_FOUND;if(data){assert(*size>=flash.size());memcpy(data,flash.data(),flash.size());}*size=flash.size();return ESP_OK;}
int nvs_set_blob(nvs_handle_t,const char*,const void* data,size_t size){staged.assign(static_cast<const uint8_t*>(data),static_cast<const uint8_t*>(data)+size);return ESP_OK;}
int nvs_commit(nvs_handle_t){flash=staged;return ESP_OK;}
int nvs_erase_all(nvs_handle_t){return ESP_OK;}
bool ParamHK_PairingPermitted=true;
uint32_t now=100;uint32_t millis(){return now;}uint32_t esp_random(){return 42;}
struct {bool ready=true,programming=false;unsigned progReads=0;bool configured(){return ready;}bool progMode(){++progReads;return programming;}}knx;
struct {bool running=true;bool active(){return running;}}openknxNfc;
struct HomeKeyModule {
 HomeKeyCredentialStore store;
 HomeKeyIdentification identification;
 bool enabled=true,fatal=false,pairingPermitted=true,endpointDirty=false;
 uint8_t commissioningResult=0;
 std::vector<HomeKey::Mapping> etsMappings;
 unsigned canceled=0;
 HomeKeyModule(){flash.clear();staged.clear();assert(store.begin());}
 bool pairingAllowed(){return false;}uint8_t authorizationStatus(){return 2;}
 void cancelCommissioning(){++canceled;}
 uint8_t requestCommissioning(bool open){return open&&!ParamHK_PairingPermitted?HomeKeyOnline::NotPermitted:HomeKeyOnline::Ok;}
 void readPermissionSnapshot(uint8_t,uint32_t,uint8_t* out,uint8_t& length){out[0]=HomeKeyOnline::Invalid;length=2;}
 bool processFunctionProperty(uint8_t,uint8_t,uint8_t,uint8_t*,uint8_t*,uint8_t&);
 bool processFunctionPropertyState(uint8_t,uint8_t,uint8_t,uint8_t*,uint8_t*,uint8_t&);
};
'''
cpp+=method('bool HomeKeyModule::processFunctionProperty(')+'\n'+method('bool HomeKeyModule::processFunctionPropertyState(')
cpp+=r'''
int main(){
 using namespace HomeKeyOnline;
 uint8_t result[256]={},length=0;
 HomeKey::IssuerId iid{0,1,2,3,4,5,6,7};HomeKey::EndpointId eid{0,1,2,3,4,5};
 auto invoke=[&](HomeKeyModule& module,std::vector<uint8_t> data,bool state=false){
  length=0;assert(state?module.processFunctionPropertyState(Object,Property,data.size(),data.data(),result,length):module.processFunctionProperty(Object,Property,data.size(),data.data(),result,length));return result[0];
 };
 auto tokenPacket=[](uint8_t command,uint32_t token){std::vector<uint8_t> data(5);data[0]=command;put32(data.data()+1,token);return data;};
 auto identityPacket=[&](HomeKeyModule& module,uint8_t command){std::vector<uint8_t> data(19);data[0]=command;put32(data.data()+1,module.store.generation);std::copy(iid.begin(),iid.end(),data.begin()+5);std::copy(eid.begin(),eid.end(),data.begin()+13);return data;};
 {
  HomeKeyModule m;
  assert(invoke(m,{5})==Ok&&length==6);const auto token=get32(result+2);
  assert(invoke(m,tokenPacket(6,token),true)==Ok&&length==22&&!result[2]);
  assert(m.identification.capture(token,now,iid,eid,false));
  assert(invoke(m,tokenPacket(6,token))==Ok&&result[2]);
  assert(!memcmp(result+8,iid.data(),8)&&!memcmp(result+16,eid.data(),6));
  assert(invoke(m,tokenPacket(7,token))==Ok&&!m.identification.active(now));
  assert(invoke(m,{5})==Ok);now+=60000;assert(invoke(m,tokenPacket(6,get32(result+2)))==Expired);
 }
 {
  HomeKeyModule m;ParamHK_PairingPermitted=false;
  assert(invoke(m,{5})==NotPermitted);ParamHK_PairingPermitted=true;
  assert(invoke(m,{5})==Ok);const auto token=get32(result+2);
  ParamHK_PairingPermitted=false;assert(invoke(m,tokenPacket(6,token))==NotPermitted&&!m.identification.active(now));
  // Revocation remains possible with commissioning disabled; restoration does not.
  assert(invoke(m,identityPacket(m,8))==Ok&&m.store.isRevoked(iid,eid));
  assert(invoke(m,identityPacket(m,9))==NotPermitted&&m.store.isRevoked(iid,eid));
  ParamHK_PairingPermitted=true;assert(invoke(m,identityPacket(m,9))==Ok&&!m.store.isRevoked(iid,eid));
  HomeKeyCredentialStore boot;assert(boot.begin()&&!boot.isRevoked(iid,eid));
  m.enabled=false;assert(invoke(m,{5})==Disabled);m.enabled=true;
  openknxNfc.running=false;assert(invoke(m,{5})==Disabled);openknxNfc.running=true;
 }
 {
  HomeKeyModule m;
  ddk::Issuer issuer;issuer.id.assign(iid.begin(),iid.end());issuer.public_key.resize(32,1);
  ddk::Endpoint endpoint;endpoint.id.assign(eid.begin(),eid.end());endpoint.public_key.resize(65,2);endpoint.public_key_x.resize(32,3);endpoint.persistent_key.resize(32,4);endpoint.key_type=ddk::KeyType::Secp256r1;
  issuer.endpoints.push_back(endpoint);m.store.entries.push_back(issuer);m.store.save();assert(m.store.healthy);
  std::vector<uint8_t> legacy(6);legacy[0]=4;put32(legacy.data()+2,m.store.generation);
  ParamHK_PairingPermitted=false;assert(invoke(m,legacy)==Ok&&m.store.isRevoked(iid,eid));
  HomeKeyCredentialStore boot;assert(boot.begin()&&boot.isRevoked(iid,eid)&&!boot.contains(iid,eid));
  auto stale=identityPacket(m,8);--stale[4];assert(invoke(m,stale)==Stale);
  assert(invoke(m,identityPacket(m,8),true)==Invalid);
  assert(invoke(m,identityPacket(m,9),true)==Invalid);
  assert(invoke(m,{5},true)==Invalid);
  knx.ready=false;assert(invoke(m,{5})==Unconfigured);assert(invoke(m,identityPacket(m,8))==Unconfigured);knx.ready=true;
 }
 assert(!knx.programming&&knx.progReads==0);
 std::cout<<"Actual ETS handlers: no programming mode, scan permissions/expiry, persistent deletion, explicit restoration and read-only safety passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='homekey-ets-commissioning-') as temp:
 path=Path(temp);(path/'test.cpp').write_text(cpp);binary=path/'test'
 subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined','-DOPENKNX_HOMEKEY_ENABLED','-DARDUINO_ARCH_ESP32','-I'+str(root/'lib/OFM-HomeKey/test/stubs'),'-I'+str(root/'lib/OFM-HomeKey/src'),'-I'+str(root/'lib/DigitalDoorKey/include'),str(path/'test.cpp'),str(root/'lib/OFM-HomeKey/src/CredentialStore.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
