#include "ratriggerkill.hpp"
#include "raachievement.hpp"
#include "rastats.hpp"

namespace
{
    int sumKills(const RAMW::AchievementData& info)
    {
        int total = 0;
        for (const auto& obj : info.objectives)
            total += RAMW::RAStats::get(obj.stat_name);
        return total;
    }
}

namespace RAMW
{
    RATriggerKill::RATriggerKill(bool sum)
        : mSum(sum)
    {
    }

    bool RATriggerKill::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        return obj.matches(RAStats::get(obj.stat_name));
    }

    // Sum mode counts every listed creature towards a single target.
    bool RATriggerKill::evaluate(const RATriggerContext& ctx, const AchievementData& info) const
    {
        if (!mSum)
            return RATrigger::evaluate(ctx, info);

        return sumKills(info) >= info.target_value;
    }

    int RATriggerKill::getProgress(const AchievementData& info) const
    {
        return mSum ? sumKills(info) : RATrigger::getProgress(info);
    }
}
