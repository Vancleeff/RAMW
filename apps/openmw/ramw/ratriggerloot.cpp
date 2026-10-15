#include "ratriggerloot.hpp"
#include "raachievement.hpp"
#include "rastats.hpp"

#include <components/esm/refid.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"

#include <cstring>

namespace RAMW
{
    bool RATriggerLoot::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        if (obj.stat_name.rfind(RAStatKeys::InventoryPrefix, 0) == 0)
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            if (player.isEmpty())
                return false;

            const std::string itemId = obj.stat_name.substr(std::strlen(RAStatKeys::InventoryPrefix));
            const MWWorld::ContainerStore& store = player.getClass().getContainerStore(player);
            return obj.matches(store.count(ESM::RefId::deserializeText(itemId)));
        }

        return obj.matches(RAStats::get(obj.stat_name));
    }
}
