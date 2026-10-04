#ifndef RAMW_RATRIGGERQUESTPROGRESS_HPP
#define RAMW_RATRIGGERQUESTPROGRESS_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerQuestProgress : public RATrigger
    {
    public:
        bool evaluate(const RATriggerContext& ctx, const AchievementData& info) const override;
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        std::string getType() const override { return "QuestProgress"; }
    };
}

#endif
