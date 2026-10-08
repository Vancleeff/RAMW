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
        if (obj.stat_name.rfind(Prefix, 0) != 0)
            return false;

        const std::string_view varName = std::string_view(obj.stat_name).substr(std::strlen(Prefix));
        return MWBase::Environment::get().getWorld()->getGlobalInt(MWWorld::GlobalVariableName(varName)) >= obj.amount;
    }
}
