#include "ratriggerquestprogress.hpp"
#include "raachievement.hpp"

namespace RAMW
{
    bool RATriggerQuestProgress::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        return false;
    }

    bool RATriggerQuestProgress::evaluate(const RATriggerContext& ctx, const AchievementData& info) const
    {
        for (const auto& obj : info.objectives)
            if (obj.stat_name == ctx.key && obj.matches(ctx.value))
                return true;
        return false;
    }
}
