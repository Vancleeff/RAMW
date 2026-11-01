#include "ratriggers.hpp"
#include "raachievement.hpp"

namespace RAMW
{
    bool RATrigger::evaluate(const RATriggerContext& ctx, const AchievementData& info) const
    {
        for (const auto& obj : info.objectives)
        {
            const bool done = info.isObjectiveDone(obj);
            if (!done)
                return false;
        }
        return true;
    }

    int RATrigger::getProgress(const AchievementData& info) const
    {
        return info.tracked_stat.empty() ? 0 : RAStats::get(info.tracked_stat);
    }
}
