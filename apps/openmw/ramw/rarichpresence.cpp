#include "rarichpresence.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadrace.hpp>

#include <cmath>
#include <iostream>
#include <sstream>

namespace RAMW
{

    std::string RARichPresence::mCachedRP = "";
    float RARichPresence::mUpdTimer = 0.0f;
    const float RARichPresence::UPDATE_INTERVAL = 10.0f;

    void RARichPresence::init()
    {
        std::cout << "[RAMW] RP init" << std::endl;
        mCachedRP = "Starting game...";
    }

    void RARichPresence::update()
    {
        mUpdTimer += 0.016f;

        if (mUpdTimer >= UPDATE_INTERVAL)
        {
            mCachedRP = format();
            display();
            mUpdTimer = 0.0f;
        }
    }

    std::string RARichPresence::getRP()
    {
        return mCachedRP;
    }

    void RARichPresence::display()
    {
        std::cout << "\n";
        std::cout << "========================================" << std::endl;
        std::cout << " RetroAchievements Rich Presence:" << std::endl;
        std::cout << mCachedRP << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\n";
    }

    std::string RARichPresence::format()
    {
        try
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();
            MWWorld::Ptr player = world->getPlayerPtr();
            const auto& stats = player.getClass().getCreatureStats(player);

            const ESM::Race* race = world->getStore().get<ESM::Race>().find(player.get<ESM::NPC>()->mBase->mRace);

            std::ostringstream presence;
            presence << race->mName << " level " << stats.getLevel() << " - "
                     << std::lround(stats.getHealth().getCurrent()) << "/"
                     << std::lround(stats.getHealth().getModified()) << " HP | "
                     << std::lround(stats.getMagicka().getCurrent()) << "/"
                     << std::lround(stats.getMagicka().getModified()) << " MP - "
                     << world->getCellName(player.getCell());
            return presence.str();
        }
        catch (const std::exception& e)
        {
            std::cerr << "[RAMW] Error formatting RP: " << e.what() << std::endl;
            return "Error getting player status";
        }
    }
}
