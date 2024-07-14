#include "OpenKNX.h"
#include "Fingerprint.h"

class ActionChannel : public OpenKNX::Channel
{
    private:
        uint32_t _actionCallResetTime = 0;
        uint32_t _stairLightTime = 0;
    
        inline static bool _authenticateActive = false;

    public:
        ActionChannel(uint8_t index, Fingerprint finger);
        const std::string name() override;
        Fingerprint _finger;

        void loop();
        void processInputKo(GroupObject &ko) override;
        bool processScan(uint16_t location);
};