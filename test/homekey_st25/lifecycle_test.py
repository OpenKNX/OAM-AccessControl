"""Exercise the actual module loop/event dispatch with a deterministic RTOS queue."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[2]
cpp=(root/'lib/OFM-HomeKey/src/HomeKeyModule.cpp').read_text()
def method(signature):
    start=cpp.index(signature);end=cpp.index('{',start)+1;depth=1
    while depth:
        depth+=(cpp[end]=='{')-(cpp[end]=='}');end+=1
    return cpp[start:end]
source=r'''
#include "HomeKeyPolicy.h"
#include <atomic>
#include <mutex>
#include <deque>
#include <cassert>
#include <iostream>
#include <algorithm>
enum class EventKind {Denied,Uid,Authenticated,ReaderReady,ReaderFailed,TagDetected};
struct TestEvent {
    EventKind kind=EventKind::Denied; const char* reason="denied";
    HomeKey::IssuerId issuer{}; HomeKey::EndpointId endpoint{};
    uint32_t generation=1; uint8_t uid[10]={1,2,3,4};uint8_t uidLength=4;
};
using TaskHandle_t=void*;
unsigned hapTasks=0,nfcTasks=0;
constexpr int pdPASS=1,pdTRUE=1;
#define pdMS_TO_TICKS(n) (n)
#define OPENKNX_HOMEKEY_HAP_STACK_SIZE 8192
#define OPENKNX_HOMEKEY_NFC_STACK_SIZE 8192
void vTaskDelay(int) {}
int xTaskCreate(void(*)(void*),const char* name,int,void*,int,TaskHandle_t* handle) {
    if(std::string(name)=="HomeKeyHap")++hapTasks;else ++nfcTasks;
    *handle=reinterpret_cast<void*>(1);return pdPASS;
}
int xQueueReceive(void* ptr,TestEvent* e,int) {
    auto& q=*static_cast<std::deque<TestEvent>*>(ptr);
    if(q.empty())return 0;*e=q.front();q.pop_front();return pdTRUE;
}
int xQueueSend(void* ptr,const TestEvent* e,int) {static_cast<std::deque<TestEvent>*>(ptr)->push_back(*e);return pdTRUE;}
uint32_t millis() {return 100;}
#define logInfoP(...) ((void)0)
#define logErrorP(...) ((void)0)
#define logWarningP(...) ((void)0)
bool ParamHK_Enabled=false,ParamHK_AllowUid=true,ParamHK_PairingPermitted=false,ParamHK_EtsAuthorization=false;
unsigned ParamACC_NfcScanner=2;
constexpr unsigned HK_Authorization01Enabled=1,HK_Authorization02Enabled=34;
struct {bool configured(){return true;}const uint8_t* paramData(unsigned){static uint8_t data[528]={};return data;}}knx;
struct {
    unsigned uid=0,hk=0,rejected=0;std::vector<uint16_t> reserved;
    bool processExternalNfcUid(const uint8_t*,uint8_t,const uint16_t* slots,size_t size) {++uid;reserved.assign(slots,slots+size);return true;}
    bool processHomeKeyCredential(uint16_t) {++hk;return true;}
    void rejectExternalCredential(){++rejected;}
}openknxAccessControl;
struct Provisioning {void begin(){}void poll(){}};
class HomeKeyModule {
public:
    using Event=TestEvent;
    struct Store {
        bool healthy=true;uint32_t generation=1;std::recursive_mutex mutex;
        std::vector<HomeKey::Mapping> mappings;
        bool contains(const HomeKey::IssuerId&,const HomeKey::EndpointId&) {return true;}
    }store;
    std::deque<Event> queue;
    void* events=&queue;
    Provisioning instance;Provisioning* provisioning=&instance;
    bool enabled=false,readerEnabled=false,readerFatal=false,fatal=false,policyLoaded=true;
    bool webCodeAllowed=false,etsAuthorization=false,etsTableValid=true,endpointDirty=false;
    bool allowUid=false,pairingPermitted=false,started=false,provisioningReady=false;
    int reportedAuthorization=0,reportedCommissioning=0,reportedResult=0;
    TaskHandle_t readerHandle=nullptr,hapHandle=nullptr;
    std::vector<HomeKey::Mapping> etsMappings;
    std::atomic<uint32_t> pairingUntil=0;
    const auto& activeMappings() const {return etsAuthorization?etsMappings:store.mappings;}
    uint16_t lookupAccessId(const HomeKey::IssuerId&,const HomeKey::EndpointId&) {return 0;}
    void cancelCommissioning() {pairingUntil=0;}
    void updateStatus() {}
    void readerTask() {}
    void loop();void enqueue(const Event&);
};
__ACTUAL__
void reset(){hapTasks=nfcTasks=0;openknxAccessControl.uid=openknxAccessControl.hk=openknxAccessControl.rejected=0;
    ParamHK_Enabled=false;ParamHK_AllowUid=true;ParamACC_NfcScanner=2;}
int main(){
    reset();{
        HomeKeyModule m;HomeKeyModule::Event e;e.kind=EventKind::Uid;
        m.loop();m.enqueue(e);m.loop();
        assert(nfcTasks==1&&hapTasks==0&&openknxAccessControl.uid==1&&!m.enabled);
    }
    reset();{
        HomeKeyModule m;m.store.healthy=false;ParamHK_Enabled=true;
        m.loop();TestEvent e;e.kind=EventKind::Uid;m.enqueue(e);m.loop();
        assert(nfcTasks==1&&hapTasks==0&&openknxAccessControl.uid==1&&!m.enabled);
    }
    reset();{
        HomeKeyModule m;ParamHK_Enabled=true;m.loop();
        assert(hapTasks==1&&nfcTasks==0);
        m.provisioningReady=true;m.loop();assert(nfcTasks==1);
        TestEvent e;e.kind=EventKind::Authenticated;m.enqueue(e);m.loop();assert(openknxAccessControl.hk==1);
        ParamHK_Enabled=false;m.enqueue(e);m.loop();
        assert(openknxAccessControl.hk==1&&openknxAccessControl.rejected==1); // No queued grant after disable.
        e.kind=EventKind::Uid;m.enqueue(e);m.loop();assert(openknxAccessControl.uid==1); // UID reader survives.
    }
    reset();{
        HomeKeyModule m;ParamACC_NfcScanner=0;m.loop();TestEvent e;e.kind=EventKind::Uid;m.enqueue(e);
        assert(nfcTasks==0&&m.queue.empty());
    }
    reset();{
        HomeKeyModule m;m.loop();TestEvent e;e.kind=EventKind::Uid;
        ParamHK_AllowUid=false;m.enqueue(e);m.loop();assert(openknxAccessControl.uid==0);
        ParamHK_AllowUid=true;e.generation=0;m.enqueue(e);m.loop();assert(openknxAccessControl.uid==0);
        e.generation=1;HomeKey::Mapping slot;slot.accessId=100;m.store.mappings.push_back(slot);
        m.enqueue(e);m.loop();assert(openknxAccessControl.uid==1&&openknxAccessControl.reserved==std::vector<uint16_t>({100}));
    }
    std::cout<<"Actual NFC lifecycle: standalone UID, HAP gating, storage failure, disable/re-enable and stale-event checks passed\n";
}
'''
source=source.replace('__ACTUAL__',method('void HomeKeyModule::loop()')+'\n'+method('void HomeKeyModule::enqueue('))
with tempfile.TemporaryDirectory(prefix='oam-nfc-lifecycle-') as tmp:
    src=Path(tmp)/'test.cpp';src.write_text(source);exe=Path(tmp)/'test'
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined','-I'+str(root/'lib/OFM-HomeKey/src'),str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
