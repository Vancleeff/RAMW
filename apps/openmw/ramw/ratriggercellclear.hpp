#ifndef RAMW_RATRIGGERCELLCLEAR_HPP
#define RAMW_RATRIGGERCELLCLEAR_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    /// Objectives mix cell completion flags with regular loot/kill keys:
    ///   cellclear_<cellName> — set to 1 once no enemy remains in that cell
    ///   loot_<itemId>        — unique treasure picked up there
    ///   npc_kill_<npcId>     — named enemy killed there
    class RATriggerCellClear : public RATrigger
    {
    public:
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        std::string getType() const override { return "CellClear"; }
    };
}

#endif
