#include "raconsole.hpp"
#include "raachievement.hpp"
#include "raconfig.hpp"
#include "rastats.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <string>
#include <vector>

namespace
{
    // Same contract as a script string argument: quotes keep the spaces, case is left alone
    std::string nextArgument(std::string& args)
    {
        Misc::StringUtils::trim(args);

        if (args.empty())
            return {};

        if (args.front() == '"')
        {
            const size_t end = args.find('"', 1);
            const std::string value = args.substr(1, end == std::string::npos ? end : end - 1);

            args.erase(0, end == std::string::npos ? std::string::npos : end + 1);

            return value;
        }

        const size_t end = args.find_first_of(" \t");
        const std::string value = args.substr(0, end);

        args.erase(0, end == std::string::npos ? std::string::npos : end + 1);

        return value;
    }
}

namespace RAMW
{
    // Commands white list for hardcore mode
    const std::set<std::string> RAConsole::sAllowedCommands = {
        // RAMW commands
        "ramwstats",
        "ramwclearcreatures",
        "ramwclearkills",
        "ramw_unlock",

        // OPENMW Legacy commands that are okay in hardcore
        "help",
        "getpos",
        "getangle",
        "getpccell",
        "getinterior",
        "getlevel",
        "gethealth",
        "getmagicka",
        "getfatigue",
        "getgold",
        "getdisposition",
        "getpcrank",
        "getreputation",
        "getcurrentweather",
        "getdistance",
        "getlos",
        "getlocked",
        "getitemcount",
        "getspellcount",
        "getdetected",
        "getcurrentaipackage",
        "getdeadcount",
        "getstandingactor",
        "getstandingpc",
        "getweaponhealth",
        "getarmorhealth",
        "getcollidingpc",
        "getcollidingactor",

        "tfv", // toggle first person view
        "tcb", // toggle collision boxes (debug render)
        "tb", // toggle borders
        "tpg", // toggle path grid
        "ts", // toggle sky
        "tws", // toggle world stats
        "tfps", // toggle fps
        "tm", // toggle menus
        "tsg", // toggle stats graph
        "twf", // toggle wireframe
        "tt", // toggle textures

        "save",
        "quit",
        "showvars",
        "sv", // showvars alias
        "showscenegraph",
    };

    std::string RAConsole::extractCommandName(const std::string& command)
    {
        std::string s = command;

        // remove target if any
        auto arrow = s.find("->");
        if (arrow != std::string::npos)
            s = s.substr(arrow + 2);

        auto start = s.find_first_not_of(" \t");
        if (start == std::string::npos)
            return {};
        s = s.substr(start);
        s = s.substr(0, s.find_first_of(" \t"));

        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    bool RAConsole::isCommandAllowed(const std::string& command)
    {
        if constexpr (RAMW::RAConfig::TEST_MODE)
            return true;

        std::string cmdName = extractCommandName(command);
        if (cmdName.empty())
            return false;

        return sAllowedCommands.contains(cmdName);
    }

    bool RAConsole::interceptCommand(MWGui::Console* console, const std::string& command)
    {
        if (!isCommandAllowed(command))
        {
            Log(Debug::Warning) << "[RA Security] Blocked cheating command: " << command;
            console->printError("This command is disabled when retro achievement integration is active.");
            return false;
        }
        execute(console, command);

        return true;
    }

    void RAConsole::execute(MWGui::Console* console, const std::string& command)
    {
        if (command == "ramwstats") // Testing command - show all RA stats stored on the current save file
        {
            const auto& allStats = RAStats::getAllStats();

            if (allStats.empty())
            {
                console->printError("No stats tracked yet.");
                return;
            }

            console->printOK("=== RetroAchievements Stats (" + std::to_string(allStats.size()) + " total) ===");

            std::vector<std::pair<std::string, int>> creatureKills;
            std::vector<std::pair<std::string, int>> npcKills;
            std::vector<std::pair<std::string, int>> dungeonStats;
            std::vector<std::pair<std::string, int>> otherStats;
            std::vector<std::pair<std::string, int>> bookstats;

            for (const auto& [statName, value] : allStats)
            {
                if (statName.find("creature_kill_") == 0)
                    creatureKills.push_back({ statName.substr(14), value });
                else if (statName.find("npc_kill_") == 0)
                    npcKills.push_back({ statName.substr(9), value });
                else if (statName.find("dungeon_") == 0)
                    dungeonStats.push_back({ statName, value });
                else if (statName.find("bk_") == 0 or statName.find("bookskill") == 0)
                    bookstats.push_back({ statName, value });
                else
                    otherStats.push_back({ statName, value });
            }

            if (!creatureKills.empty())
            {
                console->printOK("");
                console->printOK("--- Creature Kills ---");
                for (const auto& [name, count] : creatureKills)
                {
                    console->printOK("  " + name + ": " + std::to_string(count));
                }
            }

            if (!npcKills.empty())
            {
                console->printOK("");
                console->printOK("--- NPC Kills ---");
                for (const auto& [name, count] : npcKills)
                {
                    console->printOK("  " + name + ": " + std::to_string(count));
                }
            }

            if (!dungeonStats.empty())
            {
                console->printOK("");
                console->printOK("--- Dungeon Stats ---");
                for (const auto& [name, value] : dungeonStats)
                {
                    console->printOK("  " + name + ": " + std::to_string(value));
                }
            }

            if (!bookstats.empty())
            {
                console->printOK("");
                console->printOK("--- Book Stats ---");
                for (const auto& [name, value] : bookstats)
                {
                    console->printOK("  " + name);
                }
            }

            if (!otherStats.empty())
            {
                console->printOK("");
                console->printOK("--- Other Stats ---");
                for (const auto& [name, value] : otherStats)
                {
                    console->printOK("  " + name + ": " + std::to_string(value));
                }
            }

            return;
        }

        if (command == "ramwclearcreatures" || command == "ramwclearkills") // Testing command - clean up kill counts
        {
            int removedCount = 0;
            auto& allStats = RAStats::getAllStats();

            std::vector<std::string> toRemove;
            for (const auto& [statName, value] : allStats)
            {
                if (statName.find("creature_kill_") == 0)
                {
                    toRemove.push_back(statName);
                }
            }

            for (const std::string& statName : toRemove)
            {
                RAStats::reset(statName);
                removedCount++;
            }

            console->printOK("Cleared " + std::to_string(removedCount) + " creature kill stats");
            return;
        }

        if (command.rfind("ramw_unlock ", 0) == 0) // testing command - unlock specific achievement
        {
            std::string achievementId = command.substr(12);
            achievementId.erase(0, achievementId.find_first_not_of(" \t"));
            achievementId.erase(achievementId.find_last_not_of(" \t") + 1);

            if (achievementId.empty())
            {
                console->printError("Usage: ramw_unlock [ACHIEVEMENT_ID]");
                return;
            }

            const AchievementData* info = RAAchievementManager::getAchievementData(achievementId);
            if (!info)
            {
                console->printError("Achievement not found: " + achievementId);
                return;
            }

            if (info->unlocked)
            {
                console->printOK("Achievement already unlocked: " + achievementId);
                return;
            }

            RAAchievementManager::unlock(achievementId);
            console->printOK("Achievement unlocked: " + achievementId);
            return;
        }

        if (Misc::StringUtils::ciStartsWith(command, "rastat_set ")) // testing command - force a stat value
        {
            std::string args = command.substr(11);
            const std::string statName = nextArgument(args);
            const std::string rawValue = nextArgument(args);

            if (statName.empty() || rawValue.empty())
            {
                console->printError("Usage: rastat_set [stat_name] [value]");
                return;
            }

            int value = 0;
            const auto parsed = std::from_chars(rawValue.data(), rawValue.data() + rawValue.size(), value);

            if (parsed.ec != std::errc() || parsed.ptr != rawValue.data() + rawValue.size())
            {
                console->printError("Not a number: " + rawValue);
                return;
            }

            RAStats::set(statName, value);
            console->printOK("Stat set: " + statName + " = " + std::to_string(value));
            return;
        }

        if (Misc::StringUtils::ciStartsWith(command, "rastat_del ")) // testing command - drop a stat from the save
        {
            std::string args = command.substr(11);
            const std::string statName = nextArgument(args);

            if (statName.empty())
            {
                console->printError("Usage: rastat_del [stat_name]");
                return;
            }

            if (!RAStats::erase(statName))
            {
                console->printError("Stat not found: " + statName);
                return;
            }

            console->printOK("Stat removed: " + statName);
            return;
        }
    }
}
