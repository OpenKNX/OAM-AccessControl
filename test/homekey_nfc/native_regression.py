"""Compile the actual changed NFC/flash methods with small instrumented dependencies."""
from pathlib import Path
import subprocess
import tempfile
import os

root = Path(__file__).resolve().parents[2]

def method(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

access = (root / 'lib/OFM-AccessControl/src/AccessControl.cpp').read_text()
switch = (root / 'lib/OFM-SwitchActuator/src/SwitchActuatorModule.cpp').read_text()
source = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <vector>
#include <array>
#include <algorithm>
unsigned logs = 0;
#define logInfoP(...) (++logs)
#define logDebugP(...) (++logs)
#define logHexDebugP(...) (++logs)
#define logIndentUp() (++logs)
#define logIndentDown() (++logs)
#define MIN(a,b) std::min(unsigned(a),unsigned(b))
#define bitWrite(value,bit,state) ((value) |= (uint8_t(state) << (bit)))
#define OPENKNX_SWA_CHANNEL_COUNT CHANNELS
#define OPENKNX_SWA_FLASH_VERSION 1
#define OPENKNX_SWA_FLASH_MAGIC_WORD 1234
#define MAX_NFCS 1500
#define ACC_CalcNfcStorageOffset(id) (4097 + (id) * 38)
unsigned ParamSWA_VisibleChannels = 0;
struct Flash {
    std::vector<uint8_t> bytes;
    void writeByte(uint8_t b) { bytes.push_back(b); }
    void writeInt(uint32_t n) { for(int i=0;i<4;i++)writeByte(n>>(i*8)); }
};
struct { Flash flash; } openknx;
struct Channel {
    unsigned calls = 0;
    bool allowed = true, state = true;
    bool processScan(uint16_t) { ++calls; return allowed; }
    bool isRelayActive() { return state; }
};
Channel channelObjects[32];
class SwitchActuatorModule {
public:
    Channel* channel[32];
    SwitchActuatorModule() { for(unsigned i=0;i<32;i++)channel[i]=&channelObjects[i]; }
    void writeFlash(); uint16_t flashSize();
};
struct KO { template<typename... T> void value(T...) {} } KoACC_NfcScanSuccess, KoACC_NfcScanSuccessId;
int DPT_Switch = 0;
int Dpt(int, int) { return 0; }
unsigned ParamNFCACT_NfcActionCount = 0, ParamACC_VisibleActions = 1;
constexpr unsigned ACC_VisibleActions = 4;
constexpr unsigned NFCACT_FaNfcId = 0, NFCACT_FaActionId = 2, NFCACT_ParamBlockOffset = 0, NFCACT_ParamBlockSize = 4;
struct { std::array<uint16_t,16> words{}; uint16_t paramWord(unsigned offset) { return words.at(offset/2); } } knx;
enum class SyncType { NFC };
int delayTimerInit() { return 1; }
class AccessControl {
public:
    Channel* _channels[4] = {&channelObjects[0], nullptr, nullptr, nullptr};
    struct Storage {
        std::vector<uint8_t> bytes = std::vector<uint8_t>(131072);
        unsigned reads = 0;
        void read(unsigned offset, uint8_t* out, unsigned size) { assert(offset+size<=bytes.size()); ++reads; std::memcpy(out,bytes.data()+offset,size); }
    } _nfcStorage;
    int resetTouchPcbLedTimer = 0, resetTouchPcbLedTimerFast = 0;
    unsigned successes = 0;
    void sendScanAccessData(SyncType, bool, uint16_t) { ++successes; }
    void switchLedGreenPower(bool) {}
    void processNfcScanSuccess(uint16_t foundId, bool external);
    void handleFunctionPropertySearchTagByNfcId(uint8_t*,uint8_t*,uint8_t&,bool quiet=false);
};
__METHODS__
int main() {
    SwitchActuatorModule swa;
    for (unsigned visible=0;visible<=CHANNELS;visible++) {
        ParamSWA_VisibleChannels=visible; openknx.flash.bytes.clear(); swa.writeFlash();
        assert(openknx.flash.bytes.size()==swa.flashSize());
        assert(openknx.flash.bytes.size()==6+(visible+7)/8);
        assert(openknx.flash.bytes[5]==visible);
        for (unsigned n=0;n<visible;n++) assert(openknx.flash.bytes[6+n/8]&(1<<(n%8)));
    }
    AccessControl acc;
    // Valid credential slot 0 without an action must not become action 65535.
    ParamNFCACT_NfcActionCount=1; knx.words[0]=0; knx.words[1]=0;
    logs=0; acc.processNfcScanSuccess(0,true);
    assert(acc.successes==1 && channelObjects[0].calls==0 && logs==1);
    knx.words[1]=1; acc.processNfcScanSuccess(0,true);
    assert(channelObjects[0].calls==1);
    // A downloaded reference to a hidden channel must not dereference nullptr.
    knx.words[1]=2; acc.processNfcScanSuccess(0,true);
    assert(channelObjects[0].calls==1);
    ParamACC_VisibleActions=9; knx.words[1]=8;
    acc.processNfcScanSuccess(0,true); // Corrupt parameters cannot exceed compiled capacity.
    // Repeated slot assignments intentionally execute more than one action.
    ParamNFCACT_NfcActionCount=2; ParamACC_VisibleActions=2;
    acc._channels[1]=&channelObjects[1]; knx.words[1]=1; knx.words[2]=0; knx.words[3]=2;
    acc.processNfcScanSuccess(0,true);
    assert(channelObjects[0].calls==2 && channelObjects[1].calls==1);
    uint8_t request[4]={111,0,0,1}, response[40]={}; uint8_t size=0;
    logs=0;
    acc.handleFunctionPropertySearchTagByNfcId(request,response,size,true);
    assert(size==1 && response[0]==1 && logs==0); // Empty UID slot is normal for Home Key.
    acc.handleFunctionPropertySearchTagByNfcId(request,response,size,false);
    assert(logs>0); // Existing legacy diagnostic behavior is preserved.
    acc._nfcStorage.bytes[4097]=0xAB; acc._nfcStorage.bytes[4107]='X';
    logs=0; acc.handleFunctionPropertySearchTagByNfcId(request,response,size,true);
    assert(response[0]==0 && response[1]==0xAB && response[11]=='X' && logs==0);
    puts("Actual NFC slot-0 routing, hidden-channel safety, quiet lookup and relay flash checks passed");
}
'''
source = source.replace('__METHODS__', '\n'.join([
    method(access, 'void AccessControl::processNfcScanSuccess('),
    method(access, 'void AccessControl::handleFunctionPropertySearchTagByNfcId('),
    method(switch, 'void SwitchActuatorModule::writeFlash('),
    method(switch, 'uint16_t SwitchActuatorModule::flashSize('),
]))
with tempfile.TemporaryDirectory(prefix='oam-nfc-regression-') as directory:
    cpp=Path(directory)/'regression.cpp';cpp.write_text(source)
    for channels in [0,1,8,9,16]:
        binary=Path(directory)/str(channels)
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-fsanitize=address,undefined',f'-DCHANNELS={channels}',str(cpp),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
