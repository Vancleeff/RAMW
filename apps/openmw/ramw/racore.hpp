#ifndef RAMW_RACORE_HPP
#define RAMW_RACORE_HPP
#include "achievementlistwindow.hpp"
#include "rastatswindow.hpp"
#include <future> 

namespace RAMW
{
    class AchievementNotification;
    const bool _TEST_MODE = true;

    class RACore
    {
    public:
        static void init(const std::vector<std::string>& contentFiles = {});
        static void onGameReady();
        static void shutdown();
        static void update(float dt);
        static bool isInitialized() { return mInitialized; }

        static void setNotificationWidget(AchievementNotification* widget);
        static AchievementListWindow* getAchievementListWindow();
        static RAStatsWindow* getStatsWindow();

    private:
        static bool mInitialized;
        static bool mSessionStarted;
        static bool mWidgetsCreated;
        static bool mIsAuthenticated;
        static float mHeartbeatTimer;
        static AchievementNotification* mNotificationWidget;
        static std::unique_ptr<AchievementListWindow> sAchievementListWindow;
        static std::unique_ptr<RAStatsWindow> sStatsWindow;
    };

}

#endif
