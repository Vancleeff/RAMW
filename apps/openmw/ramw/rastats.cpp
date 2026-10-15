#include "rastats.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwgui/windowmanagerimp.hpp"
#include "raachievement.hpp"
#include "raeventprocessor.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadcrea.hpp>

namespace RAMW
{
    std::map<std::string, int> RAStats::mStats;
    std::map<std::string, std::vector<RAStats::ProgressEntry>> RAStats::mProgressIndex;
    std::map<std::string, float> RAStats::mPendingNotifications;

    void RAStats::init()
    {
        mStats.clear();
        mProgressIndex.clear();
        mPendingNotifications.clear();
        Log(Debug::Info) << "[RAMW] RAStats initialized";
    }

    void RAStats::update(float dt)
    {
        for (auto it = mPendingNotifications.begin(); it != mPendingNotifications.end();)
        {
            it->second += dt;
            if (it->second < kNotificationDelay)
            {
                ++it;
                continue;
            }

            const std::string& statName = it->first;
            const int currentValue = get(statName);

            auto indexIt = mProgressIndex.find(statName);
            if (indexIt != mProgressIndex.end())
            {
                // a notification on grouped achievement is triggered only for the current tier
                const ProgressEntry* tier = nullptr;
                for (const auto& entry : indexIt->second)
                {
                    if (RAAchievementManager::isUnlocked(entry.achievementId) || currentValue > entry.target)
                        continue;
                    if (!tier || entry.target < tier->target)
                        tier = &entry;
                }

                const AchievementData* info
                    = tier ? RAAchievementManager::getAchievementData(tier->achievementId) : nullptr;
                const int step = (info && info->notification_step > 1) ? info->notification_step : 1;

                if (tier && (step == 1 || currentValue % step == 0))
                {
                    const std::string msg = (tier->target == 1)
                        ? tier->label + " Completed"
                        : tier->label + ": " + std::to_string(currentValue) + " / " + std::to_string(tier->target);

                    MWBase::Environment::get().getWindowManager()->scheduleMessageBox(
                        msg, MWGui::ShowInDialogueMode_Never);
                }
            }

            it = mPendingNotifications.erase(it);
        }
    }

    void RAStats::increment(const std::string& statName, int amount)
    {
        mStats[statName] += amount;
        checkProgressNotification(statName);
        RAEventProcessor::onStatChanged(statName, mStats[statName]);
        Log(Debug::Verbose) << "[RAMW] Stat incremented: " << statName << " += " << amount
                            << " (now: " << mStats[statName] << ")";
    }

    int RAStats::get(const std::string& statName)
    {
        auto it = mStats.find(statName);
        return (it != mStats.end()) ? it->second : 0;
    }

    void RAStats::set(const std::string& statName, int value)
    {
        mStats[statName] = value;
        checkProgressNotification(statName);
        RAEventProcessor::onStatChanged(statName, value);
        Log(Debug::Verbose) << "[RAMW] Stat set: " << statName << " = " << value;
    }

    void RAStats::reset(const std::string& statName)
    {
        mStats[statName] = 0;
        Log(Debug::Verbose) << "[RAMW] Stat reset: " << statName;
    }

    void RAStats::clear()
    {
        mStats.clear();
        Log(Debug::Info) << "[RAMW] All stats cleared";
    }

    void RAStats::write(ESM::ESMWriter& writer)
    {
        writer.startRecord(ESM::REC_RAST);

        for (const auto& [key, value] : mStats)
        {
            writer.writeHNString("STAT", key);
            writer.writeHNT("VALU", value);
        }

        writer.endRecord(ESM::REC_RAST);

        Log(Debug::Info) << "[RAMW] Saved " << mStats.size() << " RA stats to save file";
    }

    void RAStats::readRecord(ESM::ESMReader& reader)
    {
        mStats.clear();

        while (reader.isNextSub("STAT"))
        {
            std::string key = reader.getHString();
            int value = 0;
            reader.getHNT(value, "VALU");
            mStats[key] = value;
        }

        Log(Debug::Info) << "[RAMW] Loaded " << mStats.size() << " RA stats from save file";
    }

    void RAStats::kill(const std::string& targetId, bool isNpc, int creatureType)
    {
        increment((isNpc ? RAStatKeys::NpcKillPrefix : RAStatKeys::CreatureKillPrefix) + targetId);

        if (isNpc)
        {
            increment(RAStatKeys::KillNpcTotal);
            return;
        }

        if (creatureType == ESM::Creature::Daedra)
            increment(RAStatKeys::KillDaedraTotal);
        else if (creatureType == ESM::Creature::Undead)
            increment(RAStatKeys::KillUndeadTotal);
        else if (creatureType == ESM::Creature::Humanoid) // Humanoid != NPC (sixth house mobs mostly)
            increment(RAStatKeys::KillHumanoidTotal);
        else
            increment(RAStatKeys::KillCreatureTotal);
    }

    bool RAStats::readBook(const std::string& bookId, bool isSkillBook, bool isScroll)
    {
        const std::string key = RAStatKeys::BookReadPrefix + bookId;

        if (get(key))
            return false;

        // We don't count Scroll and note as book but we track them in case of achievement condition
        set(key, 1);

        if (!isScroll)
        {
            increment(RAStatKeys::BooksRead);

            if (isSkillBook)
                increment(RAStatKeys::SkillBooksRead);
        }

        return true;
    }

    const std::map<std::string, int>& RAStats::getAllStats()
    {
        return mStats;
    }

    void RAStats::buildProgressIndex()
    {
        mProgressIndex.clear();
        for (const auto& [id, info] : RAAchievementManager::getAllAchievements())
        {
            if (info.unlocked)
                continue;

            if (info.hasProgressBar() && !info.tracked_stat.empty())
            {
                mProgressIndex[info.tracked_stat].push_back({ id, info.name, info.target_value });
                Log(Debug::Info) << "[RAMW] Added to Index list : " << info.tracked_stat;
            }

            for (const auto& obj : info.objectives)
            {
                mProgressIndex[obj.stat_name].push_back({ id, info.name + "\n" + obj.display_label, obj.amount });
                Log(Debug::Info) << "[RAMW] Added to Index list : " << obj.stat_name;
            }
        }

        Log(Debug::Info) << "[RAMW] Progress index built: " << mProgressIndex.size() << " tracked stats";
    }

    void RAStats::checkProgressNotification(const std::string& statName)
    {
        if (mProgressIndex.find(statName) == mProgressIndex.end())
            return;

        auto it = mPendingNotifications.find(statName);
        if (it == mPendingNotifications.end())
            mPendingNotifications[statName] = kNotificationDelay;
        else
            it->second = 0.0f;
    }
}
