#include "radiscoverynotification.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Widget.h>

#include <algorithm>

#include <components/debug/debuglog.hpp>

namespace RAMW
{
    namespace
    {
        constexpr float sFadeInTime = 0.6f;
        constexpr float sHoldTime = 2.8f;
        constexpr float sFadeOutTime = 1.2f;
        constexpr float sTotalTime = sFadeInTime + sHoldTime + sFadeOutTime;

        constexpr int sWidth = 800;
        constexpr int sSubtitleHeight = 28;
        constexpr int sTitleHeight = 70;

        const MyGUI::Colour sSubtitleColour(0.75f, 0.70f, 0.55f);
        const MyGUI::Colour sTitleColour(1.0f, 0.93f, 0.75f);
        const MyGUI::Colour sShadowColour(0.0f, 0.0f, 0.0f);
    }

    RADiscoveryNotification* RADiscoveryNotification::sInstance = nullptr;

    RADiscoveryNotification::RADiscoveryNotification()
    {
        createWidgets();
    }

    RADiscoveryNotification::~RADiscoveryNotification()
    {
        destroyWidgets();
    }

    void RADiscoveryNotification::createInstance()
    {
        if (!sInstance)
            sInstance = new RADiscoveryNotification();
    }

    void RADiscoveryNotification::destroyInstance()
    {
        delete sInstance;
        sInstance = nullptr;
    }

    RADiscoveryNotification& RADiscoveryNotification::getInstance()
    {
        createInstance();
        return *sInstance;
    }

    void RADiscoveryNotification::createWidgets()
    {
        MyGUI::Gui& gui = MyGUI::Gui::getInstance();

        mContainer = gui.createWidget<MyGUI::Widget>(
            "PanelEmpty", 0, 0, sWidth, sSubtitleHeight + sTitleHeight,
            MyGUI::Align::Default, "Notification");
        mContainer->setNeedMouseFocus(false);
        mContainer->setVisible(false);

        auto makeText = [&](int y, int height, const MyGUI::Colour& colour, MyGUI::TextBox*& shadowOut,
                            MyGUI::TextBox*& textOut) {
            shadowOut = mContainer->createWidget<MyGUI::TextBox>(
                "SandText", MyGUI::IntCoord(1, y + 2, sWidth, height), MyGUI::Align::Default);
            shadowOut->setTextAlign(MyGUI::Align::HCenter | MyGUI::Align::VCenter);
            shadowOut->setTextColour(sShadowColour);
            shadowOut->setNeedMouseFocus(false);

            textOut = mContainer->createWidget<MyGUI::TextBox>(
                "SandText", MyGUI::IntCoord(0, y, sWidth, height), MyGUI::Align::Default);
            textOut->setTextAlign(MyGUI::Align::HCenter | MyGUI::Align::VCenter);
            textOut->setTextColour(colour);
            textOut->setNeedMouseFocus(false);
        };

        makeText(0, sSubtitleHeight, sSubtitleColour, mSubtitleShadow, mSubtitle);
        makeText(sSubtitleHeight, sTitleHeight, sTitleColour, mTitleShadow, mTitle);

        mSubtitleShadow->setFontName("RAMWTitleFont");
        mSubtitle->setFontName("RAMWTitleFont");

        mSubtitleShadow->setFontHeight(32);
        mSubtitle->setFontHeight(32);
        
        mTitleShadow->setFontName("RAMWTitleFont");
        mTitle->setFontName("RAMWTitleFont");

        mTitleShadow->setFontHeight(64);
        mTitle->setFontHeight(64);

        mSubtitleShadow->setCaption("You have discovered");
        mSubtitle->setCaption("You have discovered");
        Log(Debug::Info) << "[RAMW] title font=" << mTitle->getFontName() << " height=" << mTitle->getFontHeight();
        
    }

    void RADiscoveryNotification::destroyWidgets()
    {
        if (mContainer)
        {
            MyGUI::Gui::getInstance().destroyWidget(mContainer);
            mContainer = nullptr;
        }
        mSubtitle = mSubtitleShadow = mTitle = mTitleShadow = nullptr;
    }

    void RADiscoveryNotification::layout()
    {
        const int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        const int screenHeight = MyGUI::RenderManager::getInstance().getViewSize().height;

        const int x = (screenWidth - sWidth) / 2;
        const int y = static_cast<int>(screenHeight * 0.12f);

        mContainer->setCoord(x, y, sWidth, sSubtitleHeight + sTitleHeight);
    }

    void RADiscoveryNotification::show(const std::string& locationName)
    {
        if (!mContainer)
            return;

        mLocationName = locationName;
        mTitle->setCaption(locationName);
        mTitleShadow->setCaption(locationName);

        layout();

        mTimer = 0.0f;
        mAlpha = 0.0f;
        mActive = true;
        mContainer->setAlpha(0.0f);
        mContainer->setVisible(true);

        Log(Debug::Info) << "[RAMW] Discovered: " << locationName;
    }

    void RADiscoveryNotification::update(float dt)
    {
        if (!mActive || !mContainer)
            return;

        mTimer += dt;

        if (mTimer < sFadeInTime)
            mAlpha = mTimer / sFadeInTime;
        else if (mTimer < sFadeInTime + sHoldTime)
            mAlpha = 1.0f;
        else if (mTimer < sTotalTime)
            mAlpha = 1.0f - (mTimer - sFadeInTime - sHoldTime) / sFadeOutTime;
        else
        {
            mAlpha = 0.0f;
            mActive = false;
            mContainer->setVisible(false);
            return;
        }

        mContainer->setAlpha(std::clamp(mAlpha, 0.0f, 1.0f));
    }
}
