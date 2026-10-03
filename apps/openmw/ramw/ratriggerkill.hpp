#ifndef RAMW_KILLTRIGGER_HPP
#define RAMW_KILLTRIGGER_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerKill : public RATrigger
    {
    public:
        std::string getType() const override { return "Kill"; }
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
    };
}

#endif
