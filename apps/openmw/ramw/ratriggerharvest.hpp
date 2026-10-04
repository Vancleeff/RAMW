#ifndef RAMW_RATRIGGERHARVEST_HPP
#define RAMW_RATRIGGERHARVEST_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerHarvest : public RATrigger
    {
    public:
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        std::string getType() const override { return "Harvest"; }
    };
}

#endif
