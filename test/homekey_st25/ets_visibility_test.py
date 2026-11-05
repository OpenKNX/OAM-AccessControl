"""Evaluate HomeKey ETS visibility and exercise actual KNX input/status methods."""
from pathlib import Path
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[2]
ns = '{http://knx.org/xml/project/20}'
hk = ET.parse(root/'lib/OFM-HomeKey/src/HomeKey.share.xml').getroot()
raw_ref = '%AID%_UP-2000011_R-200001101'
enabled_ref = '%AID%_UP-%TT%00001_R-%TT%0000101'
uid_ref = '%AID%_UP-%TT%00003_R-%TT%0000301'
dynamic = hk.find('.//'+ns+'Dynamic')
def visible(node, values):
    tag = node.tag.removeprefix(ns)
    if tag == 'choose':
        value = values.get(node.attrib['ParamRefId'], 0)
        return [item for branch in node if branch.attrib.get('test') == str(value)
                for child in branch for item in visible(child, values)]
    return [node] + [item for child in node for item in visible(child, values)]
for enabled in (0, 1):
    for raw in (0, 1):
        nodes = visible(dynamic, {enabled_ref: enabled, raw_ref: raw, '%AID%_UP-%TT%00004_R-%TT%0000401': 1})
        objects = [n.attrib['RefId'] for n in nodes if n.tag == ns+'ComObjectRefRef']
        assert len(objects) == (0 if not enabled else 9 if raw else 1), (enabled,raw,objects)
        assert sum(n.tag == ns+'ParameterRefRef' and n.attrib['RefId']==uid_ref for n in nodes)==enabled
        if enabled:
            assert objects[0].endswith('00001_R-%TT%0000101')
            assert any(n.attrib.get('Name')=='importEndpoints' for n in nodes)
        print(f'ETS HomeKey={enabled}, raw={raw}: UID default/mode controls correct, {len(objects)} HomeKey objects')
# The external reference must resolve to the ACC raw-data parameter, not another switch.
acc = ET.parse(root/'lib/OFM-AccessControl/src/AccessControl.share.xml').getroot()
param = next(n for n in acc.iter(ns+'Parameter') if n.attrib.get('Name')=='EnableRawData')
refs = [n.attrib['Id'].replace('%T%', '2') for n in acc.iter(ns+'ParameterRef') if n.attrib['RefId']==param.attrib['Id']]
assert raw_ref in refs

if len(sys.argv) > 1:
    # Also check producer-expanded IDs and object references, when supplied.
    expanded = ET.parse(sys.argv[1]).getroot()
    parameters = {n.attrib['Name']: n.attrib['Id'] for n in expanded.iter(ns+'Parameter')}
    refs = {n.attrib['RefId']: n.attrib['Id'] for n in expanded.iter(ns+'ParameterRef')}
    block = next(n for n in expanded.iter(ns+'ParameterBlock') if n.attrib.get('Text')=='Apple Home Key')
    objects = {n.attrib['Id']: n.attrib['Name'] for n in expanded.iter(ns+'ComObject')}
    object_refs = {n.attrib['Id']: objects[n.attrib['RefId']] for n in expanded.iter(ns+'ComObjectRef')}
    uid_id = refs[parameters['HK_AllowUid']]
    for enabled in (0, 1):
        for raw in (0, 1):
            values = {refs[parameters['HK_Enabled']]: enabled,
                      refs[parameters['ACC_EnableRawData']]: raw,
                      refs[parameters['HK_EtsAuthorization']]: 1}
            nodes = visible(block, values)
            names = [object_refs[n.attrib['RefId']] for n in nodes if n.tag==ns+'ComObjectRefRef']
            assert len(names)==(0 if not enabled else 9 if raw else 1)
            if enabled and not raw:
                assert names==['HK_LockFeedback']
            assert sum(n.tag==ns+'ParameterRefRef' and n.attrib['RefId']==uid_id for n in nodes)==enabled
    print('Producer-expanded ETS visibility and object references passed')

cpp=(root/'lib/OFM-HomeKey/src/HomeKeyModule.cpp').read_text()
def method(signature):
    start=cpp.index(signature); end=cpp.index('{',start)+1; depth=1
    while depth:
        depth+=(cpp[end]=='{')-(cpp[end]=='}'); end+=1
    return cpp[start:end]
source=r'''
#include <cassert>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <iostream>
bool ParamHK_Enabled=true,ParamACC_EnableRawData=false;
struct {bool configured() const {return true;}} knx;
constexpr int HK_KoLockFeedback=11,HK_KoCommissioningControl=12,HK_KoEndpointIndex=60;
constexpr int DPT_Switch=1,DPT_Value_1_Ucount=5;
struct GroupObject {int id;int asap()const{return id;}int value(int)const{return 1;}};
unsigned feedbacks=0,commands=0,writes=0,endpoints=0;
struct Ko {template<class T>void value(T,int){++writes;}};
Ko KoHK_CommissioningOpen,KoHK_CommissioningResult,KoHK_AuthorizationStatus;
struct Provisioning {void feedback(int){++feedbacks;}};
uint32_t millis(){return 2000;}
struct HomeKeyModule {
    bool started=true,endpointDirty=false;uint8_t endpointIndex=0;
    uint32_t statusAt=0,inspectedGeneration=0;
    int reportedCommissioning=-1,reportedResult=-1,reportedAuthorization=-1;
    std::atomic<int> commissioningResult{2};
    struct {std::recursive_mutex mutex;uint32_t generation=1;} store;
    Provisioning instance;Provisioning* provisioning=&instance;
    int pairingAllowed(){return 0;}
    uint8_t authorizationStatus(){return 2;}
    void requestCommissioning(int){++commands;}
    void publishEndpoint(){++endpoints;endpointDirty=false;inspectedGeneration=store.generation;}
    void processInputKo(GroupObject&);
    void updateStatus();
};
'''
source+='\n'+method('void HomeKeyModule::processInputKo(GroupObject& ko)')+'\n'+method('void HomeKeyModule::updateStatus()')
source+=r'''
int main(){
    HomeKeyModule hk;GroupObject lock{11},pair{12},index{60};
    hk.processInputKo(lock);hk.processInputKo(pair);hk.processInputKo(index);hk.updateStatus();
    assert(feedbacks==1 && commands==0 && writes==0 && endpoints==0);
    ParamACC_EnableRawData=true;
    hk.processInputKo(pair);hk.processInputKo(index);hk.updateStatus();
    assert(commands==1 && writes==3 && endpoints==1);
    ParamACC_EnableRawData=false;hk.updateStatus();hk.processInputKo(pair);
    assert(commands==1 && writes==3 && endpoints==1);
    ParamHK_Enabled=false;ParamACC_EnableRawData=true;
    hk.processInputKo(lock);hk.processInputKo(pair);hk.processInputKo(index);hk.updateStatus();
    assert(feedbacks==1 && commands==1 && writes==3 && endpoints==1);
    ParamHK_Enabled=true;hk.updateStatus();
    assert(writes==6 && endpoints==2);
    std::cout<<"Actual KNX methods: lock feedback, diagnostic gating and fresh status after re-enable passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='homekey-ets-visibility-') as temp:
    src=Path(temp)/'test.cpp';src.write_text(source);binary=str(Path(temp)/'test')
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',str(src),'-o',binary],check=True)
    subprocess.run([binary],check=True)
