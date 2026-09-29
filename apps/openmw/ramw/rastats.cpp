#include "rastats.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwgui/windowmanagerimp.hpp"
#include "raachievement.hpp"
#include "raeventprocessor.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

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
                for (const auto& entry : indexIt->second)
                {
                    if (RAAchievement::isUnlocked(entry.achievementId) || currentValue > entry.target)
                        continue;

                    const std::string msg = (entry.target == 1)
                        ? entry.label + " Completed"
                        : entry.label + ": " + std::to_string(currentValue) + " / " + std::to_string(entry.target);

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

    void RAStats::kill(const std::string& targetId, bool isNpc)
    {
        increment((isNpc ? RAStatKeys::NpcKillPrefix : RAStatKeys::CreatureKillPrefix) + targetId);
        std::string statName = isNpc ? RAStatKeys::KillNpcTotal : RAStatKeys::KillCreatureTotal;
        increment(statName, 1);
    }

    void RAStats::readBook(const std::string& bookId, bool isSkillBook)
    {
        if (!get(bookId))
            return;

        increment(RAStatKeys::BooksRead);
        set(bookId, 1);

        if (isSkillBook)
        {
            increment(RAStatKeys::SkillBooksRead);
        }
    }

    const std::map<std::string, int>& RAStats::getAllStats()
    {
        return mStats;
    }

    void RAStats::buildProgressIndex()
    {
        mProgressIndex.clear();
        for (const auto& [id, info] : RAAchievement::getAllAchievements())
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
        auto indexIt = mProgressIndex.find(statName);
        if (indexIt == mProgressIndex.end())
            return;

        int currentValue = get(statName);
        bool shouldNotify = false;
        for (const auto& entry : indexIt->second)
        {
            const AchievementInfo* info = RAAchievement::getAchievementInfo(entry.achievementId);
            if (!info)
                continue;
            int step = info->notification_step > 1 ? info->notification_step : 1;
            if (step == 1 or currentValue % step == 0)
            {
                shouldNotify = true;
                break;
            }
        }

        if (!shouldNotify)
            return;

        auto it = mPendingNotifications.find(statName);
        if (it == mPendingNotifications.end())
            mPendingNotifications[statName] = kNotificationDelay;
        else
            it->second = 0.0f;
    }
}
