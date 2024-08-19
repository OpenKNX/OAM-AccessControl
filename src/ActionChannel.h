#include "Fingerprint.h"
#include "OpenKNX.h"

class ActionChannel : public OpenKNX::Channel
{
  private:
    uint32_t _actionCallResetTime = 0;
    uint32_t _stairLightTime = 0;
    bool _readRequestSent = false;

    inline static bool _authenticateActive = false;

  public:
    ActionChannel(uint8_t index, Fingerprint *finger);
    const std::string name() override;
    Fingerprint *_finger = nullptr;

    void loop();
    void processInputKo(GroupObject &ko) override;
    bool processScan(uint16_t location);
    void processReadRequests();
    void resetActionCall();
};