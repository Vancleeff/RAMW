#include "racore.hpp"
#include "achievementnotification.hpp"
#include "raachievement.hpp"
#include "racelltracker.hpp"
#include "raclient.hpp"
#include "raconfig.hpp"
#include "raeventprocessor.hpp"
#include "ramods.hpp"
#include "rarichpresence.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include <MyGUI_EditBox.h>
#include <MyGUI_InputManager.h>
#include <iostream>

namespace RAMW
{
    bool RACore::mInitialized = false;
    bool RACore::mSessionStarted = false;
    bool RACore::mWidgetsCreated = false;
    float RACore::mHeartbeatTimer = 0.0f;
    AchievementNotification* RACore::mNotificationWidget = nullptr;
    std::unique_ptr<AchievementListWindow> RACore::sAchievementListWindow = nullptr;
    std::unique_ptr<RACellTracker> RACore::sCellTracker;

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
        RAAchievementManager::init();
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
            sCellTracker = std::make_unique<RACellTracker>();
            RADiscoveryNotification::createInstance();
            mWidgetsCreated = true;
            Log(Debug::Info) << "[RAMW] RA widgets created";
        }

        if (RAConfig::TEST_MODE)
            RAAchievementManager::restoreFromStats();

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

        if (sCellTracker)
            sCellTracker->update(dt);

        if (RADiscoveryNotification::hasInstance())
            RADiscoveryNotification::getInstance().update(dt);

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
        sCellTracker.reset();
        AchievementNotification::destroyInstance();
        RADiscoveryNotification::destroyInstance();
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

    void RACore::toggleAchievementWindow()
    {
        if (!sAchievementListWindow)
            return;

        const auto windowManager = MWBase::Environment::get().getWindowManager();

        if (MyGUI::InputManager::getInstance().isModalAny() || windowManager->isConsoleMode())
            return;

        MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        MyGUI::EditBox* edit = focus ? focus->castType<MyGUI::EditBox>(false) : nullptr;
        if (edit && !edit->getEditStatic())
            return;

        const bool wasInOwnMode = windowManager->isGuiMode() && windowManager->getMode() == MWGui::GM_RAMWAchievements;

        if (sAchievementListWindow->pinned())
        {
            if (wasInOwnMode)
                windowManager->popGuiMode();
            else if (!windowManager->isGuiMode())
                windowManager->pushGuiMode(MWGui::GM_RAMWAchievements);

            sAchievementListWindow->setVisible(true);
            return;
        }

        sAchievementListWindow->toggle();

        if (sAchievementListWindow->isVisible())
        {
            if (!windowManager->isGuiMode())
                windowManager->pushGuiMode(MWGui::GM_RAMWAchievements);
        }
        else if (wasInOwnMode)
        {
            windowManager->popGuiMode();
        }
    }
}
