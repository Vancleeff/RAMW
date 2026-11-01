#include "ratriggerspell.hpp"
#include "raachievement.hpp"
#include "rastats.hpp"

namespace
{
    int sumCasts(const RAMW::AchievementData& info)
    {
        int total = 0;
        for (const auto& obj : info.objectives)
            total += RAMW::RAStats::get(obj.stat_name);
        return total;
    }
}

namespace RAMW
{
    RATriggerSpell::RATriggerSpell(bool sum)
        : mSum(sum)
    {
    }

    bool RATriggerSpell::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        return obj.matches(RAStats::get(obj.stat_name));
    }

    bool RATriggerSpell::evaluate(const RATriggerContext& ctx, const AchievementData& info) const
    {
        if (!mSum)
            return RATrigger::evaluate(ctx, info);

        return sumCasts(info) >= info.target_value;
    }

    int RATriggerSpell::getProgress(const AchievementData& info) const
    {
        return mSum ? sumCasts(info) : RATrigger::getProgress(info);
    }
}
