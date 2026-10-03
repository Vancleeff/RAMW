#include "ratriggerherostats.hpp"
#include "raachievement.hpp"
#include "rastats.hpp"
#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwworld/class.hpp"

namespace RAMW
{
    bool RATriggerHeroStats::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        return false;
    }

    bool RATriggerHeroStats::evaluate(const RATriggerContext& ctx, const AchievementData& info) const
    {
        for (const auto& obj : info.objectives)
            if (obj.stat_name == ctx.key)
                return ctx.value >= obj.amount;
        return false;
    }
}
