#ifndef RAMW_RAACHIEVEMENTS_HPP
#define RAMW_RAACHIEVEMENTS_HPP

#include "rastats.hpp"
#include "ratriggerQuestComplete.hpp"
#include "ratriggerherostats.hpp"
#include "ratriggerkill.hpp"
#include "ratriggers.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace RAMW
{
    struct AchievementObjective
    {
        std::string stat_name;
        std::string display_label;
        int amount = 1;

        bool isCompleted() const { return RAStats::get(stat_name) >= amount; }
    };

    struct AchievementData
    {
        std::string id;
        std::string parent_id;
        std::string name;
        std::string description;
        int points = 0;
        std::string iconPath;
        bool unlocked = false;
        int category = 0;
        int order = 0;
        int target_value = 0;
        std::string tracked_stat = "";
        int notification_step = 1;
        std::vector<AchievementObjective> objectives;
        std::unique_ptr<RATrigger> trigger;

        AchievementData() = default;
        AchievementData(const AchievementData&) = delete;
        AchievementData& operator=(const AchievementData&) = delete;
        AchievementData(AchievementData&&) = default;
        AchievementData& operator=(AchievementData&&) = default;

        bool hasProgressBar() const { return target_value > 0; }
        bool hasObjectives() const { return !objectives.empty(); }
        bool hasTrigger() const { return trigger && trigger->getType() != "None"; }

        int getCompletedObjectivesCount() const
        {
            if (!trigger)
                return 0;
            int count = 0;
            for (const auto& obj : objectives)
                if (trigger->isObjectiveCompleted(obj))
                    count++;
            return count;
        }

        int getProgress() const;
    };

    class RAAchievementManager
    {
    public:
        static void init();
        static void unlock(const std::string& id);
        static void restoreFromStats();
        static void loadAchievements(const std::string& path);
        static int getCategoryFlag(const std::string& cat);
        static bool isUnlocked(const std::string& id);
        static int getUnlocked();

        static const AchievementData* getAchievementData(const std::string& id);
        static std::map<std::string, AchievementData>& getAllAchievements();

        template <typename T>
        static std::vector<AchievementData*> getAchievementsByTrigger()
        {
            std::vector<AchievementData*> result;
            for (auto& [id, info] : getAllAchievements())
            {
                if (info.unlocked)
                    continue;
                if (dynamic_cast<T*>(info.trigger.get()))
                    result.push_back(&info);
            }
            return result;
        }

        static void setOnUnlockCallback(std::function<void()> callback);

        // bitfields for category filtering
        static constexpr int Category_Quest = (1 << 0);
        static constexpr int Category_Exploration = (1 << 1);
        static constexpr int Category_Combat = (1 << 2);
        static constexpr int Category_Misc = (1 << 3);
        static constexpr int Category_All = ~0;

    private:
        static std::map<std::string, AchievementData> mAchievements;
        static std::function<void()> mOnUnlockCallback;
    };
}

#endif
