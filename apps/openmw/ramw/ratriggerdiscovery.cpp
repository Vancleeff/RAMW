#include "ratriggerdiscovery.hpp"
#include "raachievement.hpp"
#include "rastats.hpp"

namespace RAMW
{
    bool RATriggerDiscovery::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        return RAStats::get(obj.stat_name) >= obj.amount;
    }
}
