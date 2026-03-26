#include "AccessControl.h"
#include "SwitchActuatorModule.h"
#include "Logic.h"
#include "GpioBinaryInputModule.h"
#include "VirtualButtonModule.h"
#include "FileTransferModule.h"

void setup()
{
    openknx.init();
    openknx.addModule(1, openknxLogic);
    openknx.addModule(2, openknxAccessControl);
    openknx.addModule(3, openknxVirtualButtonModule);
    openknx.addModule(4, openknxSwitchActuatorModule);
    openknx.addModule(5, openknxGpioBinaryInputModule);
    openknx.addModule(9, openknxFileTransferModule);
    openknx.setup();
}

void loop()
{
    openknx.loop();
}