#ifndef RAMW_RADISCOVERYNOTIFICATION_HPP
#define RAMW_RADISCOVERYNOTIFICATION_HPP

#include <string>
#include <vector>

namespace MyGUI
{
    class TextBox;
    class Widget;
}

namespace RAMW
{
    /// Large centred banner announcing a newly discovered location,
    /// in the style of modern open world games.
    class RADiscoveryNotification
    {
    public:
        static void createInstance();
        static void destroyInstance();
        static bool hasInstance() { return sInstance != nullptr; }
        static RADiscoveryNotification& getInstance();

        void show(const std::string& locationName);
        void update(float dt);

    private:
        RADiscoveryNotification();
        ~RADiscoveryNotification();

        void createWidgets();
        void destroyWidgets();
        void layout();

        static RADiscoveryNotification* sInstance;

        MyGUI::Widget* mContainer = nullptr;
        MyGUI::TextBox* mSubtitleShadow = nullptr;
        MyGUI::TextBox* mSubtitle = nullptr;
        MyGUI::TextBox* mTitleShadow = nullptr;
        MyGUI::TextBox* mTitle = nullptr;

        std::string mLocationName;

        float mTimer = 0.0f;
        float mAlpha = 0.0f;
        bool mActive = false;
    };
}

#endif
