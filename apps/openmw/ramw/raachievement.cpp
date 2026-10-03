#include <yaml-cpp/yaml.h>
#include "../mwbase/environment.hpp"
#include "achievementnotification.hpp"
#include "raachievement.hpp"
#include "raconfig.hpp"
#include "rastats.hpp" 
#include <components/debug/debuglog.hpp>

namespace RAMW
{
    std::map<std::string, AchievementData> RAAchievementManager::mAchievements;
    std::function<void()> RAAchievementManager::mOnUnlockCallback = nullptr;
    

    static std::string getRandomTestIcon()
    {
        if (!RAConfig::TEST_MODE)
            return "";

        if (RAConfig::TEST_ICON_POOL.empty())
            return RAConfig::TEST_USER_ICON;

        static bool seeded = false;
        if (!seeded)
        {
            std::srand(static_cast<unsigned int>(std::time(nullptr)));
            seeded = true;
        }

        size_t index = std::rand() % RAConfig::TEST_ICON_POOL.size();
        return RAConfig::TEST_ICON_POOL[index];
    }

     

    void RAAchievementManager::init()
    {
        mAchievements.clear();

        if (RAConfig::TEST_MODE)
            loadAchievements("resources/vfs/ramw/ramw_achievements.json");

        //todo getting json from a webserver and define with RA devs how to serve it

        Log(Debug::Info) << "[RA Achievement] Loaded " << mAchievements.size() << " achievements";
    }

    void RAAchievementManager::loadAchievements(const std::string& path)
    {
        try
        {
            YAML::Node root = YAML::LoadFile(path);

            for (size_t i = 0; i < root.size(); i++)
            {
                Log(Debug::Info) << "[RAMW] Parsing entry " << i;
                const auto& node = root[i];
                AchievementData info;
                info.id = node["id"].as<std::string>();
                info.name = node["name"].as<std::string>();
                info.description = node["description"].as<std::string>();
                info.points = node["points"].as<int>();
                info.unlocked = false;
                info.category = getCategoryFlag(node["category"].as<std::string>());
                info.order = node["order"].as<int>();
                info.target_value = node["target_value"] && node["target_value"].IsScalar() ? node["target_value"].as<int>() : 0;
                info.tracked_stat = node["tracked_stat"] && node["tracked_stat"].IsScalar() ? node["tracked_stat"].as<std::string>() : "";
                info.notification_step = node["notification_step"] && node["notification_step"].IsScalar() ? node["notification_step"].as<int>() : 1;
                info.parent_id = node["parent_id"] && node["parent_id"].IsScalar() ? node["parent_id"].as<std::string>() : "";

                std::string icon = node["icon"] ? node["icon"].as<std::string>() : "random";
                info.iconPath = (icon == "random") ? getRandomTestIcon() : icon;

                if (node["objectives"] && node["objectives"].IsSequence())
                {
                    for (size_t j = 0; j < node["objectives"].size(); j++)
                    {
                        YAML::Node obj = node["objectives"][j];
                        if (!obj.IsMap())
                            continue;

                        AchievementObjective objective;
                        objective.stat_name = obj["stat_name"].as<std::string>("");
                        objective.display_label = obj["display_label"].as<std::string>("");
                        objective.amount = obj["amount"].as<int>(1);
                        info.objectives.push_back(objective);
                    }
                }

                // Parse triggers
                if (node["trigger"])
                {
                    std::string triggerType = node["trigger"]["type"].as<std::string>("");
                    if (triggerType == "QuestComplete")
                    {
                        info.trigger = std::make_unique<RATriggerQuestComplete>();
                    }
                    else if (triggerType == "HeroStats")
                    {
                        Log(Debug::Info) << "[RAMW] parse HeroStats trigger : " << info.name;
                        info.trigger = std::make_unique<RATriggerHeroStats>();
                    }
                    else if (triggerType == "Kill")
                    {
                        Log(Debug::Info) << "[RAMW] parse Kill trigger : " << info.name;
                        info.trigger = std::make_unique<RATriggerKill>();
                    }
                }

                mAchievements[info.id] = std::move(info);
            }

            Log(Debug::Info) << "[RAMW] Loaded " << mAchievements.size() << " achievements from " << path;
        }
        catch (const std::exception& e)
        {
            Log(Debug::Error) << "[RAMW] Failed to load achievements from " << path << ": " << e.what();
        }
    }

    int RAAchievementManager::getCategoryFlag(const std::string& cat)
    {
        if (cat == "Quest")
            return Category_Quest;
        if (cat == "Exploration")
            return Category_Exploration;
        if (cat == "Combat")
            return Category_Combat;
        if (cat == "Misc")
            return Category_Misc;
        return Category_Misc;
    }

    void RAAchievementManager::setOnUnlockCallback(std::function<void()> callback)
    {
        mOnUnlockCallback = callback;
    }

    void RAAchievementManager::unlock(const std::string& achievementId)
    {
        auto it = mAchievements.find(achievementId);
        if (it != mAchievements.end())
        {
            if (!it->second.unlocked)
            {
                it->second.unlocked = true;
                Log(Debug::Info) << "[RAMW] UNLOCKED: " << it->second.name;
                if (RAConfig::TEST_MODE)
                    RAStats::set("ACH_TEST_" + achievementId, 1);
                RAStats::buildProgressIndex();
                if (AchievementNotification::hasInstance())
                {
                    AchievementNotification::getInstance().show(achievementId);
                    if (mOnUnlockCallback)
                        mOnUnlockCallback();
                }
            }
        }
        else
        {
            Log(Debug::Error) << "[RAMW] Unknown achievement ID: " << achievementId;
        }
    }

    void RAAchievementManager::restoreFromStats()
    {
        for (auto& [id, info] : mAchievements)
        {
            info.unlocked = false;
            if (RAStats::get("ACH_TEST_" + id) > 0)
                info.unlocked = true;
        }
    }

    bool RAAchievementManager::isUnlocked(const std::string& achievementId)
    {
        auto it = mAchievements.find(achievementId);
        return (it != mAchievements.end() && it->second.unlocked);
    }

    int RAAchievementManager::getUnlocked()
    {
        int count = 0;
        for (const auto& pair : mAchievements)
        {
            if (pair.second.unlocked)
                count++;
        }
        return count;
    }

    const AchievementData* RAAchievementManager::getAchievementData(const std::string& id)
    {
        auto it = mAchievements.find(id);
        return (it != mAchievements.end()) ? &it->second : nullptr;
    }

    std::map<std::string, AchievementData>& RAAchievementManager::getAllAchievements()
    {
        return mAchievements;
    }

    int AchievementData::getProgress() const
    {
        int progress = (hasProgressBar() && !tracked_stat.empty()) ? RAStats::get(tracked_stat) : 0;
        return std::min(progress, target_value);
    }
}
