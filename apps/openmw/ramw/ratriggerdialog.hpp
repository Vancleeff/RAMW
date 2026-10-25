#ifndef RAMW_DIALOGTRIGGER_HPP
#define RAMW_DIALOGTRIGGER_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerDialog : public RATrigger
    {
    public:
        std::string getType() const override { return "Dialog"; }
        bool evaluate(const RATriggerContext& ctx, const AchievementData& info) const override;
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
    };
}

#endif
