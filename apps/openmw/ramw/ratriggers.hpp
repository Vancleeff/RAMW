#ifndef RAMW_RATRIGGER_HPP
#define RAMW_RATRIGGER_HPP

#include <memory>
#include <string>

namespace RAMW
{
    struct AchievementData;
    struct AchievementObjective;

    struct RATriggerContext
    {
        std::string key;
        int value = 0;
    };

    class RATrigger
    {
    public:
        virtual ~RATrigger() = default;
        virtual std::string getType() const = 0;
        virtual bool evaluate(const RATriggerContext& ctx, const AchievementData& info) const;
        virtual bool isObjectiveCompleted(const AchievementObjective& obj) const = 0;
    };
}

#endif
