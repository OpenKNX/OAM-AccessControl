"""Exercise the real PN7161 adapter's staged and live ECP configuration."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
stub=r'''
#pragma once
#include <array>
#include <vector>
#include <cstdint>
struct Pn7161Reader {
 struct Config {};
 inline static bool connected=false,setOk=true,applyOk=true,found=false,pollError=false;
 inline static unsigned sets=0,applies=0;inline static size_t length=0;
 Pn7161Reader(const Config&){}
 bool setEcpFrame(const uint8_t*,size_t n){++sets;length=n;return setOk;}
 bool updateECP(){++applies;return applyOk;}
 bool init(){connected=true;return true;}void stop(){connected=false;}
 bool isConnected()const{return connected;}
 uint8_t getFwMajor()const{return 1;}uint8_t getFwMinor()const{return 0;}
 bool beginDiscovery(){return true;}
 bool pollForTag(std::vector<uint8_t>& uid,std::array<uint8_t,2>&,uint8_t&,uint32_t){uid={1,2,3,4};if(pollError)connected=false;return found;}
 bool isTagStillPresent(){return false;}void releaseTag(){}void endDiscovery(){}
 bool exchangeApdu(const std::vector<uint8_t>&,std::vector<uint8_t>&,uint32_t){return false;}
 bool healthCheck(){return true;}
};
'''
source=r'''
#include "Pn7161ReaderAdapter.hpp"
#include <cassert>
int main(){
 std::array<uint8_t,18> frame{};Pn7161ReaderAdapter adapter({},frame);
 assert(adapter.capabilities().homeKey()&&adapter.capabilities().uid);
 assert(adapter.init());
 std::vector<uint8_t> uid;std::array<uint8_t,2> atqa{};uint8_t sak=0;
 assert(adapter.pollTag(uid,atqa,sak,20)==NfcPollResult::NoTag);
 Pn7161Reader::found=true;assert(adapter.pollTag(uid,atqa,sak,20)==NfcPollResult::TagFound);
 Pn7161Reader::pollError=true;assert(adapter.pollTag(uid,atqa,sak,20)==NfcPollResult::ReaderError&&uid.empty());
 Pn7161Reader::pollError=false;Pn7161Reader::connected=true;
 assert(Pn7161Reader::sets==1&&Pn7161Reader::applies==0);
 frame[0]=0x6a;assert(adapter.updateECP());assert(Pn7161Reader::length==18&&Pn7161Reader::applies==1);
 frame.fill(0);assert(adapter.updateECP());assert(Pn7161Reader::length==0&&Pn7161Reader::applies==2);
 Pn7161Reader::applyOk=false;assert(!adapter.updateECP());
 Pn7161Reader::setOk=false;assert(!adapter.updateECP());assert(Pn7161Reader::applies==3);
 adapter.stop();Pn7161Reader::setOk=true;assert(adapter.init());assert(Pn7161Reader::applies==3);
}
'''
with tempfile.TemporaryDirectory(prefix='nfc-ecp-adapter-') as temp:
 p=Path(temp);(p/'Pn7161Reader.hpp').write_text(stub);(p/'test.cpp').write_text(source)
 subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined','-I'+temp,'-I'+str(root/'lib/OFM-AccessControl/src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PN7161 adapter: initial staging, live enable/disable, failed apply and restart passed')
