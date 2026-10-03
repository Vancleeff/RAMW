#include "ratriggers.hpp"
#include "raachievement.hpp"

namespace RAMW
{
    bool RATrigger::evaluate(const RATriggerContext& ctx, const AchievementData& info) const
    {
        for (const auto& obj : info.objectives)
            if (!isObjectiveCompleted(obj))
                return false;
        return true;
    }
}
