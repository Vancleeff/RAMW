#include "ratriggerglobal.hpp"
#include "raachievement.hpp"
#include "rastats.hpp"

#include <components/esm/refid.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/globalvariablename.hpp"

#include <cstring>

namespace RAMW
{
    bool RATriggerGlobal::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        // unprefixed names are RAMW stats, global_ reads the engine ones
        if (obj.stat_name.rfind(Prefix, 0) != 0)
            return obj.matches(RAStats::get(obj.stat_name));

        const std::string_view varName = std::string_view(obj.stat_name).substr(std::strlen(Prefix));
        return obj.matches(
            MWBase::Environment::get().getWorld()->getGlobalInt(MWWorld::GlobalVariableName(varName)));
    }

    int RATriggerGlobal::getProgress(const AchievementData& info) const
    {
        // same two sources as isObjectiveCompleted
        if (info.tracked_stat.rfind(Prefix, 0) != 0)
            return RAStats::get(info.tracked_stat);

        const MWWorld::GlobalVariableName name(std::string_view(info.tracked_stat).substr(std::strlen(Prefix)));
        const MWBase::World* world = MWBase::Environment::get().getWorld();

        // Globals::find throws on an unknown name, getType answers with a space
        return world->getGlobalVariableType(name) == ' ' ? 0 : world->getGlobalInt(name);
    }
}
