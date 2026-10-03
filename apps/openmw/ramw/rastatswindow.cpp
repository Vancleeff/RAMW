#include "rastatswindow.hpp"
#include "raeventprocessor.hpp"
#include "rastats.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_TextBox.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"
#include <components/esm/refid.hpp>
#include <components/esm3/loadcrea.hpp>

namespace RAMW
{
    int windowWidth = 300;
    namespace
    {
        std::string getCreatureDisplayName(const std::string& creatureRefIdStr)
        {
            ESM::RefId refId = ESM::RefId::stringRefId(creatureRefIdStr);

            const MWWorld::ESMStore* store = MWBase::Environment::get().getESMStore();
            const ESM::Creature* creature = store->get<ESM::Creature>().search(refId);

            if (creature && !creature->mName.empty())
                return creature->mName;

            return creatureRefIdStr;
        }
    }

    RAStatsWindow::RAStatsWindow()
        : MWGui::WindowPinnableBase("ramw_stats.layout")
        , mScrollView(nullptr)
        , mWantVisible(true)
    {
        createWidgets();
        updateStatsList();
        RAEventProcessor::setOnEventCallback([this]() { this->updateStatsList(); });
    }

    void RAStatsWindow::createWidgets()
    {
        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        int screenHeight = MyGUI::RenderManager::getInstance().getViewSize().height;

        int windowHeight = static_cast<int>(screenHeight * 0.4f);
        int marginFromRight = static_cast<int>((screenWidth * 0.255f) + 540);
        int marginFromTop = static_cast<int>(screenHeight * 0.005f);
        int x = screenWidth - windowWidth - marginFromRight;
        int y = marginFromTop;

        mMainWidget->setCoord(x, y, windowWidth, windowHeight);
        mMainWidget->setVisible(false);

        int titleBarHeight = 10;
        int padding = 10;

        mScrollView = mMainWidget->createWidget<MyGUI::ScrollView>("MW_ScrollView",
            MyGUI::IntCoord(padding, titleBarHeight + 15, windowWidth - (padding * 2),
                windowHeight - titleBarHeight - padding - 15),
            MyGUI::Align::Stretch);

        mScrollView->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        mScrollView->eventMouseWheel += MyGUI::newDelegate(this, &RAStatsWindow::onMouseWheel);
    }

    void RAStatsWindow::updateStatsList()
    {
        Log(Debug::Info) << "updateStatsList";

        if (!mScrollView)
            return;

        mScrollView->setCanvasSize(mScrollView->getWidth(), 0);
        while (mScrollView->getChildCount() > 0)
            MyGUI::Gui::getInstance().destroyWidget(mScrollView->getChildAt(0));

        int yPos = 10;
        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        int contentWidth = static_cast<int>(screenWidth * 0.225f);

        createStatsSection(yPos);
        createSeparator(yPos);
        createKillSection(yPos);

        mScrollView->setCanvasSize(contentWidth, yPos + 10);
    }

    void RAStatsWindow::createKillSection(int& yPos)
    {
        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        int contentWidth = windowWidth;

        MyGUI::TextBox* header = mScrollView->createWidget<MyGUI::TextBox>(
            "SandText", MyGUI::IntCoord(10, yPos, contentWidth - 20, 24), MyGUI::Align::Left | MyGUI::Align::Top);
        header->setCaption("Killing");
        header->setTextColour(MyGUI::Colour(1.0f, 0.84f, 0.0f));
        header->setFontHeight(20);
        yPos += 30;

        const auto& allStats = RAStats::getAllStats();

        // mapping as displayName -> totalKills
        std::map<std::string, int> killsByName;

        for (const auto& [statName, value] : allStats)
        {
            if (statName.find("creature_kill_") == 0)
            {
                std::string creatureRefIdStr = statName.substr(14);
                std::string displayName = getCreatureDisplayName(creatureRefIdStr);

                killsByName[displayName] += value; 
            }
        }

        if (killsByName.empty())
        {
            MyGUI::TextBox* noData = mScrollView->createWidget<MyGUI::TextBox>(
                "SandText", MyGUI::IntCoord(20, yPos, contentWidth - 40, 20), MyGUI::Align::Left | MyGUI::Align::Top);
            noData->setCaption("No creatures killed yet");
            noData->setTextColour(MyGUI::Colour(0.5f, 0.5f, 0.5f));
            yPos += 22;
        }
        else
        {
            for (const auto& [displayName, totalKills] : killsByName)
            {
                MyGUI::TextBox* statLine = mScrollView->createWidget<MyGUI::TextBox>("SandText",
                    MyGUI::IntCoord(20, yPos, contentWidth - 40, 20), MyGUI::Align::Left | MyGUI::Align::Top);

                std::string displayText = displayName + ": " + std::to_string(totalKills);
                statLine->setCaption(displayText);
                statLine->setTextColour(MyGUI::Colour(0.8f, 0.8f, 0.8f));

                yPos += 22;
            }
        }

        yPos += 10;
    }

    void RAStatsWindow::createSeparator(int& yPos)
    {
        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        int contentWidth = windowWidth;

        mScrollView->createWidget<MyGUI::Widget>(
            "MW_HLine", MyGUI::IntCoord(10, yPos, contentWidth - 20, 2), MyGUI::Align::HStretch | MyGUI::Align::Top);

        yPos += 15;
    }

    void RAStatsWindow::createStatsSection(int& yPos)
    {
        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        int contentWidth = windowWidth;

        Log(Debug::Info) << "[RAMW] ScrollView width: " << mScrollView->getWidth()
                         << " canvasSize: " << mScrollView->getCanvasSize().width << " windowWidth var: " << windowWidth
                         << " mainWidget width: " << mMainWidget->getWidth();

        MyGUI::TextBox* header = mScrollView->createWidget<MyGUI::TextBox>(
            "SandText", MyGUI::IntCoord(10, yPos, contentWidth - 20, 24), MyGUI::Align::Left | MyGUI::Align::Top);
        header->setCaption("Stats");
        header->setTextColour(MyGUI::Colour(1.0f, 0.84f, 0.0f));
        header->setFontHeight(20);
        yPos += 30;

        const auto& allStats = RAStats::getAllStats();

        struct StatDisplay
        {
            std::string statKey;
            std::string displayLabel;
        };

        //based on RAstats names
        std::vector<StatDisplay> statsToDisplay = {
            { "quests_finished", "Quests Finished" },
            { "kill_creature_total", "Total Creatures Killed" },
            { "kill_npc_total", "Total NPC Killed" },
            { "lockpick_count", "Container Lockpicked" },
            { "distance_walked", "Distance Walked" },
            { "items_stolen", "Items Stolen" },
            { "gold_looted", "Gold Collected" },
            { "doors_opened", "Doors Opened" },
            { "spells_cast", "Spells Cast" },
            { "books_read", "Books Read" },
            { "skillbooks_read", "Skill Books Read" },
        };

        bool foundAny = false;

        for (const auto& statDisplay : statsToDisplay)
        {
            auto it = allStats.find(statDisplay.statKey);
            if (it != allStats.end())
            {
                foundAny = true;

                MyGUI::TextBox* statLine = mScrollView->createWidget<MyGUI::TextBox>("SandText",
                    MyGUI::IntCoord(20, yPos, contentWidth - 40, 20), MyGUI::Align::Left | MyGUI::Align::Top);

                std::string displayText = statDisplay.displayLabel + ": " + std::to_string(it->second);
                statLine->setCaption(displayText);
                statLine->setTextColour(MyGUI::Colour(0.8f, 0.8f, 0.8f));

                yPos += 22;
            }
        }

        if (!foundAny)
        {
            MyGUI::TextBox* noData = mScrollView->createWidget<MyGUI::TextBox>(
                "SandText", MyGUI::IntCoord(20, yPos, contentWidth - 40, 20), MyGUI::Align::Left | MyGUI::Align::Top);
            noData->setCaption("No stats recorded yet");
            noData->setTextColour(MyGUI::Colour(0.5f, 0.5f, 0.5f));
            yPos += 22;
        }
    }

    void RAStatsWindow::toggle()
    {
        setVisible(!mMainWidget->getVisible());
    }

    void RAStatsWindow::setVisible(bool visible)
    {
        mWantVisible = visible;
        mMainWidget->setVisible(visible);

        if (visible)
            updateStatsList();
    }

    bool RAStatsWindow::isVisible() const
    {
        return mMainWidget->getVisible();
    }

    void RAStatsWindow::update(float dt)
    {
        if (!mMainWidget)
            return;

        MWGui::GuiMode currentMode = MWBase::Environment::get().getWindowManager()->getMode();
        bool inInventoryMode = (currentMode == MWGui::GM_Inventory);

        bool shouldBeVisible = mWantVisible && (mPinned || inInventoryMode);

        if (mMainWidget->getVisible() != shouldBeVisible)
        {
            mMainWidget->setVisible(shouldBeVisible);
        }
    }

    void RAStatsWindow::onPinToggled() {}

    void RAStatsWindow::onMouseWheel(MyGUI::Widget* sender, int rel)
    {
        if (mScrollView->getViewOffset().top + rel * 0.3 > 0)
            mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mScrollView->setViewOffset(
                MyGUI::IntPoint(0, static_cast<int>(mScrollView->getViewOffset().top + rel * 0.3f)));
    }

}
