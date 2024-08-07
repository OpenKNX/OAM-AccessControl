#include "FingerprintModule.h"
#include "SwitchActuatorModule.h"
#include "Logic.h"
#include "GpioBinaryInputModule.h"
#include "VirtualButtonModule.h"
#include "FileTransferModule.h"

void setup()
{
    const uint8_t firmwareRevision = 4;
    openknx.init(firmwareRevision);
    openknx.addModule(1, openknxLogic);
    openknx.addModule(2, openknxFingerprintModule);
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