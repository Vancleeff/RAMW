#ifndef RAMW_KILLTRIGGER_HPP
#define RAMW_KILLTRIGGER_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerKill : public RATrigger
    {
    public:
        explicit RATriggerKill(bool sum = false);

        std::string getType() const override { return "Kill"; }
        bool evaluate(const RATriggerContext& ctx, const AchievementData& info) const override;
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        int getProgress(const AchievementData& info) const override;

    private:
        bool mSum;
    };
}

#endif
