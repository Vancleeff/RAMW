#ifndef RAMW_HEROSTATTRIGGER_HPP
#define RAMW_HEROSTATTRIGGER_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerHeroStats : public RATrigger
    {
    public:
        static constexpr const char* LevelKey = "level";
        static constexpr const char* ReputationKey = "reputation";
        static constexpr const char* SkillPrefix = "skill_";
        static constexpr const char* AttributePrefix = "attribute_";
        static constexpr const char* SpellsKey = "spells";

        bool evaluate(const RATriggerContext& ctx, const AchievementData& info) const override;
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        int getProgress(const AchievementData& info) const override;
        std::string getType() const override { return "HeroStats"; }
    };
}

#endif
