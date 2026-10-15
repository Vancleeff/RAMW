#ifndef RAMW_RAACHIEVEMENTS_HPP
#define RAMW_RAACHIEVEMENTS_HPP

#include "rastats.hpp"
#include "ratriggers.hpp" 
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace RAMW
{
    enum AchievementType
    {
        Type_Normal = 0,
        Type_Missable,
        Type_Progression,
        Type_WinCondition
    };

    enum CompareOp
    {
        Cmp_AtLeast = 0, // >=
        Cmp_Exactly,     // ==
        Cmp_AtMost       // <=
    };

    struct AchievementObjective
    {
        std::string stat_name;
        std::string display_label;
        std::string group;
        int amount = 1;
        int op = Cmp_AtLeast;
        bool any_of = false;

        bool matches(int value) const
        {
            if (op == Cmp_Exactly)
                return value == amount;
            if (op == Cmp_AtMost)
                return value <= amount;
            return value >= amount;
        }

        bool isCompleted() const { return matches(RAStats::get(stat_name)); }
    };

    struct AchievementData
    {
        int points = 0;
        bool unlocked = false;
        int category = 0;
        int type = Type_Normal;
        int order = 0;
        int target_value = 0;
        int notification_step = 1;

        std::string id;
        std::string parent_id;
        std::string name;
        std::string description;
        std::string iconPath;
        std::string set;
        std::string tracked_stat = "";
        std::vector<std::string> regions;
        std::vector<std::string> factions;

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

        bool isListedObjective(const AchievementObjective& obj) const
        {
            if (obj.group.empty())
                return true;

            const AchievementObjective* anchor = nullptr;
            for (const auto& other : objectives)
            {
                if (other.group != obj.group)
                    continue;
                if (other.stat_name.rfind(RAStatKeys::CellClearPrefix, 0) == 0)
                {
                    anchor = &other;
                    break;
                }
                if (!anchor)
                    anchor = &other;
            }
            return anchor == &obj;
        }

        int getListedObjectivesCount() const
        {
            int n = 0;
            for (const auto& obj : objectives)
                if (isListedObjective(obj))
                    n++;
            return n;
        }

        bool isGroupCompleted(const std::string& group) const
        {
            if (!trigger)
                return false;

            bool anyOf = false;
            for (const auto& obj : objectives)
                if (obj.group == group && obj.any_of)
                    anyOf = true;

            for (const auto& obj : objectives)
            {
                if (obj.group != group)
                    continue;

                const bool done = trigger->isObjectiveCompleted(obj);
                if (anyOf && done)
                    return true;
                if (!anyOf && !done)
                    return false;
            }
            return !anyOf;
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
        static int getTypeFlag(const std::string& type);
        static int getCompareOp(const std::string& op);
        static bool isUnlocked(const std::string& id);
        static int getUnlocked();

        static const AchievementData* getAchievementData(const std::string& id);
        static std::map<std::string, AchievementData>& getAllAchievements();
        static const std::vector<std::string>& getSets();
        static std::vector<std::string> getRegions();
        static std::vector<std::string> getFactions();

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

        static void setOnUnlockCallback(std::function<void(const std::string&)> callback);

        // bitfields for category filtering
        static constexpr int Category_Quest = (1 << 0);
        static constexpr int Category_Exploration = (1 << 1);
        static constexpr int Category_Combat = (1 << 2);
        static constexpr int Category_Misc = (1 << 3);
        static constexpr int Category_Loot = (1 << 4);
        static constexpr int Category_Character = (1 << 5);
        static constexpr int Category_Collection = (1 << 6);
        static constexpr int Category_All = ~0;

    private:
        static std::map<std::string, AchievementData> mAchievements;
        static std::function<void(const std::string&)> mOnUnlockCallback;
        static std::vector<std::string> mSets;
    };
}

#endif
