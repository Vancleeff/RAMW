#ifndef RAMW_RATRIGGERDISCOVERY_HPP
#define RAMW_RATRIGGERDISCOVERY_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    /// Objectives use either the global counter or a per-location flag:
    ///   locations_discovered — total number of map markers revealed
    ///   discovered_<name>    — set to 1 once that place is found
    class RATriggerDiscovery : public RATrigger
    {
    public:
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        std::string getType() const override { return "Discovery"; }
    };
}

#endif
