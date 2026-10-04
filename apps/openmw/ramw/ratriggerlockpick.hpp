#ifndef RAMW_RATRIGGERLOCKPICK_HPP
#define RAMW_RATRIGGERLOCKPICK_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerLockpick : public RATrigger
    {
    public:
        bool evaluate(const RATriggerContext& ctx, const AchievementData& info) const override;
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        std::string getType() const override { return "Lockpick"; }
    };
}

#endif
