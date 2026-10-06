#include "racelltracker.hpp"

#include "raachievement.hpp"
#include "rastats.hpp"
#include "ratriggercellclear.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Widget.h>

#include <algorithm>

namespace RAMW
{
    namespace
    {
        constexpr float sFadeSpeed = 3.0f;   // full fade in about a third of a second
        constexpr int sWidth = 260;
        constexpr int sPadding = 8;
        constexpr int sLineHeight = 20;
        constexpr int sTitleHeight = 24;

        const MyGUI::Colour sTitleColour(1.0f, 0.84f, 0.0f);
        const MyGUI::Colour sDoneColour(0.55f, 0.80f, 0.55f);
        const MyGUI::Colour sPendingColour(0.85f, 0.85f, 0.85f);
    }

    RACellTracker::RACellTracker()
        : MWGui::WindowBase("ramw_cell_tracker.layout")
    {
        mMainWidget->setNeedMouseFocus(false);
        mMainWidget->setVisible(false);
        mMainWidget->setAlpha(0.0f);
    }

    void RACellTracker::setCell(const std::string& cellName)
    {
        if (cellName == mCellName)
            return;

        mCellName = cellName;
        rebuildLines();

        mFadingIn = (mTracked != nullptr);
    }

    void RACellTracker::refresh()
    {
        rebuildLines();
    }

    void RACellTracker::rebuildLines()
    {
        mTracked = nullptr;
        mLines.clear();

        const std::string cellKey = RAStatKeys::CellClearPrefix + mCellName;
        std::string activeGroup;

        for (const AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerCellClear>())
        {
            const auto it = std::find_if(info->objectives.begin(), info->objectives.end(),
                [&](const AchievementObjective& obj) { return obj.stat_name == cellKey; });

            if (it == info->objectives.end())
                continue;

            mTracked = info;
            activeGroup = it->group;
            break;
        }

        if (!mTracked)
        {
            layoutWidgets();
            return;
        }

        for (const AchievementObjective& obj : mTracked->objectives)
        {
            // Ungrouped objectives belong to the achievement as a whole and are
            // always relevant; grouped ones only show inside their own location.
            if (!obj.group.empty() && obj.group != activeGroup)
                continue;

            Line line;
            line.label = obj.display_label;
            line.target = obj.amount;
            line.current = std::min(RAStats::get(obj.stat_name), obj.amount);
            mLines.push_back(line);
        }

        layoutWidgets();
    }

    MyGUI::TextBox* RACellTracker::addText(const std::string& caption, int x, int y, int w,
                                           int fontHeight, const MyGUI::Colour& colour)
    {
        // Drop shadow first so the text stays readable over any background.
        MyGUI::TextBox* shadow = mMainWidget->createWidget<MyGUI::TextBox>(
            "SandText", MyGUI::IntCoord(x + 1, y + 1, w, sLineHeight), MyGUI::Align::Left | MyGUI::Align::Top);
        shadow->setCaption(caption);
        shadow->setFontHeight(fontHeight);
        shadow->setTextColour(MyGUI::Colour(0.0f, 0.0f, 0.0f));
        shadow->setNeedMouseFocus(false);

        MyGUI::TextBox* text = mMainWidget->createWidget<MyGUI::TextBox>(
            "SandText", MyGUI::IntCoord(x, y, w, sLineHeight), MyGUI::Align::Left | MyGUI::Align::Top);
        text->setCaption(caption);
        text->setFontHeight(fontHeight);
        text->setTextColour(colour);
        text->setNeedMouseFocus(false);

        mCreatedWidgets.push_back(shadow);
        mCreatedWidgets.push_back(text);

        return text;
    }

    void RACellTracker::layoutWidgets()
    {
        std::vector<MyGUI::Widget*> children;
        for (MyGUI::Widget* w : mCreatedWidgets)
            MyGUI::Gui::getInstance().destroyWidget(w);
        mCreatedWidgets.clear();

        if (!mTracked)
            return;

        const int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        const int screenHeight = MyGUI::RenderManager::getInstance().getViewSize().height;

        int y = sPadding;
        const int innerWidth = sWidth - sPadding * 2;

        addText(mTracked->name, sPadding, y, innerWidth, 18, sTitleColour);
        y += sTitleHeight;

        addText(mCellName, sPadding, y, innerWidth, 15, sPendingColour);
        y += sLineHeight;

        for (const Line& line : mLines)
        {
            const bool done = line.current >= line.target;

            std::string caption = done ? "[X] " : "[ ] ";
            caption += line.label;
            if (line.target > 1)
                caption += "  " + std::to_string(line.current) + "/" + std::to_string(line.target);

            addText(caption, sPadding, y, innerWidth, 15, done ? sDoneColour : sPendingColour);
            y += sLineHeight;
        }

        const int height = y + sPadding;
        const int x = screenWidth - sWidth - 20;
        const int top = static_cast<int>(screenHeight * 0.28f);

        mMainWidget->setCoord(x, top, sWidth, height);
    }

    void RACellTracker::update(float dt)
    {
        if (!mMainWidget)
            return;

        const float goal = (mFadingIn && mTracked) ? 1.0f : 0.0f;

        if (mAlpha == goal)
        {
            mMainWidget->setVisible(mAlpha > 0.0f);
            return;
        }

        if (mAlpha < goal)
            mAlpha = std::min(goal, mAlpha + dt * sFadeSpeed);
        else
            mAlpha = std::max(goal, mAlpha - dt * sFadeSpeed);

        mMainWidget->setVisible(mAlpha > 0.0f);
        mMainWidget->setAlpha(mAlpha);
    }
}
