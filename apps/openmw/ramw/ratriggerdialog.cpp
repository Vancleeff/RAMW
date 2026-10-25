#include "ratriggerdialog.hpp"
#include "raachievement.hpp"

namespace RAMW
{
    bool RATriggerDialog::isObjectiveCompleted(const AchievementObjective& /*obj*/) const
    {
        return false;
    }

    bool RATriggerDialog::evaluate(const RATriggerContext& ctx, const AchievementData& info) const
    {
        const size_t at = ctx.key.rfind('@');
        const std::string anySpeaker = at == std::string::npos ? ctx.key : ctx.key.substr(0, at);

        for (const auto& obj : info.objectives)
            if (obj.stat_name == ctx.key || obj.stat_name == anySpeaker)
                return true;

        return false;
    }
}
