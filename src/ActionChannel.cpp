#include "FingerprintModule.h"

ActionChannel::ActionChannel(uint8_t index, Fingerprint *finger)
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
        resetActionCall();

    if (_stairLightTime > 0 && delayCheck(_stairLightTime, ParamFIN_ActDelayTimeMS))
    {
        KoFIN_ActSwitch.value(false, DPT_Switch);
        _stairLightTime = 0;
    }
    processReadRequests();
}

void ActionChannel::processInputKo(GroupObject &ko)
{
    switch (FIN_KoCalcIndex(ko.asap()))
    {
        case FIN_KoActCallLock:
            if (ParamFIN_ActAuthenticate && ko.value(DPT_Switch))
            {
                _authenticateActive = true;
                _actionCallResetTime = delayTimerInit();
                _finger->setLed(Fingerprint::State::WaitForFinger);
            }
            break;
    }
}

bool ActionChannel::processScan(uint16_t location)
{
    // here are 3 cases relevant:
    // authentication is active and the action is without auth-flag => skip processing
    // authentication is inaction and the action has the auth-flag => skip processing
    // authentication is inaction and the action is locked
    if (_authenticateActive != ParamFIN_ActAuthenticate ||
        !ParamFIN_ActAuthenticate && FIN_KoActCallLock)
        return false;

    if (!ParamFIN_ActAuthenticate || KoFIN_ActCallLock.value(DPT_Switch))
    {
        switch (ParamFIN_ActActionType)
        {
            case 0: // action deactivated
                break;
            case 1: // switch
                KoFIN_ActSwitch.value(ParamFIN_ActOnOff, DPT_Switch);
                break;
            case 2: // toggle
                KoFIN_ActSwitch.value(!KoFIN_ActState.value(DPT_Switch), DPT_Switch);
                KoFIN_ActState.value(KoFIN_ActSwitch.value(DPT_Switch), DPT_Switch);
                break;
            case 3: // stair light
                KoFIN_ActSwitch.value(true, DPT_Switch);
                _stairLightTime = delayTimerInit();
                break;
        }

        if (KoFIN_ActCallLock.value(DPT_Switch))
        {
            KoFIN_ActCallLock.value(false, DPT_Switch);
            _actionCallResetTime = 0;
            _authenticateActive = false;
        }

        return true;
    }

    return false;
}

void ActionChannel::processReadRequests()
{
    // we send a read request just once per channel
    if (_readRequestSent) return;

    // is there a state field to read?
    if (ParamFIN_ActActionType == 2) // toggle
        _readRequestSent = openknxFingerprintModule.sendReadRequest(KoFIN_ActState);
    else
        _readRequestSent = true;
}

void ActionChannel::resetActionCall()
{
    _actionCallResetTime = 0;
    if (!_authenticateActive)
        return;

    KoFIN_ActCallLock.value(false, DPT_Switch);
    _finger->setLed(Fingerprint::State::None);
    _authenticateActive = false;
}