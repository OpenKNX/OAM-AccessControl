"""Exercise the real read-only downloaded-permission/action snapshot code."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root/'lib/OFM-HomeKey/src/HomeKeyModule.cpp').read_text()
start = source.index('void HomeKeyModule::readPermissionSnapshot(')
brace = source.index('{', start)
end, depth = brace+1, 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
method = source[start:end]
cpp = r'''
#include "HomeKeyPolicy.h"
#include "HomeKeyOnline.h"
#include "NfcAssignmentSnapshot.h"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
constexpr unsigned HK_Authorization01Enabled=0,HK_Authorization01Description=528,HK_Authorization01DescriptionLength=32;
bool ParamHK_Enabled=true;
std::array<uint8_t,1040> parameters{};
struct {const uint8_t* paramData(unsigned offset){return parameters.data()+offset;}}knx;
struct Store {
    uint32_t generation=1;bool present=true,revoked=false;
    std::vector<HomeKey::Mapping> mappings;
    bool contains(const HomeKey::IssuerId&,const HomeKey::EndpointId&){return present;}
    bool isRevoked(const HomeKey::IssuerId&,const HomeKey::EndpointId&){return revoked;}
};
struct HomeKeyModule {
    Store store;bool policyLoaded=true,etsAuthorization=true,etsTableValid=true;
    void readPermissionSnapshot(uint8_t,uint32_t,uint8_t*,uint8_t&);
};
'''+method+r'''
int main(){
    uint8_t out[256]={},length=0;
    HomeKeyModule module;
    // Slot 1 is a named placeholder. Slot 2 is the one learned phone.
    memcpy(parameters.data()+528,"iPhone Franz",12);
    auto* row=parameters.data()+33;row[0]=0x80;
    memcpy(row+1,"a8587854c92ce68c",16);memcpy(row+18,"f3d38447f0c2",12);
    memcpy(parameters.data()+560,"Downloaded phone",16);
    const auto before=parameters;
    module.readPermissionSnapshot(0,0,out,length);
    assert(out[0]==0 && length==58 && out[2]==0 && out[3]==16 && out[4]==7 && !(out[9]&8));
    const auto revision=HomeKeyOnline::get32(out+5);
    module.readPermissionSnapshot(1,revision,out,length);
    assert(out[0]==0 && length==58 && out[9]==11 && out[10]==0 && out[11]==0);
    assert(out[12]==0xa8 && out[20]==0xf3 && out[25]==0xc2 && !memcmp(out+26,"Downloaded phone",16));
    assert(parameters==before); // Readback must never change downloaded permissions.
    module.readPermissionSnapshot(16,revision,out,length);assert(out[0]==0 && length==9);
    module.readPermissionSnapshot(17,revision,out,length);assert(out[0]==HomeKeyOnline::Stale && length==2);
    parameters[560]='X';module.readPermissionSnapshot(1,revision,out,length);
    assert(out[0]==HomeKeyOnline::Stale && length==2);parameters=before;
    module.store.revoked=true;++module.store.generation;
    module.readPermissionSnapshot(1,revision,out,length);assert(out[0]==HomeKeyOnline::Stale);
    module.readPermissionSnapshot(0,0,out,length);
    module.readPermissionSnapshot(1,HomeKeyOnline::get32(out+5),out,length);assert(out[9]&4);
    module.policyLoaded=false;module.readPermissionSnapshot(0,0,out,length);assert(out[0]==HomeKeyOnline::Busy);
    module.policyLoaded=true;module.etsAuthorization=false;
    module.store.mappings.push_back({{0xa8},{0xf3},42});
    module.readPermissionSnapshot(0,0,out,length);
    assert(out[0]==0 && out[3]==1 && out[4]==5 && out[11]==42);

    std::vector<std::array<uint16_t,2>> actions(310);
    for(unsigned i=0;i<actions.size();++i)actions[i]={uint16_t(i%150+1),uint16_t(i)};
    const auto saved=actions;
    uint8_t request[7]={114,0,0,0,0,0,0};unsigned cursor=0,seen=0;
    uint32_t generation=0;
    for(;;){
        request[1]=cursor>>8;request[2]=cursor;NfcAssignmentSnapshot::put32(request+3,generation);
        NfcAssignmentSnapshot::page(request,out,length,310,150,true,[&](unsigned i,unsigned field){return actions.at(i).at(field);});
        assert(out[0]==0 && out[1]==1 && out[8]<=32 && length==14+out[8]*4 && out[13]==1);
        generation=HomeKeyOnline::get32(out+2);seen+=out[8];
        unsigned next=(unsigned(out[6])<<8)|out[7];
        for(unsigned i=0;i<out[8];++i){unsigned offset=14+i*4;assert(((unsigned(out[offset+2])<<8)|out[offset+3])==cursor+i);}
        if(cursor==310)break;assert(next>cursor);cursor=next;
    }
    assert(seen==310 && actions==saved);
    actions[0][1]=42;
    NfcAssignmentSnapshot::page(request,out,length,310,150,true,[&](unsigned i,unsigned field){return actions.at(i).at(field);});
    assert(out[0]==8 && length==2);
    request[1]=request[2]=0;NfcAssignmentSnapshot::put32(request+3,0);
    NfcAssignmentSnapshot::page(request,out,length,0,0,false,[&](unsigned,unsigned){assert(false);return 0;});
    assert(out[0]==0 && length==14 && out[8]==0 && out[13]==0);
    uint8_t wire[]={11,64,0,0,0,1};HomeKeyOnline::Request decoded;
    assert(HomeKeyOnline::decode(wire,6,decoded)&&decoded.index==64);
    wire[1]=65;assert(!HomeKeyOnline::decode(wire,6,decoded));
    assert(!HomeKeyOnline::decode(wire,5,decoded));
    std::cout<<"Downloaded HomeKey/USB/action snapshots: framing, public data, bounds, stale checks and read-only behavior passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='nfc-device-snapshot-') as temporary:
    path = Path(temporary)
    (path/'test.cpp').write_text(cpp)
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',
        '-I'+str(root/'lib/OFM-HomeKey/src'),'-I'+str(root/'lib/OFM-AccessControl/src'),
        str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
