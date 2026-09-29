#ifndef RAMW_RASTATS_HPP
#define RAMW_RASTATS_HPP

#include <map>
#include <string>
#include <vector>

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace RAMW
{
    /// Stat keys used by RAStats
    namespace RAStatKeys
    {
        inline constexpr const char* KillCreatureTotal = "kill_creature_total";
        inline constexpr const char* KillNpcTotal = "kill_npc_total";
        inline constexpr const char* ItemsLooted = "items_looted";
        inline constexpr const char* ItemsStolen = "items_stolen";
        inline constexpr const char* GoldLooted = "gold_looted";
        inline constexpr const char* QuestsFinished = "quests_finished";
        inline constexpr const char* LockpickCount = "lockpick_count";
        inline constexpr const char* BooksRead = "books_read";
        inline constexpr const char* SkillBooksRead = "skillbooks_read";
        inline constexpr const char* CreatureKillPrefix = "creature_kill_";
        inline constexpr const char* NpcKillPrefix = "npc_kill_";
        inline constexpr const char* QuestFinishedPrefix = "quest_finished_";
        inline constexpr const char* AchievementTestPrefix = "ACH_TEST_";
    }

    /// RetroAchievements Stat Manager
    class RAStats
    {
    public:
        static void init();
        static void update(float dt);
        static void buildProgressIndex();

        static void increment(const std::string& statName, int amount = 1);
        static int get(const std::string& statName);
        static void set(const std::string& statName, int value);
        static void reset(const std::string& statName);
        static void clear();
        static void write(ESM::ESMWriter& writer);
        static void readRecord(ESM::ESMReader& reader);

        static void kill(const std::string& targetId, bool isNpc);
        static void readBook(const std::string& bookId, bool isSkillBook = false);
        
        static const std::map<std::string, int>& getAllStats();
        
    private:
        struct ProgressEntry
        {
            std::string achievementId;
            std::string label;
            int target;
        };

        static std::map<std::string, int> mStats;
        static std::map<std::string, std::vector<ProgressEntry>> mProgressIndex;
        static std::map<std::string, float> mPendingNotifications;
        static constexpr float kNotificationDelay = 2.0f;

        static void checkProgressNotification(const std::string& statName);
    };
}

#endif
