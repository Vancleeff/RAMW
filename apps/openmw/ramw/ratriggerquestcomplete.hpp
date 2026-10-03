#ifndef RAMW_QUESTCOMPLETETRIGGER_HPP
#define RAMW_QUESTCOMPLETETRIGGER_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerQuestComplete : public RATrigger
    {
    public:
        std::string getType() const override { return "QuestComplete"; }
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
    };
}

#endif
