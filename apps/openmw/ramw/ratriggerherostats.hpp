#ifndef RAMW_HEROSTATTRIGGER_HPP
#define RAMW_HEROSTATTRIGGER_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerHeroStats : public RATrigger
    {
    public:
        bool evaluate(const RATriggerContext& ctx, const AchievementData& info) const override;
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        std::string getType() const override { return "HeroStats"; }
    };
}

#endif
