#include "achievementlistwindow.hpp"
#include "raachievement.hpp"
#include "raeventprocessor.hpp"
#include "rastats.hpp"
#include "rastatswindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_ProgressBar.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_TextBox.h>
#include <curl/curl.h>

#include <iostream>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

namespace RAMW
{
    int tileWidth = 480;

    AchievementListWindow::AchievementListWindow()
        : MWGui::WindowPinnableBase("ramw_achievement_list.layout")
        , mScrollView(nullptr)
        , mSearchBox(nullptr)
        , mWantVisible(true)
        , mCurrentCategory(RAAchievementManager::Category_All)
        , mNameFilter("")
        , mTooltipWidget(nullptr)
        , mStatsWindow(nullptr)
    {
        createWidgets();
        updateCategoryBtnStates();
        updateCounter();
        updateAchievementList();

        RAAchievementManager::setOnUnlockCallback([this]() { this->updateAchievementList(); });
        RAEventProcessor::setOntrackedCallback([this]() { this->updateAchievementList(); });
    }

    void AchievementListWindow::createWidgets()
    {
        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        int screenHeight = MyGUI::RenderManager::getInstance().getViewSize().height;

        int achievementWidth = static_cast<int>(540);
        int achievementHeight = static_cast<int>(screenHeight * 0.94f);

        int marginFromRight = static_cast<int>(screenWidth * 0.255f);
        int marginFromTop = static_cast<int>(screenHeight * 0.005f);

        int x = screenWidth - achievementWidth - marginFromRight;
        int y = marginFromTop;

        mMainWidget->setCoord(x, y, achievementWidth, achievementHeight);
        mMainWidget->setVisible(false);

        int titleBarHeight = 10;
        int padding = 10;
        int buttonBarHeight = 30;
        int searchBarHeight = 26;

        int searchY = titleBarHeight + 5;

        MyGUI::TextBox* searchLabel = mMainWidget->createWidget<MyGUI::TextBox>(
            "NormalText", MyGUI::IntCoord(padding, searchY + 4, 60, 20), MyGUI::Align::Left | MyGUI::Align::Top);
        searchLabel->setCaption("search :");
        searchLabel->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);

        mSearchBox = mMainWidget->createWidget<MyGUI::EditBox>(
            "MW_TextEdit", MyGUI::IntCoord(padding + 65, searchY, 216, 22), MyGUI::Align::HStretch | MyGUI::Align::Top);
        mSearchBox->setCaption("");
        mSearchBox->eventEditTextChange += MyGUI::newDelegate(this, &AchievementListWindow::onSearchTextChange);

        mStatsButton = mMainWidget->createWidget<MyGUI::Button>(
            "MW_Button", MyGUI::IntCoord(padding + 418, searchY, 50, 22), MyGUI::Align::Right | MyGUI::Align::Top);
        mStatsButton->setCaption("Stats");
        mStatsButton->eventMouseButtonClick += MyGUI::newDelegate(this, &AchievementListWindow::onStatsBtnClick);

        int buttonWidth = (achievementWidth - (padding * 4)) / 5;
        int buttonY = titleBarHeight + searchBarHeight + 10;

        mButtonAll = mMainWidget->createWidget<MyGUI::Button>("MW_Button",
            MyGUI::IntCoord(padding, buttonY, buttonWidth - 2, 24), MyGUI::Align::Left | MyGUI::Align::Top);
        mButtonAll->setCaption("All");
        mButtonAll->eventMouseButtonClick += MyGUI::newDelegate(this, &AchievementListWindow::onCategoryBtnClick);

        mButtonStory = mMainWidget->createWidget<MyGUI::Button>("MW_Button",
            MyGUI::IntCoord(padding + buttonWidth, buttonY, buttonWidth - 2, 24),
            MyGUI::Align::Left | MyGUI::Align::Top);
        mButtonStory->setCaption("Quest");
        mButtonStory->eventMouseButtonClick += MyGUI::newDelegate(this, &AchievementListWindow::onCategoryBtnClick);

        mButtonExploration = mMainWidget->createWidget<MyGUI::Button>("MW_Button",
            MyGUI::IntCoord(padding + buttonWidth * 2, buttonY, buttonWidth - 2, 24),
            MyGUI::Align::Left | MyGUI::Align::Top);
        mButtonExploration->setCaption("Explor.");
        mButtonExploration->eventMouseButtonClick
            += MyGUI::newDelegate(this, &AchievementListWindow::onCategoryBtnClick);

        mButtonCombat = mMainWidget->createWidget<MyGUI::Button>("MW_Button",
            MyGUI::IntCoord(padding + buttonWidth * 3, buttonY, buttonWidth - 2, 24),
            MyGUI::Align::Left | MyGUI::Align::Top);
        mButtonCombat->setCaption("Combat");
        mButtonCombat->eventMouseButtonClick += MyGUI::newDelegate(this, &AchievementListWindow::onCategoryBtnClick);

        mButtonMisc = mMainWidget->createWidget<MyGUI::Button>("MW_Button",
            MyGUI::IntCoord(padding + buttonWidth * 4, buttonY, buttonWidth - 2, 24),
            MyGUI::Align::Left | MyGUI::Align::Top);
        mButtonMisc->setCaption("Misc");
        mButtonMisc->eventMouseButtonClick += MyGUI::newDelegate(this, &AchievementListWindow::onCategoryBtnClick);

        mScrollView = mMainWidget->createWidget<MyGUI::ScrollView>("MW_ScrollView",
            MyGUI::IntCoord(padding, titleBarHeight + searchBarHeight + buttonBarHeight + 15,
                achievementWidth - (padding * 2),
                achievementHeight - titleBarHeight - searchBarHeight - buttonBarHeight - padding - 15),
            MyGUI::Align::Stretch);

        mScrollView->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
    }

    void AchievementListWindow::onSearchTextChange(MyGUI::EditBox* sender)
    {
        mNameFilter = sender->getCaption();
        updateAchievementList();
    }

    void AchievementListWindow::onCategoryBtnClick(MyGUI::Widget* sender)
    {
        if (sender == mButtonAll)
            setCategory(RAAchievementManager::Category_All);
        else if (sender == mButtonStory)
            setCategory(RAAchievementManager::Category_Quest);
        else if (sender == mButtonExploration)
            setCategory(RAAchievementManager::Category_Exploration);
        else if (sender == mButtonCombat)
            setCategory(RAAchievementManager::Category_Combat);
        else if (sender == mButtonMisc)
            setCategory(RAAchievementManager::Category_Misc);
    }

    void AchievementListWindow::setCategory(int category)
    {
        mCurrentCategory = category;
        updateCategoryBtnStates();
        updateAchievementList();
    }

    void AchievementListWindow::updateCategoryBtnStates()
    {
        mButtonAll->setStateSelected(false);
        mButtonStory->setStateSelected(false);
        mButtonExploration->setStateSelected(false);
        mButtonCombat->setStateSelected(false);
        mButtonMisc->setStateSelected(false);

        if (mCurrentCategory == RAAchievementManager::Category_All)
            mButtonAll->setStateSelected(true);
        else if (mCurrentCategory == RAAchievementManager::Category_Quest)
            mButtonStory->setStateSelected(true);
        else if (mCurrentCategory == RAAchievementManager::Category_Exploration)
            mButtonExploration->setStateSelected(true);
        else if (mCurrentCategory == RAAchievementManager::Category_Combat)
            mButtonCombat->setStateSelected(true);
        else if (mCurrentCategory == RAAchievementManager::Category_Misc)
            mButtonMisc->setStateSelected(true);
    }

    void AchievementListWindow::updateCounter()
    {
        const auto& achievements = RAAchievementManager::getAllAchievements();

        int totalCount = 0;
        int unlockedCount = 0;

        for (const auto& [id, ach] : achievements)
        {
            totalCount++;
            if (ach.unlocked)
                unlockedCount++;
        }

        std::string title = "Achievement List | " + std::to_string(unlockedCount) + "/" + std::to_string(totalCount);
        mMainWidget->castType<MyGUI::Window>()->setCaptionWithReplacing(title);
    }

    void AchievementListWindow::updateAchievementList()
    {
        Log(Debug::Info) << "[RA Events] updateAchievementList";

        while (mScrollView->getChildCount() > 0)
            MyGUI::Gui::getInstance().destroyWidget(mScrollView->getChildAt(0));

        const auto& achievements = RAAchievementManager::getAllAchievements();

        int yPos = 0;
        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;

        std::map<std::string, std::vector<const AchievementData*>> groups;

        for (const auto& [id, ach] : achievements)
        {
            if (!(ach.category & mCurrentCategory))
                continue;

            if (!mNameFilter.empty())
            {
                std::string lowerName = ach.name;
                std::string lowerDesc = ach.description;
                std::string lowerFilter = mNameFilter;

                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);
                std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

                if (lowerName.find(lowerFilter) == std::string::npos
                    && lowerDesc.find(lowerFilter) == std::string::npos)
                    continue;
            }

            std::string groupKey = ach.parent_id.empty() ? ach.id : ach.parent_id;
            groups[groupKey].push_back(&ach);
        }

        for (auto& [key, achList] : groups)
        {
            std::sort(achList.begin(), achList.end(),
                [](const AchievementData* a, const AchievementData* b) { return a->order < b->order; });
        }

        std::vector<std::pair<std::string, std::vector<const AchievementData*>>> sortedGroups(
            groups.begin(), groups.end());

        std::sort(sortedGroups.begin(), sortedGroups.end(),
            [](const auto& a, const auto& b) { return a.second[0]->order < b.second[0]->order; });

        for (const auto& [groupKey, groupAchievements] : sortedGroups)
        {
            const AchievementData* displayAch = nullptr;

            if (groupAchievements.size() == 1)
            {
                displayAch = groupAchievements[0];
            }
            else
            {
                for (const AchievementData* ach : groupAchievements)
                {
                    if (!ach->unlocked)
                    {
                        displayAch = ach;
                        break;
                    }
                }
                if (!displayAch)
                    displayAch = groupAchievements.back();
            }

            if (displayAch)
                createAchievementTile(displayAch, groupAchievements, yPos);
        }

        mScrollView->setCanvasSize(tileWidth + 10, yPos + 30);
        updateCounter();
    }

    void AchievementListWindow::createAchievementTile(
        const AchievementData* mainAchievement, const std::vector<const AchievementData*>& groupAchievements, int& yPos)
    {
        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        int tileHeight;
        if (mainAchievement->hasProgressBar())
        {
            tileHeight = (groupAchievements.size() > 1) ? 160 : 120;
        }
        else if (mainAchievement->hasObjectives() && mainAchievement->objectives.size() > 1)
        {
            int objectiveHeight = mainAchievement->objectives.size() * 20;
            tileHeight = 60 + objectiveHeight + 10;
        }
        else
            tileHeight = (groupAchievements.size() > 1) ? 120 : 80;

        MyGUI::Widget* tile
            = mScrollView->createWidget<MyGUI::Widget>("PanelEmpty", MyGUI::IntCoord(0, yPos, tileWidth, tileHeight),
                MyGUI::Align::Center | MyGUI::Align::Top | MyGUI::Align::HStretch);

        tile->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);
        float alpha = mainAchievement->unlocked ? 1.0f : 0.4f;
        MyGUI::Colour textColour
            = mainAchievement->unlocked ? MyGUI::Colour(1.0f, 0.84f, 0.0f) : MyGUI::Colour(1.0f, 0.84f, 0.0f, 0.5f);
        MyGUI::Colour descColour
            = mainAchievement->unlocked ? MyGUI::Colour(0.8f, 0.8f, 0.8f) : MyGUI::Colour(0.5f, 0.5f, 0.5f);
        MyGUI::Colour pointsColour
            = mainAchievement->unlocked ? MyGUI::Colour(0.7f, 0.7f, 0.7f) : MyGUI::Colour(0.4f, 0.4f, 0.4f);

        MyGUI::ImageBox* icon = tile->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(5, 8, 64, 64), MyGUI::Align::Left | MyGUI::Align::Top);
        icon->setImageTexture(mainAchievement->iconPath);
        icon->setAlpha(alpha);
        icon->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);

        if (groupAchievements.size() > 1)
        {
            int unlockedCount = 0;
            for (const AchievementData* ach : groupAchievements)
            {
                if (ach->unlocked)
                    unlockedCount++;
            }

            MyGUI::TextBox* rankLabel = tile->createWidget<MyGUI::TextBox>(
                "NormalText", MyGUI::IntCoord(5, 84, 64, 24), MyGUI::Align::Left | MyGUI::Align::Top);

            std::string rankText = std::to_string(unlockedCount) + "/" + std::to_string(groupAchievements.size());
            rankLabel->setCaption(rankText);
            rankLabel->setTextAlign(MyGUI::Align::Center | MyGUI::Align::Top);
            rankLabel->setFontHeight(20);

            MyGUI::Colour rankColour
                = unlockedCount > 0 ? MyGUI::Colour(1.0f, 0.84f, 0.0f) : MyGUI::Colour(0.5f, 0.5f, 0.5f);
            rankLabel->setTextColour(rankColour);
            rankLabel->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);
        }

        MyGUI::Button* checkbox = tile->createWidget<MyGUI::Button>(
            "MW_Button", MyGUI::IntCoord(tileWidth - 30, 30, 25, 25), MyGUI::Align::Left | MyGUI::Align::Top);
        checkbox->setCaption(mainAchievement->unlocked ? "X" : "");
        checkbox->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);

        MyGUI::TextBox* name
            = tile->createWidget<MyGUI::TextBox>("NormalText", MyGUI::IntCoord(75, 8, tileWidth - 130, 20),
                MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
        name->setCaption(mainAchievement->name);
        name->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        name->setFontHeight(16);
        name->setTextColour(textColour);
        name->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);

        MyGUI::EditBox* description = tile->createWidget<MyGUI::EditBox>(
            "NormalText", MyGUI::IntCoord(75, 30, tileWidth - 130, 80), MyGUI::Align::Left | MyGUI::Align::Top);
        description->setEditStatic(true);
        description->setEditMultiLine(true);
        description->setEditWordWrap(true);
        description->setCaption(mainAchievement->description);
        description->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        description->setMaxTextLength(200);
        description->setFontHeight(14);
        description->setTextColour(descColour);
        description->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);

        MyGUI::TextBox* points = tile->createWidget<MyGUI::TextBox>(
            "NormalText", MyGUI::IntCoord(tileWidth - 60, 58, 55, 18), MyGUI::Align::Left | MyGUI::Align::Top);
        points->setCaption(std::to_string(mainAchievement->points) + " pts");
        points->setTextAlign(MyGUI::Align::Right | MyGUI::Align::Top);
        points->setFontHeight(15);
        points->setTextColour(pointsColour);
        points->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);

        if (groupAchievements.size() > 1)
        {
            const int MINI_SIZE = 24;
            const int SPACING = 5;
            int startX = 75;
            int startY = 84;

            for (size_t i = 0; i < groupAchievements.size(); ++i)
            {
                const AchievementData* ach = groupAchievements[i];

                MyGUI::ImageBox* miniIcon = tile->createWidget<MyGUI::ImageBox>("ImageBox",
                    MyGUI::IntCoord(startX + i * (MINI_SIZE + SPACING), startY, MINI_SIZE, MINI_SIZE),
                    MyGUI::Align::Default);

                if (!ach->iconPath.empty())
                    miniIcon->setImageTexture(ach->iconPath);

                miniIcon->setAlpha(ach->unlocked ? 1.0f : 0.3f);
                miniIcon->setNeedMouseFocus(true);
                miniIcon->setUserString("achievementId", ach->id);
                miniIcon->eventMouseSetFocus += MyGUI::newDelegate(this, &AchievementListWindow::onMiniIconMouseEnter);
                miniIcon->eventMouseLostFocus += MyGUI::newDelegate(this, &AchievementListWindow::onMiniIconMouseLeave);
                miniIcon->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);
            }
        }

        if (mainAchievement->hasProgressBar())
        {
            int progressBarY;
            int progressBarWidth = 300;

            if (groupAchievements.size() > 1)
            {
                progressBarY = 124;
            }
            else
            {
                progressBarY = 84;
            }

            MyGUI::ProgressBar* progressBar = tile->createWidget<MyGUI::ProgressBar>("MW_Progress_Blue",
                MyGUI::IntCoord(75, progressBarY, progressBarWidth, 18), MyGUI::Align::Left | MyGUI::Align::Top);

            progressBar->setProgressRange(mainAchievement->target_value);
            progressBar->setProgressPosition(mainAchievement->getProgress());
            progressBar->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);

            MyGUI::TextBox* progressText = tile->createWidget<MyGUI::TextBox>(
                "SandText", MyGUI::IntCoord(0, progressBarY - 5, 64, 24), MyGUI::Align::Left | MyGUI::Align::Top);

            std::string progressCaption
                = std::to_string(mainAchievement->getProgress()) + "/" + std::to_string(mainAchievement->target_value);
            progressText->setCaption(progressCaption);
            progressText->setTextAlign(MyGUI::Align::Right | MyGUI::Align::VCenter);
            progressText->setTextColour(MyGUI::Colour(0.8f, 0.8f, 0.8f));
            progressText->setFontHeight(18);
        }

        if (mainAchievement->hasObjectives() && mainAchievement->objectives.size() > 1)
        {
            int objectiveY = 60;

            for (const auto& objective : mainAchievement->objectives)
            {
                bool completed = mainAchievement->trigger && mainAchievement->trigger->isObjectiveCompleted(objective);

                MyGUI::TextBox* checkbox = tile->createWidget<MyGUI::TextBox>(
                    "SandText", MyGUI::IntCoord(84, objectiveY, 20, 18), MyGUI::Align::Left | MyGUI::Align::Top);

                checkbox->setCaption(completed ? "[X]" : "[ ]");
                checkbox->setTextColour(completed ? MyGUI::Colour(1.0f, 0.84f, 0.0f) : MyGUI::Colour(0.4f, 0.4f, 0.4f));
                checkbox->setFontHeight(14);
                checkbox->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);

                MyGUI::TextBox* label = tile->createWidget<MyGUI::TextBox>("SandText",
                    MyGUI::IntCoord(110, objectiveY, tileWidth - 45, 18), MyGUI::Align::Left | MyGUI::Align::Top);

                int stat = RAStats::get(objective.stat_name);
                label->setCaption(objective.display_label
                    + (objective.amount > 1
                            ? " " + (stat > objective.amount ? std::to_string(objective.amount) : std::to_string(stat))
                                + "/" + std::to_string(objective.amount)
                            : ""));
                if (stat > 0 and objective.amount > 1 and !completed)
                    label->setTextColour(MyGUI::Colour(1.0f, 0.84f, 0.0f, 0.5f));
                else
                    label->setTextColour(
                        completed ? MyGUI::Colour(1.0f, 0.84f, 0.0f) : MyGUI::Colour(0.4f, 0.4f, 0.4f));
                label->setFontHeight(14);
                label->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);

                objectiveY += 20;
            }
        }

        yPos += tileHeight + 5;

        MyGUI::ImageBox* separator
            = mScrollView->createWidget<MyGUI::ImageBox>("MW_HLine", MyGUI::IntCoord(0, yPos, tileWidth - 40, 18),
                MyGUI::Align::Center | MyGUI::Align::Top | MyGUI::Align::HStretch);
        separator->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);
        yPos += 14;
    }

    void AchievementListWindow::toggle()
    {
        mWantVisible = !mWantVisible;
    }

    void AchievementListWindow::setVisible(bool visible)
    {
        mWantVisible = visible;
    }

    bool AchievementListWindow::isVisible() const
    {
        return mWantVisible;
    }

    void AchievementListWindow::update()
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
        updateStatsBtnState();
    }

    void AchievementListWindow::onPinToggled() {}

    void AchievementListWindow::onMiniIconMouseEnter(MyGUI::Widget* sender, MyGUI::Widget* old)
    {
        std::string achId = std::string(sender->getUserString("achievementId"));
        const AchievementData* ach = RAAchievementManager::getAchievementData(achId);

        if (!ach)
            return;

        if (!mTooltipWidget)
        {
            mTooltipWidget = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>(
                "HUD_Box_NoTransp", MyGUI::IntCoord(0, 0, 280, 68), MyGUI::Align::Default, "Popup");
        }
        mTooltipWidget->setAlpha(1.0f);

        while (mTooltipWidget->getChildCount() > 0)
            MyGUI::Gui::getInstance().destroyWidget(mTooltipWidget->getChildAt(0));

        float alpha = 1.0f;
        MyGUI::Colour textColour = MyGUI::Colour(1.0f, 0.84f, 0.0f);
        MyGUI::Colour descColour = MyGUI::Colour(0.8f, 0.8f, 0.8f);
        MyGUI::Colour pointsColour = MyGUI::Colour(0.7f, 0.7f, 0.7f);

        MyGUI::ImageBox* icon = mTooltipWidget->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(10, 10, 48, 48), MyGUI::Align::Default);
        icon->setImageTexture(ach->iconPath);
        icon->setAlpha(alpha);

        MyGUI::TextBox* name = mTooltipWidget->createWidget<MyGUI::TextBox>(
            "NormalText", MyGUI::IntCoord(63, 10, 160, 18), MyGUI::Align::Default);
        name->setCaption(ach->name);
        name->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        name->setFontHeight(14);
        name->setTextColour(textColour);

        MyGUI::EditBox* description = mTooltipWidget->createWidget<MyGUI::EditBox>(
            "NormalText", MyGUI::IntCoord(63, 29, 200, 35), MyGUI::Align::Default);
        description->setEditStatic(true);
        description->setEditMultiLine(true);
        description->setEditWordWrap(true);
        description->setCaption(ach->description);
        description->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        description->setFontHeight(12);
        description->setTextColour(descColour);

        MyGUI::TextBox* points = mTooltipWidget->createWidget<MyGUI::TextBox>(
            "NormalText", MyGUI::IntCoord(235, 10, 180, 18), MyGUI::Align::Default);
        points->setCaption(std::to_string(ach->points) + " pts");
        points->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        points->setFontHeight(13);
        points->setTextColour(pointsColour);

        MyGUI::IntPoint mousePos = MyGUI::InputManager::getInstance().getMousePosition();
        int tooltipX = mousePos.left;
        int tooltipY = mousePos.top + 30;

        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        int screenHeight = MyGUI::RenderManager::getInstance().getViewSize().height;

        if (tooltipX + 250 > screenWidth)
            tooltipX = screenWidth - 250 - 10;
        if (tooltipY < 0)
            tooltipY = mousePos.top + 20;

        mTooltipWidget->setPosition(tooltipX, tooltipY);
        mTooltipWidget->setVisible(true);
    }

    void AchievementListWindow::onMiniIconMouseLeave(MyGUI::Widget* sender, MyGUI::Widget* dest)
    {
        if (mTooltipWidget)
        {
            mTooltipWidget->setVisible(false);

            while (mTooltipWidget->getChildCount() > 0)
                MyGUI::Gui::getInstance().destroyWidget(mTooltipWidget->getChildAt(0));
        }
    }

    void AchievementListWindow::onMouseWheel(MyGUI::Widget* /*sender*/, int rel)
    {
        if (mScrollView->getViewOffset().top + rel * 0.3 > 0)
            mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mScrollView->setViewOffset(
                MyGUI::IntPoint(0, static_cast<int>(mScrollView->getViewOffset().top + rel * 0.3f)));
    }

    void AchievementListWindow::setStatsWindow(RAStatsWindow* statsWindow)
    {
        mStatsWindow = statsWindow;
    }

    void AchievementListWindow::onStatsBtnClick(MyGUI::Widget* sender)
    {
        if (mStatsWindow)
        {
            mStatsWindow->toggle();
            updateStatsBtnState();
        }
    }

    void AchievementListWindow::updateStatsBtnState()
    {
        if (!mStatsButton || !mStatsWindow)
            return;

        if (mStatsWindow->isVisible())
        {
            mStatsButton->setStateSelected(true);
        }
        else
        {
            mStatsButton->setStateSelected(false);
        }
    }
}
