#ifndef RAMW_ACHIEVEMENTNOTIFICATION_HPP
#define RAMW_ACHIEVEMENTNOTIFICATION_HPP

#include <MyGUI_EditBox.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Window.h>
#include <memory>
#include <queue>
#include <string>


namespace RAMW
{

    struct AchievementData;

    class AchievementNotification
    {
    public:
        // Singleton access
        static AchievementNotification& getInstance();

        // Delete copy constructor and assignment
        AchievementNotification(const AchievementNotification&) = delete;
        AchievementNotification& operator=(const AchievementNotification&) = delete;

        void show(const std::string& achievementId);
        void showCustom(const std::string& windowTitle, const std::string& title, const std::string& description, const std::string& iconPath);
        void update(float dt);

        // Lifecycle management
        static void createInstance();
        static void destroyInstance();
        static bool hasInstance() { return sInstance != nullptr; }
        static std::string convertPNGtoMGUI(
            const std::string& textureName, const std::vector<unsigned char>& pngData);
        ~AchievementNotification();


    private:
        AchievementNotification(); // Private constructor

        void loadLayout();
        void destroyWidgets();
        void showNext();
        void animateIn();
        void animateOut();

        

        MyGUI::Window* mMainWidget;
        MyGUI::ImageBox* mIconImage;
        MyGUI::TextBox* mAchievementName;
        MyGUI::EditBox* mDescription;
        MyGUI::TextBox* mPoints;

        std::queue<std::string> mQueue;
        bool mIsShowing;
        float mDisplayTimer;
        float mAnimationTimer;

        enum AnimState
        {
            ANIM_NONE,
            ANIM_SLIDE_IN,
            ANIM_DISPLAY,
            ANIM_SLIDE_OUT
        };

        AnimState mAnimState;

        static const float DISPLAY_DURATION;
        static const float ANIMATION_DURATION;

        // Singleton instance
        static std::unique_ptr<AchievementNotification> sInstance;
    };
}
#endif
