#include "ramods.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "raconfig.hpp"
#include <algorithm>
#include <cctype>
#include <components/debug/debuglog.hpp>

namespace RAMW
{
    // White list of mods allowed in hardcore mode. Should be discuss before release with comunity/admins.
    const std::set<std::string> RAMods::sAllowedMods = {
        "morrowind", "tribunal", "bloodmoon", "builtin", "better bodies", "better heads", "better clothes", "mgexe",
        "watercolored", "accurate attack", "patch for purists", "morrowind code patch", "ui expansion",
        "better dialogue font", "morrowind optimization patch", "Legacy of the Nerevarine.omwaddon",
        //... To define for the release
    };

    std::string RAMods::cleanModName(const std::string& modName)
    {
        std::string normalized = modName;

        size_t dotPos = normalized.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            normalized = normalized.substr(0, dotPos);
        }

        std::transform(
            normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) { return std::tolower(c); });

        return normalized;
    }

    bool RAMods::isModAllowed(const std::string& modName)
    {
        if constexpr (RAMW::RAConfig::TEST_MODE)
            return true;

        std::string normalized = cleanModName(modName);
        return sAllowedMods.find(normalized) != sAllowedMods.end();
    }

    std::vector<std::string> RAMods::getUnauthorizedMods(const std::vector<std::string>& loadedMods)
    {
        std::vector<std::string> unauthorized;

        for (const auto& mod : loadedMods)
        {
            if (!isModAllowed(mod))
            {
                unauthorized.push_back(mod);
            }
        }

        return unauthorized;
    }

    bool RAMods::checkLoadedMods(const std::vector<std::string>& loadedMods)
    {
        auto unauthorized = getUnauthorizedMods(loadedMods);

        if (!unauthorized.empty())
        {
            MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();

            if (!winMgr->isConsoleMode())
                winMgr->toggleConsole();

            winMgr->printToConsole("\n[RAMW] Unauthorized mods detected: ", MWBase::WindowManager::sConsoleColor_Error);

            for (const auto& mod : unauthorized)
                winMgr->printToConsole("\n - " + mod, MWBase::WindowManager::sConsoleColor_Error);

            winMgr->printToConsole("\n[RAMW] Achievements are DISABLED.\n", MWBase::WindowManager::sConsoleColor_Error);

            return false;
        }

        Log(Debug::Info) << "[RAMW] All loaded mods are authorized.";
        MWBase::Environment::get().getWindowManager()->printToConsole(
            "\n[RAMW] All mods are authorized. Achievements enabled!\n", MWBase::WindowManager::sConsoleColor_Error);

        return true;
    }
}
