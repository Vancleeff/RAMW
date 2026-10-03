#include "ratriggerquestcomplete.hpp"
#include "raachievement.hpp"
#include "rastats.hpp"

namespace RAMW
{
    bool RATriggerQuestComplete::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        return RAStats::get(obj.stat_name) >= obj.amount;
    }
}
