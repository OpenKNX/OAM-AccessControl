"""Exercise the actual shared worker, queue and lifecycle without HomeKey."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parents[2]
cpp=(root/'lib/OFM-AccessControl/src/NfcReaderService.cpp').read_text()
cpp=re.sub(r'^#include "(?:AccessControl.h|Pn7161ReaderAdapter.hpp|St25r3916Reader.hpp)"\n','',cpp,flags=re.M)
access=(root/'lib/OFM-AccessControl/src/AccessControl.cpp').read_text()
loop_start=access.index('void AccessControl::loop()')
loop_start=access.index('{',loop_start)+1
loop_prefix=access[loop_start:access.index('    if (!isLocked)',loop_start)]
rtos=r'''
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <deque>
#include <vector>
using TickType_t=uint32_t;using TaskHandle_t=void*;using QueueHandle_t=void*;
constexpr int pdPASS=1,pdTRUE=1;
#define pdMS_TO_TICKS(n) (n)
struct Stop {};
inline uint32_t ticks=0,budget=60;
inline bool taskOk=true,queueOk=true;
inline unsigned tasks=0;
inline void(*worker)(void*)=nullptr;inline void* argument=nullptr;
inline TickType_t xTaskGetTickCount(){return ticks;}
inline void vTaskDelay(uint32_t n){ticks+=n;if(ticks>budget)throw Stop{};}
inline unsigned uxTaskGetStackHighWaterMark(TaskHandle_t){return 1024;}
inline int xTaskCreate(void(*f)(void*),const char*,int,void* arg,int,TaskHandle_t* out){
    ++tasks;if(!taskOk)return 0;worker=f;argument=arg;*out=reinterpret_cast<void*>(1);return pdPASS;
}
inline struct Queue {size_t width=0,limit=0;std::deque<std::vector<uint8_t>> items;} queue;
inline QueueHandle_t xQueueCreate(size_t limit,size_t width){queue={width,limit,{}};return queueOk?&queue:nullptr;}
inline int xQueueSend(QueueHandle_t, const void* data,int){
    if(queue.items.size()>=queue.limit)return 0;
    queue.items.emplace_back(static_cast<const uint8_t*>(data),static_cast<const uint8_t*>(data)+queue.width);return pdTRUE;
}
inline int xQueueReceive(QueueHandle_t,void* out,int){
    if(queue.items.empty())return 0;std::memcpy(out,queue.items.front().data(),queue.width);queue.items.pop_front();return pdTRUE;
}
'''
source=r'''
#include "NfcReaderService.h"
#include <cassert>
#include <iostream>
#include <new>
#define logInfo(...) ((void)0)
#define logError(...) ((void)0)
#define logWarning(...) ((void)0)
int ParamACC_NfcScanner=2;
struct {bool configuredValue=true;bool configured()const{return configuredValue;}} knx;
struct {
    unsigned uidCalls=0,rejected=0;bool grant=true;
    bool processExternalNfcUid(const uint8_t* uid,uint8_t len,const uint16_t* reserved,size_t count){
        assert(len==4&&uid[0]==1&&reserved==nullptr&&count==0);++uidCalls;return grant;
    }
    void rejectExternalCredential(){++rejected;}
}openknxAccessControl;
int sharedWire;
#define OPENKNX_GPIO_WIRE sharedWire
#define OPENKNX_GPIO_SDA 21
#define OPENKNX_GPIO_SCL 22
#define OPENKNX_GPIO_CLOCK 400000
#define NFC_IRQ_PIN 14
#define NFC_VEN_PIN 13
#define NFC_PN7160_ADDR 0x28
struct Pn7161Reader {struct Config {int* wire;int sda,scl,irq,ven,dwl;uint8_t address;uint32_t clock;uint16_t vantMv;};};
unsigned inits=0,stops=0,polls=0,updates=0;bool initOk=true,healthOk=true;
void(*onPoll)()=nullptr;
class Pn7161ReaderAdapter:public INfcReader {
public:
    Pn7161ReaderAdapter(const Pn7161Reader::Config& c,const std::array<uint8_t,18>&){assert(c.sda==21&&c.scl==22);}
    bool init()override{++inits;return initOk;}
    void stop()override{++stops;}
    bool isConnected()const override{return true;}
    uint8_t getFwMajor()const override{return 1;}
    uint8_t getFwMinor()const override{return 0;}
    bool beginDiscovery()override{return true;}
    bool pollForTag(std::vector<uint8_t>& uid,std::array<uint8_t,2>&,uint8_t& sak,uint32_t)override{
        ++polls;if(onPoll)onPoll();uid={1,2,3,4};sak=0;return polls==1;
    }
    bool isTagStillPresent()override{return false;}
    void releaseTag()override{}
    void endDiscovery()override{}
    bool exchangeApdu(const std::vector<uint8_t>&,std::vector<uint8_t>&,uint32_t)override{assert(false);return false;}
    bool healthCheck()override{return healthOk;}
    bool updateECP()override{++updates;return true;}
};
'''+cpp+r'''
struct DelayLoop {bool delayCallbackActive=true;unsigned continued=0;void loop(){
__ACCESS_DELAY_PREFIX__
++continued;}};
struct Extension:public NfcReaderExtension {
    bool ready=true,consume=false,allow=true,advertise=false;uint32_t token=7;unsigned tags=0,uidCalls=0;
    bool nfcReady()const override{return ready;}
    bool prepareNfc(std::array<uint8_t,18>& frame,uint32_t& out)override{frame.fill(0);if(advertise)frame[0]=0x6a;out=token;return true;}
    bool processNfcTag(INfcReader&,const std::vector<uint8_t>&,uint8_t)override{++tags;return consume;}
    bool authorizeNfcUid(const uint8_t*,uint8_t,uint32_t captured)override{++uidCalls;return allow&&captured==token;}
};
void reset(){
    openknxNfc.~NfcReaderService();new(&openknxNfc)NfcReaderService;
    ticks=0;tasks=0;taskOk=queueOk=initOk=healthOk=true;worker=nullptr;onPoll=nullptr;
    inits=stops=polls=updates=0;ParamACC_NfcScanner=2;knx.configuredValue=true;
    openknxAccessControl.uidCalls=openknxAccessControl.rejected=0;
    openknxAccessControl.grant=true;
}
void run(){try{assert(worker);worker(argument);}catch(Stop&){} }
Extension* changingExtension=nullptr;
void changeEcp(){if(polls==1)changingExtension->advertise=true;}
int main(){
    for(int scanner:{1,2}) {
        reset();ParamACC_NfcScanner=scanner;openknxNfc.setup();openknxNfc.loop();run();openknxNfc.loop();
        assert(inits==1&&openknxAccessControl.uidCalls==1&&updates==0&&tasks==1);
    }
    reset();ParamACC_NfcScanner=0;openknxNfc.setup();openknxNfc.loop();assert(!openknxNfc.active()&&tasks==0);
    reset();knx.configuredValue=false;openknxNfc.setup();openknxNfc.loop();assert(tasks==0);
    reset();ParamACC_NfcScanner=0;openknxNfc.setup();openknxNfc.beginTest();openknxNfc.loop();run();openknxNfc.loop();
    assert(openknxNfc.testTagFound()&&openknxAccessControl.uidCalls==0);openknxNfc.endTest();openknxNfc.loop();assert(!openknxNfc.active());
    reset();queueOk=false;openknxNfc.setup();openknxNfc.loop();assert(!openknxNfc.active()&&tasks==0);
    reset();taskOk=false;openknxNfc.setup();openknxNfc.loop();assert(!openknxNfc.active());
    reset();openknxNfc.setup();openknxNfc.loop();run();openknxNfc.suspend();openknxNfc.loop();assert(openknxAccessControl.uidCalls==0);
    reset();healthOk=false;polls=1;openknxNfc.setup();openknxNfc.loop();run();openknxNfc.loop();assert(stops==1&&inits==1&&openknxAccessControl.uidCalls==0);
    reset();initOk=false;openknxNfc.setup();openknxNfc.loop();run();openknxNfc.loop();assert(stops==1&&openknxAccessControl.uidCalls==0);
    reset();{openknxNfc.setup();openknxNfc.loop();run();
        DelayLoop access;openknxAccessControl.grant=false;access.loop();
        assert(openknxAccessControl.uidCalls==1&&openknxAccessControl.rejected==1&&access.continued==0);
        access.delayCallbackActive=false;openknxAccessControl.grant=true;access.loop();
        assert(openknxAccessControl.uidCalls==1&&access.continued==1); // No queued grant after delay expires.
    }
    reset();{Extension ext;ext.ready=false;openknxNfc.attach(&ext);openknxNfc.setup();openknxNfc.loop();assert(tasks==0);
        ext.ready=true;openknxNfc.loop();run();openknxNfc.loop();assert(ext.tags==1&&ext.uidCalls==1&&openknxAccessControl.uidCalls==0);}
    reset();{Extension ext;ext.consume=true;openknxNfc.attach(&ext);openknxNfc.setup();openknxNfc.loop();run();openknxNfc.loop();assert(ext.tags==1&&ext.uidCalls==0);}
    reset();{Extension ext;openknxNfc.attach(&ext);openknxNfc.setup();openknxNfc.loop();run();++ext.token;openknxNfc.loop();assert(ext.uidCalls==1&&openknxAccessControl.rejected==1);}
    reset();{Extension ext;openknxNfc.attach(&ext);openknxNfc.setup();openknxNfc.loop();run();ext.allow=false;openknxNfc.loop();assert(openknxAccessControl.rejected==1);}
    reset();{Extension ext;changingExtension=&ext;onPoll=changeEcp;openknxNfc.attach(&ext);openknxNfc.setup();openknxNfc.loop();run();assert(inits==1&&updates==1);}
    reset();std::cout<<"Shared NFC service: no-extension UID, consumed authentication, readiness, stale epochs/tokens and allocation/retry failures passed\n";
}
'''
source=source.replace('__ACCESS_DELAY_PREFIX__',loop_prefix)
with tempfile.TemporaryDirectory(prefix='nfc-service-') as temp:
    path=Path(temp);(path/'freertos').mkdir()
    (path/'freertos/FreeRTOS.h').write_text(rtos)
    for name in ['task.h','queue.h']:(path/'freertos'/name).write_text('#pragma once\n#include "FreeRTOS.h"\n')
    src=path/'test.cpp';src.write_text(source);binary=str(path/'test')
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined','-DOPENKNX_ACC_NFC_READER','-DARDUINO_ARCH_ESP32','-DOPENKNX_NFC_READER_I2C','-I'+temp,'-I'+str(root/'lib/OFM-AccessControl/src'),str(src),'-o',binary],check=True)
    subprocess.run([binary],check=True)
