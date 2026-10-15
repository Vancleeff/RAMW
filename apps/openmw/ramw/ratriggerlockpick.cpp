#include "ratriggerlockpick.hpp"
#include "raachievement.hpp"

namespace RAMW
{
    bool RATriggerLockpick::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        return false;   // no persisted source — evaluated on event only
    }

    bool RATriggerLockpick::evaluate(const RATriggerContext& ctx, const AchievementData& info) const
    {
        for (const auto& obj : info.objectives)
            if (obj.stat_name == ctx.key && obj.matches(ctx.value))
                return true;
        return false;
    }
}
