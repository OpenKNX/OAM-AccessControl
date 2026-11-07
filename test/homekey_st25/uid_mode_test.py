"""Exercise actual HomeKey extension hooks; ordinary UID work stays in the service."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
auth=(root/'lib/OFM-HomeKey/src/HomeKeyAuth.cpp').read_text()
module=(root/'lib/OFM-HomeKey/src/HomeKeyModule.cpp').read_text()
header=(root/'lib/OFM-HomeKey/src/HomeKeyModule.h').read_text()
def method(text,signature):
    start=text.index(signature);end=text.index('{',start)+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
source=r'''
#include "HomeKeyPolicy.h"
#include "HomeKeyIdentification.h"
#include <array>
#include <algorithm>
#include <cassert>
#include <mutex>
#include <iostream>
uint32_t millis(){return 100;}
template<typename T> void xQueueOverwrite(void*,const T*){}
struct Reader {
    unsigned exchanges=0;bool success=false;std::vector<uint8_t> response;
    bool exchangeApdu(const std::vector<uint8_t>&,std::vector<uint8_t>& out,uint32_t){++exchanges;out=response;return success;}
};
using INfcReader=Reader;
struct {bool active()const{return true;}}openknxNfc;
struct {unsigned calls=0;std::vector<uint16_t> reserved;
    bool processExternalNfcUid(const uint8_t*,uint8_t,const uint16_t* ids,size_t count){++calls;reserved.assign(ids,ids+count);return true;}
}openknxAccessControl;
struct HomeKeyModule {
    struct NfcDiagnostic
    {
        std::array<uint8_t, 64> response{};
        std::array<uint8_t, 14> version{};
        HomeKey::CardVersionDiagnostic versionDiagnostic{};
        size_t responseLength = 0;
        uint16_t status = 0;
        bool exchanged = false;
        bool versionAttempted = false;
        bool versionValid = false;
    };
    void* diagnostics=nullptr;
    HomeKeyIdentification identification;
    enum class EventKind {Denied,Authenticated};
    struct Event {EventKind kind=EventKind::Denied;const char* reason=nullptr;uint32_t generation=0;};
    bool homeKeyConfigured=false,enabled=false,allowUid=true,provisioningReady=false,policyLoaded=true;
    struct {std::recursive_mutex mutex;uint32_t generation=7;bool healthy=true;
        struct {std::vector<uint8_t> group_identifier;}identity;
        std::vector<HomeKey::Mapping> mappings;
        bool revoked=false;
        bool isRevoked(const HomeKey::IssuerId&,const HomeKey::EndpointId&)const{return revoked;}
    }store;
    Reader instance;unsigned authentications=0;std::vector<Event> events;
    void authenticate(INfcReader&,const std::vector<uint8_t>&){++authentications;}
    void enqueue(const Event& e){events.push_back(e);}
    const auto& activeMappings()const{return store.mappings;}
    bool processNfcTag(INfcReader&,const std::vector<uint8_t>&,uint8_t);
    bool authorizeNfcUid(const uint8_t*,uint8_t,uint32_t);
    bool prepareNfc(std::array<uint8_t,18>&,uint32_t&);
    __READY__
};
'''.replace('__READY__',method(header,'bool nfcReady() const').replace(' override',''))
source+='\n'+method(auth,'bool HomeKeyModule::processNfcTag(')+'\n'+method(module,'bool HomeKeyModule::authorizeNfcUid(')+'\n'+method(module,'bool HomeKeyModule::prepareNfc(')
source+=r'''
int main(){
    const std::vector<uint8_t> uid{1,2,3,4};
    for(auto sak:{0,0x20}){
        HomeKeyModule m;assert(!m.processNfcTag(m.instance,uid,sak));
        assert(m.instance.exchanges==0&&m.events.empty()&&m.authentications==0);
        assert(m.authorizeNfcUid(uid.data(),uid.size(),7));
    }
    for(bool enabled:{false,true}) {
        HomeKeyModule m;m.homeKeyConfigured=true;m.enabled=enabled;m.allowUid=false;
        m.instance.success=true;m.instance.response={0x6a,0x82};
        assert(!m.processNfcTag(m.instance,uid,0x20)); // Ordinary card; service will call authorization hook.
        assert(!m.authorizeNfcUid(uid.data(),4,7)); // Explicit HomeKey-only setting survives a runtime fault.
    }
    {
        HomeKeyModule m;m.homeKeyConfigured=true;m.instance.success=true;m.instance.response={0x90,0};
        assert(m.processNfcTag(m.instance,uid,0x20)&&m.authentications==1&&m.events.empty());
    }
    for(bool exchanged:{false,true})for(auto response:std::vector<std::vector<uint8_t>>{{},{0x90},{0x69,0x85}}){
        HomeKeyModule m;m.homeKeyConfigured=true;m.instance.success=exchanged;m.instance.response=response;
        assert(m.processNfcTag(m.instance,uid,0x20));
        assert(m.events.back().kind==HomeKeyModule::EventKind::Denied&&m.authentications==0);
    }
    {
        HomeKeyModule m;HomeKey::Mapping reserved;reserved.accessId=42;m.store.mappings.push_back(reserved);
        assert(!m.authorizeNfcUid(uid.data(),4,6));
        assert(m.authorizeNfcUid(uid.data(),4,7)&&openknxAccessControl.reserved==std::vector<uint16_t>{42});
    }
    {
        HomeKeyModule m;m.policyLoaded=false;assert(!m.nfcReady());
        m.policyLoaded=true;m.enabled=true;assert(!m.nfcReady());
        m.provisioningReady=true;assert(m.nfcReady());
        m.store.identity.group_identifier={1,2,3,4,5,6,7,8};
        std::array<uint8_t,18> frame;uint32_t token=0;
        assert(m.prepareNfc(frame,token)&&token==7&&frame[0]==0x6a&&frame[8]==1&&frame[15]==8);
        m.enabled=false;assert(m.prepareNfc(frame,token));
        assert((frame==std::array<uint8_t,18>{}));
        m.enabled=true;m.store.healthy=false;assert(m.prepareNfc(frame,token)&&frame[0]==0);
    }
    {
        HomeKeyModule m;m.homeKeyConfigured=true;m.identification.start(77,100);
        m.instance.success=true;m.instance.response={0x6a,0x82};
        assert(m.processNfcTag(m.instance,uid,0x20)); // UID cannot escape a HomeKey identification session.
        assert(!m.authorizeNfcUid(uid.data(),4,7));
        m.identification.cancel();assert(m.authorizeNfcUid(uid.data(),4,7));
        HomeKey::Mapping reserved;reserved.accessId=42;m.store.mappings.push_back(reserved);m.store.revoked=true;
        assert(m.authorizeNfcUid(uid.data(),4,7)&&openknxAccessControl.reserved.empty());
    }
    std::cout<<"Actual HomeKey hooks: optional selection, UID policy/reservations, stale tokens, ECP gating and startup readiness passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='homekey-nfc-hooks-') as temp:
    src=Path(temp)/'test.cpp';src.write_text(source);binary=str(Path(temp)/'test')
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined','-I'+str(root/'lib/OFM-HomeKey/src'),str(src),'-o',binary],check=True)
    subprocess.run([binary],check=True)
