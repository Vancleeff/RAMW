#include "racore.hpp"
#include "achievementnotification.hpp"
#include "raachievement.hpp"
#include "raclient.hpp"
#include "raconfig.hpp"
#include "raeventprocessor.hpp"
#include "ramods.hpp"
#include "rarichpresence.hpp"

#include "../mwbase/environment.hpp"
#include <iostream>

namespace RAMW
{
    bool RACore::mInitialized = false;
    bool RACore::mSessionStarted = false;
    bool RACore::mWidgetsCreated = false;
    float RACore::mHeartbeatTimer = 0.0f;
    AchievementNotification* RACore::mNotificationWidget = nullptr;
    std::unique_ptr<AchievementListWindow> RACore::sAchievementListWindow = nullptr;
    std::unique_ptr<RAStatsWindow> RACore::sStatsWindow = nullptr;

    void RACore::init(const std::vector<std::string>& contentFiles)
    {
        if (mInitialized)
            return;

        Log(Debug::Info) << "[RAMW] RA integration init...";

        if (!contentFiles.empty())
        {
            if (!RAMods::checkLoadedMods(contentFiles))
            {
                Log(Debug::Info) << "[RAMW] Blacklisted mod installed, RAMW is shutting down...";
                return;
            }
        }

        try
        {
            RAClient::createInstance();
            RAClient::getInstance().authenticate();
            AchievementNotification::createInstance();
        }
        catch (const std::exception& e)
        {
            Log(Debug::Warning) << "[RAMW] Authentication failed: " << e.what() << " - continuing offline";
        }

        mInitialized = true;
        RAAchievement::init();
        if (!mWidgetsCreated)
            onGameReady();
        Log(Debug::Info) << "[RAMW] RA Integration initialized";
    }

    void RACore::onGameReady()
    {
        if (!mInitialized)
            return;

        RARichPresence::init();
        RAStats::buildProgressIndex();

        if (!mWidgetsCreated)
        {
            sAchievementListWindow = std::make_unique<AchievementListWindow>();
            sStatsWindow = std::make_unique<RAStatsWindow>();
            sAchievementListWindow->setStatsWindow(sStatsWindow.get());
            mWidgetsCreated = true;
            Log(Debug::Info) << "[RAMW] RA widgets created";
        }

        if (RAConfig::TEST_MODE)
            RAAchievement::restoreFromStats();

        sAchievementListWindow->update();
    }

    void RACore::update(float dt)
    {
        if (!mInitialized || !mWidgetsCreated)
            return;

        if (AchievementNotification::hasInstance())
        {
            AchievementNotification::getInstance().update(dt);
        }

        if (getAchievementListWindow())
        {
            getAchievementListWindow()->update();
        }

        if (getStatsWindow())
        {
            getStatsWindow()->update(dt);
        }

        RARichPresence::update();
        RAStats::update(dt);

        if (!mSessionStarted)
        {
            Log(Debug::Info) << "[RAMW] Trying to start session...";
            if (RAClient::getInstance().startSession(682, RAClient::getInstance().getPlayerUsername()))
            {
                Log(Debug::Info) << "[RAMW] Session started successfully!";
                mSessionStarted = true;
                mHeartbeatTimer = 0.0f;

                Log(Debug::Info) << "[RAMW] Sending immediate heartbeat...";
                RAClient::getInstance().sendHeartbeat(
                    682, RAClient::getInstance().getPlayerUsername(), "Playing Morrowind");
            }
        }

        if (mSessionStarted)
        {
            mHeartbeatTimer += dt;

            if (mHeartbeatTimer >= 120.0f)
            {
                Log(Debug::Info) << "[RAMW] 120s elapsed, sending heartbeat...";
                RAClient::getInstance().sendHeartbeat(
                    682, RAClient::getInstance().getPlayerUsername(), "Playing Morrowind");
                mHeartbeatTimer = 0.0f;
            }
        }
    }

    void RACore::shutdown()
    {
        if (!mInitialized)
            return;

        Log(Debug::Info) << "[RAMW] RA Integration shutting down";
        sAchievementListWindow.reset();
        sStatsWindow.reset();
        AchievementNotification::destroyInstance();
        mInitialized = false;
    }

    void RACore::setNotificationWidget(AchievementNotification* widget)
    {
        mNotificationWidget = widget;
    }

    AchievementListWindow* RACore::getAchievementListWindow()
    {
        return sAchievementListWindow.get();
    }

    RAStatsWindow* RACore::getStatsWindow()
    {
        return sStatsWindow.get();
    }
}
