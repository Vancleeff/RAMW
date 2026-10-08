#ifndef RAMW_RATRIGGERGLOBAL_HPP
#define RAMW_RATRIGGERGLOBAL_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerGlobal : public RATrigger
    {
    public:
        static constexpr const char* Prefix = "global_";

        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        std::string getType() const override { return "Global"; }
    };
}

#endif
