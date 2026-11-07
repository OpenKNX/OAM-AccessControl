"""Exercise actual HomeKey setup dispatch and HTTP routes without an ETS application."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[2]
def method(file,signature):
    source=(root/file).read_text()
    start=source.index(signature);end=source.index('{',start)+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
# Braces inside the HTML/raw strings are balanced in this method as well.
source=r'''
#include "HomeKeyPolicy.h"
#include "HomeKeyCodeRequest.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <iostream>
#define OPENKNX_HOMEKEY_WEB
#define MAIN_ApplicationVersion 183
#define BUILD_TIMESTAMP 1
#define logErrorP(...)
#define logWarningP(...)
unsigned queueCreates=0,queued=0;bool queueFails=false;
void* xQueueCreate(unsigned,size_t){++queueCreates;return queueFails?nullptr:reinterpret_cast<void*>(1);}
constexpr int pdTRUE=1;
int xQueueSend(void*,const void*,int){++queued;return pdTRUE;}
int xQueueReceive(void*,void*,int){return 0;}
unsigned millis(){return 100;}
void esp_fill_random(uint8_t* data,size_t count){std::fill(data,data+count,0xab);}
int sodium_memcmp(const void* a,const void* b,size_t size){return std::memcmp(a,b,size);}
void sodium_memzero(void* data,size_t size){std::memset(data,0,size);}
struct {bool ready=false,prog=false;bool configured(){return ready;}bool progMode(){return prog;}}knx;
namespace WebAssets {
const uint8_t homekey_js_gz[]={0},homekey_css_gz[]={0},qrcode_js_gz[]={0};
const char *homekey_js_mime="text/javascript",*homekey_css_mime="text/css",*qrcode_js_mime="text/javascript";
}
namespace OpenKNX {namespace Network {
constexpr int WEB_GET=0,WEB_POST=1;
struct WebRequest {
    std::string data;
    const uint8_t* body(){return reinterpret_cast<const uint8_t*>(data.data());}
    size_t bodyLength(){return data.size();}
};
struct WebResponse {
    int status=200;std::string body;std::map<std::string,std::string> headers;
    void setHeader(const char* key,const char* value){headers[key]=value;}
    void setContentType(const char*){}
    void setLayout(bool){}
    void setActiveMenu(const char*){}
    void setStatus(int value){status=value;}
    void send(const char* value){body=value;}
};
struct HtmlWriter {
    WebResponse& response;HtmlWriter(WebResponse& value):response(value){}
    void operator+=(const char* value){response.body+=value;}
    void operator+=(const std::string& value){response.body+=value;}
};
struct Webserver {
    struct Asset {Asset(const char*,const uint8_t*,size_t){}};
    unsigned menus=0,assets=0;
    std::map<std::pair<int,std::string>,std::function<void(WebRequest&,WebResponse&)>> routes;
    void addMenuItem(const char*,const char*,int){++menus;}
    void addRoute(int,const char*,Asset){++assets;}
    void addRoute(int method,const char* path,std::function<void(WebRequest&,WebResponse&)> handler){assert(!routes.count({method,path}));routes[{method,path}]=handler;}
    WebResponse request(int method,const char* path,const std::string& data=""){
        WebRequest request{data};WebResponse response;
        auto it=routes.find({method,path});if(it==routes.end())response.status=404;else it->second(request,response);return response;
    }
};
}}
using namespace OpenKNX::Network;
struct {Webserver webserver;}openknxNetwork;
struct Provisioning {bool configurePairingCode(const char*,const char*,bool){return true;}};
struct HomeKeyModule {
    unsigned normalSetups=0;bool webInitialized=false;
    void* webCommands=nullptr;std::string webToken;
    bool enabled=false,pairingPermitted=false,provisioningReady=false;
    std::atomic<bool> webCodeAllowed{false},codeConfigured{false};
    std::array<char,5> qrSetupId{{'A','B','C','D',0}};
    std::atomic<uint32_t> codeBlockedUntil{0};
    enum class WebCodeState:uint8_t {Idle,Pending,Saved,Rejected,Failed,WrongCode,RateLimited};
    std::atomic<WebCodeState> webCodeState{WebCodeState::Idle};
    struct WebCodeCommand {char code[9]={},previous[9]={};uint32_t epoch=0;};
    std::atomic<uint32_t> commissioningEpoch{0};
    Provisioning provision;Provisioning* provisioning=&provision;
    bool pairingAllowed()const{return enabled&&pairingPermitted;}
    void setup(bool);
    void setup(){++normalSetups;setupWeb();}
    void setupWeb();void loopWeb();
};
'''
source+='\n'+method('lib/OFM-HomeKey/src/HomeKeyModule.cpp','void HomeKeyModule::setup(bool configured)')
source+='\n'+method('lib/OFM-HomeKey/src/HomeKeyWeb.cpp','void HomeKeyModule::setupWeb()')
source+='\n'+method('lib/OFM-HomeKey/src/HomeKeyWeb.cpp','void HomeKeyModule::loopWeb()')
source+=r'''
int main(){
    HomeKeyModule module;module.setup(false);
    assert(module.normalSetups==0&&queueCreates==1);
    assert(openknxNetwork.webserver.menus==1&&openknxNetwork.webserver.assets==3);
    auto page=openknxNetwork.webserver.request(WEB_GET,"/homekey");
    assert(page.status==200&&page.body.find("homekey-page")!=std::string::npos);
    auto status=openknxNetwork.webserver.request(WEB_GET,"/homekey/status");
    assert(status.status==200&&status.body.find("\"configured\":false")!=std::string::npos);
    assert(status.body.find("\"applicationVersion\":\"11.7\"")!=std::string::npos);
    assert(status.body.find("\"canSetCode\":false")!=std::string::npos);
    assert(status.headers["Cache-Control"]=="no-store");
    // Programming mode alone never authorizes setup-code changes.
    knx.prog=true;module.enabled=module.pairingPermitted=module.provisioningReady=true;
    module.webCodeAllowed=true;
    auto denied=openknxNetwork.webserver.request(WEB_POST,"/homekey/code",module.webToken+"\n\n24681357");
    assert(denied.status==409&&queued==0);
    module.loopWeb();assert(!module.webCodeAllowed);
    status=openknxNetwork.webserver.request(WEB_GET,"/homekey/status");
    assert(status.body.find("\"enabled\":false")!=std::string::npos);
    assert(status.body.find("\"commissioning\":false")!=std::string::npos);
    assert(status.body.find("\"pairing\":false")!=std::string::npos);
    assert(status.body.find("\"setupId\":\"\"")!=std::string::npos);
    // Restored configuration has ordinary behavior and no duplicate route registration.
    knx.ready=true;module.setup(true);module.loopWeb();
    assert(module.normalSetups==1&&queueCreates==1&&module.webCodeAllowed);
    status=openknxNetwork.webserver.request(WEB_GET,"/homekey/status");
    assert(status.body.find("\"configured\":true")!=std::string::npos);
    assert(status.body.find("\"canSetCode\":true")!=std::string::npos);
    assert(openknxNetwork.webserver.request(WEB_POST,"/homekey/code",module.webToken+"\n\n24681357").status==202);
    assert(queued==1);
    // Queue allocation failure must leave diagnostics reachable and code writes rejected.
    openknxNetwork.webserver=Webserver{};queueFails=true;HomeKeyModule failed;failed.setup(false);
    assert(openknxNetwork.webserver.request(WEB_GET,"/homekey/status").status==200);
    assert(openknxNetwork.webserver.request(WEB_POST,"/homekey/code").status==503);
    assert(queued==1);
    std::cout<<"Unconfigured HomeKey HTTP routes, setup lifecycle, write gating and allocation failure passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='homekey-unconfigured-') as temporary:
    temporary=Path(temporary);cpp=temporary/'test.cpp';cpp.write_text(source);binary=temporary/'test'
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined','-I'+str(root/'lib/OFM-HomeKey/src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
