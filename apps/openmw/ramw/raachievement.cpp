#include "raachievement.hpp"
#include "../mwbase/environment.hpp"
#include "achievementnotification.hpp"
#include "raconfig.hpp"
#include "rastats.hpp"
#include "ratriggerQuestComplete.hpp"
#include "ratriggerQuestProgress.hpp"
#include "ratriggercellclear.hpp"
#include "ratriggerdiscovery.hpp"
#include "ratriggerglobal.hpp"
#include "ratriggerharvest.hpp"
#include "ratriggerherostats.hpp"
#include "ratriggerkill.hpp"
#include "ratriggerlocation.hpp"
#include "ratriggerlockpick.hpp"
#include "ratriggerloot.hpp"
#include "ratriggerreadbook.hpp"
#include "raeventprocessor.hpp"

#include <components/debug/debuglog.hpp>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <set>

namespace RAMW
{
    std::map<std::string, AchievementData> RAAchievementManager::mAchievements;
    std::vector<std::string> RAAchievementManager::mSets;
    std::function<void(const std::string&)> RAAchievementManager::mOnUnlockCallback = nullptr;

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

    const std::vector<std::string>& RAAchievementManager::getSets()
    {
        return mSets;
    }

    std::vector<std::string> RAAchievementManager::getRegions()
    {
        std::set<std::string> unique;
        for (const auto& [id, ach] : mAchievements)
            unique.insert(ach.regions.begin(), ach.regions.end());
        return { unique.begin(), unique.end() };
    }

    std::vector<std::string> RAAchievementManager::getFactions()
    {
        std::set<std::string> unique;
        for (const auto& [id, ach] : mAchievements)
            unique.insert(ach.factions.begin(), ach.factions.end());
        return { unique.begin(), unique.end() };
    }

    void RAAchievementManager::init()
    {
        mAchievements.clear();
        mSets.clear();

        if (RAConfig::TEST_MODE)
        {
            // Every json in the folder is a set for testing mode
            const std::filesystem::path setsDir = "resources/vfs/ramw";

            if (std::filesystem::is_directory(setsDir))
            {
                for (const auto& entry : std::filesystem::directory_iterator(setsDir))
                    if (entry.is_regular_file() && entry.path().extension() == ".json")
                        loadAchievements(entry.path().string());
            }
        }

        // todo getting jsons from a webserver and define with RA devs how to serve it

        Log(Debug::Info) << "[RA Achievement] Loaded " << mAchievements.size() << " achievements";

        RAEventProcessor::rebuildTrackedGlobals();
    }

    void RAAchievementManager::loadAchievements(const std::string& path)
    {
        try
        {
            YAML::Node root = YAML::LoadFile(path);

            // A set file names itself once, for all its content
            const std::string setName = root["set"] ? root["set"].as<std::string>() : "Base";
            const std::string setPrefix = root["prefix"] ? root["prefix"].as<std::string>() : "";
            const YAML::Node list = root["achievements"] ? root["achievements"] : root;

            for (size_t i = 0; i < list.size(); i++)
            {
                const auto& node = list[i];
                AchievementData info;
                info.id = node["id"].as<std::string>();
                // Ids should use their set prefix to avoid duplicate IDs between 2 sets
                if (!setPrefix.empty() && info.id.rfind(setPrefix + "_", 0) != 0)
                    Log(Debug::Warning) << "[RAMW] " << info.id << " is missing the " << setPrefix << "_ prefix of set " << setName;
                // if a duplicate happens, keep the first one only, ignore the rest and log the issue
                if (mAchievements.count(info.id) > 0)
                {
                    Log(Debug::Warning) << "[RAMW] " << path << " reuses id " << info.id << " - entry ignored";
                    continue;
                }
                info.name = node["name"].as<std::string>();
                info.description = node["description"].as<std::string>();
                info.points = node["points"].as<int>();
                info.unlocked = false;
                info.type = getTypeFlag(
                    node["type"] && node["type"].IsScalar() ? node["type"].as<std::string>() : std::string());
                info.category = getCategoryFlag(node["category"].as<std::string>());
                info.order = node["order"].as<int>();
                info.target_value
                    = node["target_value"] && node["target_value"].IsScalar() ? node["target_value"].as<int>() : 0;
                info.tracked_stat = node["tracked_stat"] && node["tracked_stat"].IsScalar()
                    ? node["tracked_stat"].as<std::string>()
                    : "";
                info.notification_step = node["notification_step"] && node["notification_step"].IsScalar()
                    ? node["notification_step"].as<int>()
                    : 1;
                info.parent_id
                    = node["parent_id"] && node["parent_id"].IsScalar() ? node["parent_id"].as<std::string>() : "";
                info.set = setName;

                if (node["regions"] && node["regions"].IsSequence())
                    for (const auto& r : node["regions"])
                        info.regions.push_back(r.as<std::string>(""));

                if (node["factions"] && node["factions"].IsSequence())
                    for (const auto& f : node["factions"])
                        info.factions.push_back(f.as<std::string>(""));

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
                        objective.op = getCompareOp(obj["op"].as<std::string>(""));
                        objective.group = obj["group"].as<std::string>("");
                        objective.any_of = obj["any_of"].as<bool>(false);
                        info.objectives.push_back(objective);
                    }
                    if (node["progress_bar"].as<bool>(false) && info.tracked_stat.empty() && info.target_value == 0
                        && info.objectives.size() == 1)
                    {
                        info.tracked_stat = info.objectives.front().stat_name;
                        info.target_value = info.objectives.front().amount;
                    }
                }

                // Parse triggers
                if (node["trigger"])
                {
                    std::string triggerType = node["trigger"]["type"].as<std::string>("");
                    if (triggerType == "QuestComplete")
                        info.trigger = std::make_unique<RATriggerQuestComplete>();
                    else if (triggerType == "QuestProgress")
                        info.trigger = std::make_unique<RATriggerQuestProgress>();
                    else if (triggerType == "HeroStats")
                        info.trigger = std::make_unique<RATriggerHeroStats>();
                    else if (triggerType == "Kill")
                        info.trigger = std::make_unique<RATriggerKill>(node["trigger"]["sum"].as<bool>(false));
                    else if (triggerType == "Lockpick")
                        info.trigger = std::make_unique<RATriggerLockpick>();
                    else if (triggerType == "Harvest")
                        info.trigger = std::make_unique<RATriggerHarvest>();
                    else if (triggerType == "Loot")
                        info.trigger = std::make_unique<RATriggerLoot>();
                    else if (triggerType == "CellClear")
                        info.trigger = std::make_unique<RATriggerCellClear>();
                    else if (triggerType == "Discovery")
                        info.trigger = std::make_unique<RATriggerDiscovery>();
                    else if (triggerType == "Location")
                        info.trigger = std::make_unique<RATriggerLocation>();
                    else if (triggerType == "Global")
                        info.trigger = std::make_unique<RATriggerGlobal>();
                    else if (triggerType == "ReadBook")
                        info.trigger = std::make_unique<RATriggerReadBook>();

                }

                mAchievements[info.id] = std::move(info);
            }
            // Kept in load order so the base set stays first in the selector
            if (std::find(mSets.begin(), mSets.end(), setName) == mSets.end())
                mSets.push_back(setName);

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
        if (cat == "Loot")
            return Category_Loot;
        if (cat == "Character")
            return Category_Character;
        if (cat == "Collection")
            return Category_Collection;
        return Category_Misc;
    }

    int RAAchievementManager::getTypeFlag(const std::string& type)
    {
        if (type == "missable")
            return Type_Missable;
        if (type == "progression")
            return Type_Progression;
        if (type == "win_condition")
            return Type_WinCondition;
        return Type_Normal;
    }

    int RAAchievementManager::getCompareOp(const std::string& op)
    {
        if (op == "==")
            return Cmp_Exactly;
        if (op == "<=")
            return Cmp_AtMost;
        return Cmp_AtLeast;
    }

    void RAAchievementManager::setOnUnlockCallback(std::function<void(const std::string&)> callback)
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
                    AchievementNotification::getInstance().show(achievementId);

                if (mOnUnlockCallback)
                    mOnUnlockCallback(achievementId);
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
        if (!hasProgressBar())
            return 0;

        const int progress = trigger ? trigger->getProgress(*this)
                                     : (tracked_stat.empty() ? 0 : RAStats::get(tracked_stat));
        return std::min(progress, target_value);
    }
}
