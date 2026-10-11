#include "ratriggerlocation.hpp"
#include "raachievement.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/cell.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace
{
    bool iequals(std::string_view a, std::string_view b)
    {
        return a.size() == b.size()
            && std::equal(a.begin(), a.end(), b.begin(), [](unsigned char x, unsigned char y) {
                   return std::tolower(x) == std::tolower(y);
               });
    }
}

namespace RAMW
{
    bool RATriggerLocation::evaluate(const RATriggerContext& ctx, const AchievementData& info) const
    {
        for (const auto& obj : info.objectives)
            if (obj.stat_name == ctx.key)
                return true;
        return false;
    }

    bool RATriggerLocation::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        if (obj.stat_name.rfind(Prefix, 0) != 0)
            return false;

        const MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        if (player.isEmpty() || !player.getCell())
            return false;

        return iequals(player.getCell()->getCell()->getDisplayName(),
            std::string_view(obj.stat_name).substr(std::strlen(Prefix)));
    }
}
