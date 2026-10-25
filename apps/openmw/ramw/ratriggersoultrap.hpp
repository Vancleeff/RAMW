#ifndef RAMW_SOULTRAPTRIGGER_HPP
#define RAMW_SOULTRAPTRIGGER_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerSoulTrap : public RATrigger
    {
    public:
        explicit RATriggerSoulTrap(bool sum = false);

        std::string getType() const override { return "SoulTrap"; }
        bool evaluate(const RATriggerContext& ctx, const AchievementData& info) const override;
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        int getProgress(const AchievementData& info) const override;

    private:
        bool mSum;
    };
}

#endif
