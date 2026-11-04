#include <ST25R3916.hpp>
#include "St25r3916Reader.hpp"
#include <cassert>
#include <iostream>

int main() {
    TwoWire wire;
    std::array<uint8_t,18> frame{};
    St25r3916Reader adapter(wire,21,22,frame);
    INfcReader& reader=adapter;
    assert(reader.init() && reader.beginDiscovery());
    std::vector<uint8_t> uid; std::array<uint8_t,2> atqa{}; uint8_t sak=0;
    reader.pollForTag(uid,atqa,sak,20);
    for(const auto& sent:sentFrames) assert(sent.empty() || sent[0]!=0x6a);
    frame={0x6a,2,0xcb,2,6,2,0x11,0,1,2,3,4,5,6,7,8,9,10};
    assert(reader.updateECP());
    auto start=sentFrames.size();
    reader.pollForTag(uid,atqa,sak,20);
    assert(sentFrames[start]==std::vector<uint8_t>(frame.begin(),frame.end()));
    frame.fill(0);
    assert(reader.updateECP());
    start=sentFrames.size();
    reader.pollForTag(uid,atqa,sak,20);
    for(auto i=start;i<sentFrames.size();++i)assert(sentFrames[i].empty()||sentFrames[i][0]!=0x6a);
    reader.stop();
    assert(wire.liveDevices==0);
    std::cout << "HomeKey adapter: ECP enable/disable and shared-bus cleanup passed\n";
}
