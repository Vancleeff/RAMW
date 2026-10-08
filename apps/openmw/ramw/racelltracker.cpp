#include "racelltracker.hpp"

#include "raachievement.hpp"
#include "rastats.hpp"
#include "ratriggercellclear.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Widget.h>

#include "../mwworld/esmstore.hpp"
#include <components/debug/debuglog.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loaddoor.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadnpc.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"

#include <algorithm>

namespace RAMW
{
    int hostileAt()
    {
        static const int base = MWBase::Environment::get()
                                    .getESMStore()
                                    ->get<ESM::GameSetting>()
                                    .find("iFightDistanceBase")
                                    ->mValue.getInteger();
        return 100 - base;
    }

    namespace
    {
        constexpr float sFadeSpeed = 3.0f; // full fade in about a third of a second
        constexpr int sWidth = 320;
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

    void RACellTracker::setCell(const std::string& cellName, MWWorld::CellStore* cell)
    {
        if (cellName == mCellName)
            return;

        mCellName = cellName;
        mCell = cell;
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
            if (mCell && obj.stat_name.rfind(RAStatKeys::CellClearPrefix, 0) == 0 && line.current == 0)
            {
                const CellCount count = countLivingActors(mCell);
                line.remaining = count.remaining;
                line.cellTotal = count.total;
            }
            else if (mCell && obj.stat_name.rfind(RAStatKeys::CellUnlockedPrefix, 0) == 0 && line.current == 0)
            {
                const CellCount count = countLockedContainers(mCell);
                line.remaining = count.remaining;
                line.cellTotal = count.total;
            }
            mLines.push_back(line);
        }

        layoutWidgets();
    }

    MyGUI::TextBox* RACellTracker::addText(
        const std::string& caption, int x, int y, int w, int fontHeight, const MyGUI::Colour& colour)
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
            if (line.remaining >= 0 && line.cellTotal > 0)
                caption
                    += "  " + std::to_string(line.cellTotal - line.remaining) + "/" + std::to_string(line.cellTotal);
            else if (line.target > 1)
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

    bool RACellTracker::inTrackedCell(const std::string& statName) const
    {
        if (!mTracked)
            return false;

        for (const AchievementObjective& obj : mTracked->objectives)
            if (obj.stat_name == statName)
                return true;

        return false;
    }

    // Seems very reliable now with levListVisitor included. To test massively.
    // Could be used to define "clean location x" kind of achievement when no other strict condition is possible.
    RACellTracker::CellCount RACellTracker::countLivingActors(MWWorld::CellStore* cell) const
    {
        CellCount result;

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

        auto creatureVisitor = [&](const MWWorld::Ptr& ptr) {
            if (!MWWorld::CellStore::isAccessible(ptr.getRefData(), ptr.getCellRef()))
                return true;

            const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
            const bool dead = stats.isDead();

            const bool hostile = stats.getAiSetting(MWMechanics::AiSetting::Fight).getBase() >= hostileAt()
                || (!player.isEmpty() && stats.getAiSequence().isInCombat(player));

            if (!dead && !hostile)
                return true;

            result.total++;
            if (!dead)
                result.remaining++;
            return true;
        };

        auto levListVisitor = [&](const MWWorld::Ptr& ptr) {
            if (!MWWorld::CellStore::isAccessible(ptr.getRefData(), ptr.getCellRef()))
                return true;

            if (ptr.getRefData().getCustomData() != nullptr)
                return true;

            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
            const ESM::CreatureLevList* list = store.get<ESM::CreatureLevList>().search(ptr.getCellRef().getRefId());
            if (!list)
                return true;

            const int playerLevel = player.getClass().getCreatureStats(player).getLevel();
            const bool canSpawn = std::any_of(list->mList.begin(), list->mList.end(),
                [&](const ESM::LevelledListBase::LevelItem& e) { return e.mLevel <= playerLevel; });

            if (canSpawn)
            {
                result.total++;
                result.remaining++;
            }
            return true;
        };

        cell->forEachType<ESM::Creature>(creatureVisitor, true);
        cell->forEachType<ESM::NPC>(creatureVisitor, true);
        cell->forEachType<ESM::CreatureLevList>(levListVisitor, true);

        return result;
    }

    RACellTracker::CellCount RACellTracker::countLockedContainers(MWWorld::CellStore* cell) const
    {
        CellCount result;

        auto visitor = [&](const MWWorld::Ptr& ptr) {
            if (!MWWorld::CellStore::isAccessible(ptr.getRefData(), ptr.getCellRef()))
                return true;

            // unlock() negates the level rather than clearing it, so a non-zero
            // value means the container had a lock to begin with.
            if (ptr.getCellRef().getLockLevel() != 0)
            {
                result.total++;
                if (ptr.getCellRef().isLocked())
                    result.remaining++;
            }
            return true;
        };

        cell->forEachType<ESM::Container>(visitor, true);
        cell->forEachType<ESM::Door>(visitor, true);

        return result;
    }
}
