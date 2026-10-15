#include "ratriggercellclear.hpp"
#include "raachievement.hpp"
#include "rastats.hpp"

namespace RAMW
{
    bool RATriggerCellClear::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        return obj.matches(RAStats::get(obj.stat_name));
    }
}
