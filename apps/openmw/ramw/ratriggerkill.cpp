#include "ratriggerkill.hpp"
#include "raachievement.hpp"
#include "rastats.hpp"

namespace RAMW
{
    bool RATriggerKill::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        return RAStats::get(obj.stat_name) >= obj.amount;
    }
}
