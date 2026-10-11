#ifndef RAMW_RATRIGGERLOCATION_HPP
#define RAMW_RATRIGGERLOCATION_HPP

#include "ratriggers.hpp"

namespace RAMW
{
    class RATriggerLocation : public RATrigger
    {
    public:
        static constexpr const char* Prefix = "visited_";

        bool evaluate(const RATriggerContext& ctx, const AchievementData& info) const override;
        bool isObjectiveCompleted(const AchievementObjective& obj) const override;
        std::string getType() const override { return "Location"; }
    };
}

#endif
