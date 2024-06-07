#include "ActionChannel.h"

ActionChannel::ActionChannel(uint8_t index, Fingerprint finger)
{
    _channelIndex = index;
    _finger = finger;
}

const std::string ActionChannel::name()
{
    return "Action";
}

void ActionChannel::loop()
{
    if (_actionCallResetTime > 0 && delayCheck(_actionCallResetTime, ParamFIN_AuthDelayTimeMS))
    {
        KoFIN_ActionCall.value(false, DPT_Switch);
        _finger.setLed(Fingerprint::State::None);
        _actionCallResetTime = 0;
        _authenticateActive = false;
    }

    if (_stairLightTime > 0 && delayCheck(_stairLightTime, ParamFIN_ActionDelayTimeMS))
    {
        KoFIN_ActionSwitch.value(false, DPT_Switch);
        _stairLightTime = 0;
    }
}

void ActionChannel::processInputKo(GroupObject &ko)
{
    switch (FIN_KoCalcIndex(ko.asap()))
    {
        case FIN_KoActionCall:
            if (ko.value(DPT_Switch))
            {
                _authenticateActive = true;
                _actionCallResetTime = delayTimerInit();
                _finger.setLed(Fingerprint::State::WaitForFinger);
            }
            break;
    }
}

void ActionChannel::processScan(uint16_t location)
{
    if (_authenticateActive && !ParamFIN_ActionAuthenticate)
        return;

    if (!ParamFIN_ActionAuthenticate || KoFIN_ActionCall.value(DPT_Switch))
    {
        switch (ParamFIN_ActionActionType)
        {
            case 0: // action deactivated
                break;
            case 1: // switch
                KoFIN_ActionSwitch.value(ParamFIN_ActionOnOff, DPT_Switch);
                break;
            case 2: // toggle
                KoFIN_ActionSwitch.value(!KoFIN_ActionState.value(DPT_Switch), DPT_Switch);
                KoFIN_ActionState.value(KoFIN_ActionSwitch.value(DPT_Switch), DPT_Switch);
                break;
            case 3: // stair light
                KoFIN_ActionSwitch.value(true, DPT_Switch);
                _stairLightTime = delayTimerInit();
                break;
        }

        if (KoFIN_ActionCall.value(DPT_Switch))
        {
            KoFIN_ActionCall.value(false, DPT_Switch);
            // finger->setLed(Fingerprint::State::None); do not use here
            _actionCallResetTime = 0;
            _authenticateActive = false;
        }
    }
}