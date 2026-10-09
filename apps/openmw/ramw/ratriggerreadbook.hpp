#ifndef RAMW_RATRIGGERREADBOOK_HPP
#define RAMW_RATRIGGERREADBOOK_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerReadBook : public RATrigger
    {
    public:
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        std::string getType() const override { return "ReadBook"; }
    };
}

#endif
