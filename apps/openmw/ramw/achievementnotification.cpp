#include "achievementnotification.hpp"
#include "raachievement.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwsound/constants.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_LayoutManager.h>
#include <MyGUI_RenderManager.h>
#include <components/debug/debuglog.hpp>
#include <iostream>
#include <osg/Image>
#include <osgDB/ReadFile>

#include <sstream>

namespace RAMW
{

    const float AchievementNotification::DISPLAY_DURATION = 5.0f;
    const float AchievementNotification::ANIMATION_DURATION = 0.3f;

    std::unique_ptr<AchievementNotification> AchievementNotification::sInstance = nullptr;

    AchievementNotification& AchievementNotification::getInstance()
    {
        if (!sInstance)
        {
            std::cerr << "[RAMW-Notification] Warning: getInstance() called but instance not created!" << std::endl;
            createInstance();
        }
        return *sInstance;
    }

    void AchievementNotification::createInstance()
    {
        if (sInstance)
        {
            std::cout << "[RAMW-Notification] Instance already exists" << std::endl;
            return;
        }

        std::cout << "[RAMW-Notification] Creating singleton instance" << std::endl;
        sInstance = std::unique_ptr<AchievementNotification>(new AchievementNotification());
    }

    void AchievementNotification::destroyInstance()
    {
        if (!sInstance)
            return;

        std::cout << "[RAMW-Notification] Destroying singleton instance" << std::endl;
        sInstance.reset();
    }

    AchievementNotification::AchievementNotification()
        : mMainWidget(nullptr)
        , mIconImage(nullptr)
        , mAchievementName(nullptr)
        , mDescription(nullptr)
        , mPoints(nullptr)
        , mIsShowing(false)
        , mDisplayTimer(0.0f)
        , mAnimationTimer(0.0f)
        , mAnimState(ANIM_NONE)
    {
        std::cout << "[RAMW-Notification] Constructor called" << std::endl;
        loadLayout();
    }

    AchievementNotification::~AchievementNotification()
    {
        destroyWidgets();
    }

    void AchievementNotification::loadLayout()
    {
        if (MyGUI::Gui::getInstancePtr() == nullptr)
        {
            std::cerr << "[RAMW-Notification] MyGUI not ready yet" << std::endl;
            return;
        }

        try
        {
            std::cout << "[RAMW-Notification] Creating widgets programmatically..." << std::endl;

            int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
            int screenHeight = MyGUI::RenderManager::getInstance().getViewSize().height;

            int windowWidth = 400;
            int windowHeight = 120;

            mMainWidget = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("MW_Window",
                MyGUI::IntCoord(screenWidth, screenHeight - windowHeight - 20, windowWidth, windowHeight),
                MyGUI::Align::Default, "Notification", "AchievementWindow");

            if (!mMainWidget)
            {
                std::cerr << "[RAMW-Notification] Failed to create main window" << std::endl;
                return;
            }

            mMainWidget->setCaption("Achievement Unlocked");
            mMainWidget->setVisible(false);

            // Content panel
            MyGUI::Widget* contentPanel = mMainWidget->createWidget<MyGUI::Widget>(
                "PanelEmpty", MyGUI::IntCoord(8, 0, 384, 80), MyGUI::Align::Stretch);

            // Icon (64x64)
            mIconImage = contentPanel->createWidget<MyGUI::ImageBox>(
                "ImageBox", MyGUI::IntCoord(10, 8, 64, 64), MyGUI::Align::Left | MyGUI::Align::Top);
            mIconImage->setImageTexture("textures/tx_goldicon.dds");

            // Achievement name
            mAchievementName = contentPanel->createWidget<MyGUI::TextBox>("NormalText", MyGUI::IntCoord(84, 8, 320, 20),
                MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
            mAchievementName->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
            mAchievementName->setFontHeight(16);
            mAchievementName->setTextColour(MyGUI::Colour(1.0f, 0.84f, 0.0f));

            // Description
            mDescription = contentPanel->createWidget<MyGUI::EditBox>("NormalText", MyGUI::IntCoord(84, 34, 300, 40),
                MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
            mDescription->setEditStatic(true);
            mDescription->setEditMultiLine(true);
            mDescription->setEditWordWrap(true);
            mDescription->setFontHeight(14);
            mDescription->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
            mDescription->setTextColour(MyGUI::Colour(0.8f, 0.8f, 0.8f));

            // Points
            mPoints = contentPanel->createWidget<MyGUI::TextBox>("NormalText", MyGUI::IntCoord(60, 60, 300, 20),
                MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
            mPoints->setTextAlign(MyGUI::Align::Right | MyGUI::Align::Top);
            mPoints->setFontHeight(16);
            mPoints->setTextColour(MyGUI::Colour(0.7f, 0.7f, 0.7f));

            std::cout << "[RAMW-Notification] Widgets created successfully" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[RAMW-Notification] Error creating widgets: " << e.what() << std::endl;
        }
    }

    void AchievementNotification::destroyWidgets()
    {
        MyGUI::Widget* widgetToDestroy = mMainWidget;
        mMainWidget = nullptr;
        mIconImage = nullptr;
        mAchievementName = nullptr;
        mDescription = nullptr;
        mPoints = nullptr;

        if (widgetToDestroy && MyGUI::Gui::getInstancePtr() != nullptr)
        {
            try
            {
                MyGUI::Gui::getInstance().destroyWidget(widgetToDestroy);
            }
            catch (const std::exception& e)
            {
                std::cerr << "[RAMW-Notification] Error destroying widget: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "[RAMW-Notification] Unknown error destroying widget" << std::endl;
            }
        }
    }

    void AchievementNotification::show(const std::string& achievementId)
    {
        std::cout << "[RAMW-Notification] Queuing achievement: " << achievementId << std::endl;

        mQueue.push(achievementId);

        if (!mIsShowing)
        {
            showNext();
        }
    }

    void AchievementNotification::showCustom(const std::string& windowTitle, const std::string& title,
        const std::string& description, const std::string& iconPath)
    {
        Log(Debug::Info) << "[RAMW-Notification] Showing custom notification: " << title;

        if (!mMainWidget || !mAchievementName || !mDescription || !mPoints)
        {
            Log(Debug::Error) << "[RAMW-Notification] Widgets not initialized";
            return;
        }

        mMainWidget->setCaption(windowTitle);
        mAchievementName->setCaption(title);
        mDescription->setCaption(description);

        if (!iconPath.empty() && mIconImage)
        {
            try
            {
                mIconImage->setImageTexture(iconPath);
            }
            catch (const std::exception& e)
            {
                Log(Debug::Error) << "[RAMW-Notification] Failed to load icon: " << e.what();
            }
        }
        else
        {

            mIconImage->setImageTexture("icons\\k\\magic_enchant.dds");
        }

        mIsShowing = true;
        mAnimState = ANIM_SLIDE_IN;
        mAnimationTimer = 0.0f;
        mDisplayTimer = 0.0f;

        MWBase::Environment::get().getSoundManager()->playSound(
            ESM::RefId::stringRefId("skillraise"), 1.0f, 1.0f, MWSound::Type::Sfx, MWSound::PlayMode::NoEnvNoScaling);

        animateIn();
    }

    void AchievementNotification::showNext()
    {
        if (mQueue.empty())
        {
            mIsShowing = false;
            return;
        }
        MWBase::Environment::get().getSoundManager()->playSound(
            ESM::RefId::stringRefId("skillraise"), 1.0f, 1.0f, MWSound::Type::Sfx, MWSound::PlayMode::NoEnvNoScaling);
        std::string achievementId = mQueue.front();
        mQueue.pop();

        const AchievementData* info = RAAchievementManager::getAchievementData(achievementId);
        if (!info)
        {
            std::cerr << "[RA-Notification] Unknown achievement: " << achievementId << std::endl;
            showNext();
            return;
        }

        if (!mMainWidget || !mAchievementName || !mDescription || !mPoints)
        {
            std::cerr << "[RA-Notification] Widgets not initialized" << std::endl;
            return;
        }

        mMainWidget->setCaption("Achievement Unlocked");
        mAchievementName->setCaption(info->name);
        mDescription->setCaption(info->description);
        mPoints->setCaption("Points: " + std::to_string(info->points));

        if (mIconImage)
        {
            mIconImage->setImageTexture("icons\\k\\magic_enchant.dds");
        }

        mIsShowing = true;
        mAnimState = ANIM_SLIDE_IN;
        mAnimationTimer = 0.0f;
        mDisplayTimer = 0.0f;

        animateIn();
    }

    void AchievementNotification::animateIn()
    {
        if (!mMainWidget)
            return;

        mMainWidget->setVisible(true);

        // Start position (off-screen to the right)
        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        int screenHeight = MyGUI::RenderManager::getInstance().getViewSize().height;
        int windowHeight = mMainWidget->getHeight();

        mMainWidget->setPosition(screenWidth, screenHeight - windowHeight - 20);
    }

    void AchievementNotification::animateOut()
    {
        mAnimState = ANIM_SLIDE_OUT;
        mAnimationTimer = 0.0f;
    }

    void AchievementNotification::update(float dt)
    {
        if (!mIsShowing || !mMainWidget)
            return;

        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        int screenHeight = MyGUI::RenderManager::getInstance().getViewSize().height;
        int windowWidth = mMainWidget->getWidth();
        int windowHeight = mMainWidget->getHeight();

        switch (mAnimState)
        {
            case ANIM_SLIDE_IN:
            {
                mAnimationTimer += dt;
                float progress = std::min(1.0f, mAnimationTimer / ANIMATION_DURATION);
                float eased = progress * progress * (3.0f - 2.0f * progress);

                int targetX = screenWidth - windowWidth - 20;
                int startX = screenWidth;
                int currentX = startX + static_cast<int>((targetX - startX) * eased);

                mMainWidget->setPosition(currentX, screenHeight - windowHeight - 20);

                if (progress >= 1.0f)
                {
                    mAnimState = ANIM_DISPLAY;
                    mDisplayTimer = 0.0f;
                }
                break;
            }

            case ANIM_DISPLAY:
            {
                mDisplayTimer += dt;
                if (mDisplayTimer >= DISPLAY_DURATION)
                {
                    animateOut();
                }
                break;
            }

            case ANIM_SLIDE_OUT:
            {
                mAnimationTimer += dt;
                float progress = std::min(1.0f, mAnimationTimer / ANIMATION_DURATION);

                float eased = progress * progress * (3.0f - 2.0f * progress);

                // Slide from visible to off-screen right
                int startX = screenWidth - windowWidth - 20;
                int targetX = screenWidth;
                int currentX = startX + static_cast<int>((targetX - startX) * eased);

                mMainWidget->setPosition(currentX, screenHeight - windowHeight - 20);

                if (progress >= 1.0f)
                {
                    mMainWidget->setVisible(false);
                    mAnimState = ANIM_NONE;
                    showNext();
                }
                break;
            }

            default:
                break;
        }
    }

    //convert png from RA to MGUI Texture for user/achievements icons
    //right now, only users are using this function cause testing achievement uses random fake icons
    std::string AchievementNotification::convertPNGtoMGUI(
        const std::string& textureName, const std::vector<unsigned char>& pngData)
    {
        std::string tempPath = "temp_download.png";
        std::ofstream tempFile(tempPath, std::ios::binary);
        if (!tempFile.is_open())
        {
            Log(Debug::Error) << "[RAMW] Failed to create user icon temp file";
            return "";
        }

        tempFile.write(reinterpret_cast<const char*>(pngData.data()), pngData.size());
        tempFile.close();

        osg::ref_ptr<osg::Image> image = osgDB::readImageFile(tempPath);
        std::remove(tempPath.c_str());

        if (!image.valid())
        {
            Log(Debug::Error) << "[RAMW] Failed to load user icon PNG with OSG";
            return "";
        }

        image->flipVertical();
        int width = image->s();
        int height = image->t();
        int numPixels = width * height;

        std::vector<unsigned char> rgbaData;

        if (image->getPixelFormat() == GL_RGB)
        {
            rgbaData.resize(numPixels * 4);

            const unsigned char* srcData = image->data();
            for (int i = 0; i < numPixels; ++i)
            {
                rgbaData[i * 4 + 0] = srcData[i * 3 + 0]; // R
                rgbaData[i * 4 + 1] = srcData[i * 3 + 1]; // G
                rgbaData[i * 4 + 2] = srcData[i * 3 + 2]; // B
                rgbaData[i * 4 + 3] = 255; // A (opaque)
            }
        }
        else if (image->getPixelFormat() == GL_RGBA)
        {
            rgbaData.assign(image->data(), image->data() + image->getTotalSizeInBytes());
        }
        else
        {
            Log(Debug::Error) << "[RAMW] User icon - Unsupported pixel format: " << image->getPixelFormat();
            return "";
        }

        MyGUI::ITexture* tex = MyGUI::RenderManager::getInstance().createTexture(textureName);
        tex->createManual(width, height, MyGUI::TextureUsage::Write, MyGUI::PixelFormat::R8G8B8A8);

        unsigned char* data = reinterpret_cast<unsigned char*>(tex->lock(MyGUI::TextureUsage::Write));
        memcpy(data, rgbaData.data(), rgbaData.size());
        tex->unlock();

        Log(Debug::Info) << "[RAMW] User icon - Created texture: " << textureName;
        return textureName;
    }

}
