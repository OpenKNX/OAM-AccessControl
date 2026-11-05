"""Compile and exercise the real cooperative service with no FreeRTOS headers."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parents[2]
cpp=(root/'lib/OFM-AccessControl/src/NfcReaderService.cpp').read_text()
cpp=re.sub(r'^#include "(?:AccessControl.h|Pn7160ReaderAdapter.hpp|Pn7161ReaderAdapter.hpp|St25r3916Reader.hpp)"\n','',cpp,flags=re.M)
source=r'''
#include "NfcReaderService.h"
#include <cassert>
#include <iostream>
#include <new>
#define logInfo(...) ((void)0)
#define logError(...) ((void)0)
#define logWarning(...) ((void)0)
#define HK_Enabled 1
bool ParamHK_Enabled=false;
int ParamACC_NfcScanner=2;
struct {bool configuredValue=true;bool configured()const{return configuredValue;}}knx;
uint32_t now=0;
uint32_t millis(){return now;}
unsigned inits=0,stops=0,polls=0;bool initOk=true,healthy=true,emit=false,ready=false;
size_t uidLength=4;
void(*onPoll)()=nullptr;
struct {unsigned calls=0,denied=0;bool grant=true;
    bool processExternalNfcUid(const uint8_t*,uint8_t length,const uint16_t*,size_t){assert(length==uidLength);++calls;return grant;}
    void rejectExternalCredential(){++denied;}
}openknxAccessControl;
#define NFC_IRQ_PIN 16
#define NFC_VEN_PIN 17
#define NFC_PN7160_ADDR 0x28
struct PN7160Reader {struct Config {int irq,ven,dwl;uint8_t address;};};
class Pn7160ReaderAdapter:public INfcReader {
public:
    Pn7160ReaderAdapter(const PN7160Reader::Config& c,const std::array<uint8_t,18>&){assert(c.irq==16&&c.ven==17);}
    bool init()override{++inits;return initOk;}
    void stop()override{++stops;}
    bool beginDiscovery()override{return true;}
    bool isConnected()const override{return true;}
    bool discoveryReady()const override{return ready;}
    uint8_t getFwMajor()const override{return 0;}
    uint8_t getFwMinor()const override{return 0;}
    bool pollForTag(std::vector<uint8_t>& uid,std::array<uint8_t,2>&,uint8_t& sak,uint32_t timeout)override{
        assert(timeout==0);++polls;sak=0;uid.assign(uidLength,1);if(onPoll)onPoll();const bool found=emit;emit=false;return found;
    }
    bool isTagStillPresent()override{assert(false);return false;} // Cooperative polling never blocks for removal.
    void releaseTag()override{}
    void endDiscovery()override{}
    bool healthCheck()override{return healthy;}
    bool updateECP()override{return true;}
    bool exchangeApdu(const std::vector<uint8_t>&,std::vector<uint8_t>&,uint32_t)override{assert(false);return false;}
};
'''+cpp+r'''
void reset(){
    openknxNfc.~NfcReaderService();new(&openknxNfc)NfcReaderService;
    now=0;inits=stops=polls=0;initOk=healthy=true;emit=ready=false;onPoll=nullptr;
    uidLength=4;ParamACC_NfcScanner=2;knx.configuredValue=true;ParamHK_Enabled=false;
    openknxAccessControl.calls=openknxAccessControl.denied=0;openknxAccessControl.grant=true;
}
void invalidate(){openknxNfc.suspend();}
struct Extension:public NfcReaderExtension {
    bool ready=true,consumed=false,allow=true;
    bool nfcReady()const override{return ready;}
    bool prepareNfc(std::array<uint8_t,18>& frame,uint32_t& token)override{frame.fill(0);token=4;return true;}
    bool processNfcTag(INfcReader&,const std::vector<uint8_t>&,uint8_t)override{return consumed;}
    bool authorizeNfcUid(const uint8_t*,uint8_t,uint32_t token)override{assert(token==4);return allow;}
};
int main(){
    for(int scanner:{1,2})for(size_t length:{4,7,10}){
        reset();ParamACC_NfcScanner=scanner;uidLength=length;openknxNfc.setup();
        openknxNfc.loop();assert(inits==1&&polls==1&&now==0);
        emit=ready=true;openknxNfc.loop();assert(openknxAccessControl.calls==1&&now==0);
        openknxNfc.loop();assert(openknxAccessControl.calls==1); // Adapter controls repeat suppression.
        ParamACC_NfcScanner=0;openknxNfc.loop();assert(!openknxNfc.active()&&stops==1);
    }
    for(size_t length:{0,3,5,11}){reset();uidLength=length;emit=true;openknxNfc.setup();openknxNfc.loop();assert(openknxAccessControl.calls==0);}
    reset();ParamHK_Enabled=true;openknxNfc.setup();openknxNfc.loop();assert(inits==0&&!openknxNfc.active());
    reset();initOk=false;openknxNfc.setup();openknxNfc.loop();openknxNfc.loop();assert(inits==1&&polls==0);
    now=4999;openknxNfc.loop();assert(inits==1);now=5000;initOk=true;openknxNfc.loop();assert(inits==2&&polls==1);
    reset();emit=true;onPoll=invalidate;openknxNfc.setup();openknxNfc.loop();assert(openknxAccessControl.calls==0);
    reset();healthy=false;openknxNfc.setup();openknxNfc.loop();assert(stops==1);openknxNfc.loop();assert(inits==1);
    reset();knx.configuredValue=false;ParamACC_NfcScanner=0;openknxNfc.setup();openknxNfc.beginTest();
    emit=true;openknxNfc.loop();assert(openknxNfc.testTagFound()&&openknxAccessControl.calls==0);
    openknxNfc.endTest();openknxNfc.loop();assert(!openknxNfc.active());
    reset();{Extension e;e.ready=false;openknxNfc.attach(&e);openknxNfc.setup();openknxNfc.loop();assert(inits==0);
        e.ready=true;emit=true;openknxNfc.loop();e.ready=false;openknxNfc.loop();assert(stops==1);}
    reset();std::cout<<"Cooperative shared service: nonblocking UID flow, diagnostic isolation, retries, disabled scanner and stale-result rejection passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='nfc-cooperative-') as temp:
    src=Path(temp)/'test.cpp';src.write_text(source);binary=str(Path(temp)/'test')
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined','-I'+str(root/'lib/OFM-AccessControl/src'),str(src),'-o',binary],check=True)
    subprocess.run([binary],check=True)
