#include "ratriggersoultrap.hpp"
#include "raachievement.hpp"
#include "rastats.hpp"

namespace
{
    int sumSouls(const RAMW::AchievementData& info)
    {
        int total = 0;
        for (const auto& obj : info.objectives)
            total += RAMW::RAStats::get(obj.stat_name);
        return total;
    }
}

namespace RAMW
{
    RATriggerSoulTrap::RATriggerSoulTrap(bool sum)
        : mSum(sum)
    {
    }

    bool RATriggerSoulTrap::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        return obj.matches(RAStats::get(obj.stat_name));
    }

    bool RATriggerSoulTrap::evaluate(const RATriggerContext& ctx, const AchievementData& info) const
    {
        if (!mSum)
            return RATrigger::evaluate(ctx, info);

        return sumSouls(info) >= info.target_value;
    }

    int RATriggerSoulTrap::getProgress(const AchievementData& info) const
    {
        return mSum ? sumSouls(info) : RATrigger::getProgress(info);
    }
}
