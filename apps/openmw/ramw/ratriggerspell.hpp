#ifndef RAMW_SPELLTRIGGER_HPP
#define RAMW_SPELLTRIGGER_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerSpell : public RATrigger
    {
    public:
        explicit RATriggerSpell(bool sum = false);

        std::string getType() const override { return "Spell"; }
        bool evaluate(const RATriggerContext& ctx, const AchievementData& info) const override;
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        int getProgress(const AchievementData& info) const override;

    private:
        bool mSum;
    };
}

#endif
