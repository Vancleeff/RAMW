#ifndef RAMW_RATRIGGERLOOT_HPP
#define RAMW_RATRIGGERLOOT_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    /// Handles two type of objectives:
    ///----loot_<itemId> check if an item has been looted through RAstats
    ///----inv_<itemId> check if a specific amount of an item is currently in the inventory
    class RATriggerLoot : public RATrigger
    {
    public:
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        std::string getType() const override { return "Loot"; }
    };
}

#endif
