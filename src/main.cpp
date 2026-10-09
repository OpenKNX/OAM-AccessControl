#include "AccessControl.h"
#include "SwitchActuatorModule.h"
#include "Logic.h"
#include "GpioBinaryInputModule.h"
#include "VirtualButtonModule.h"
#include "FileTransferModule.h"
#include "Feedback.h"

void setup()
{
    openknx.init();
    openknx.addModule(1, openknxLogic);
    openknx.addModule(2, openknxAccessControl);
    openknx.addModule(3, openknxVirtualButtonModule);
    openknx.addModule(4, openknxSwitchActuatorModule);
#if defined(OPENKNX_BI_GPIO_PINS) && OPENKNX_BI_GPIO_COUNT > 0 && BI_ChannelCount > 0
    openknx.addModule(5, openknxGpioBinaryInputModule);
#endif
    openknx.addModule(6, openknxFeedback);
    openknx.addModule(9, openknxFileTransferModule);
    openknx.setup();
}

void loop()
{
    openknx.loop();
}
