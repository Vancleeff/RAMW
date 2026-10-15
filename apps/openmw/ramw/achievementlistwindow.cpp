#include "achievementlistwindow.hpp"
#include "raachievement.hpp"
#include "raeventprocessor.hpp"
#include "rastats.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_ProgressBar.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_Window.h>
#include <MyGUI_comboBox.h>
#include <functional>

#include <cctype>
#include <curl/curl.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwgui/itemwidget.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/ptr.hpp"

#include <components/esm/refid.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/settings/values.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/pathutil.hpp>

namespace
{
    constexpr const char* doneColour = "#D6A948";

    std::string toLower(std::string s)
    {
        std::transform(
            s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    bool matchesWordStart(const std::string& haystack, const std::string& needle)
    {
        size_t pos = haystack.find(needle);
        while (pos != std::string::npos)
        {
            if (pos == 0 || !std::isalnum(static_cast<unsigned char>(haystack[pos - 1])))
                return true;
            pos = haystack.find(needle, pos + 1);
        }
        return false;
    }

    std::string filterCaption(const std::string& value, int total, int locked)
    {
        if (total == 0)
            return value + " (0)";

        if (locked > 0)
            return value + " (" + std::to_string(locked) + ")";

        return doneColour + value + " (" + std::to_string(total) + ") *";
    }

    std::string comboValueAt(MyGUI::ComboBox* combo, size_t index)
    {
        if (index == 0 || index == MyGUI::ITEM_NONE)
            return {};

        const std::string* value = combo->getItemDataAt<std::string>(index, false);
        return value ? *value : std::string();
    }

    std::string creatureName(const std::string& refIdString)
    {
        const MWWorld::ESMStore* store = MWBase::Environment::get().getESMStore();
        const ESM::Creature* creature = store->get<ESM::Creature>().search(ESM::RefId::stringRefId(refIdString));

        return creature && !creature->mName.empty() ? creature->mName : refIdString;
    }

    bool isCollection(const RAMW::AchievementData& ach)
    {
        return (ach.category & RAMW::RAAchievementManager::Category_Collection) != 0;
    }

    std::string itemIdFromStat(const std::string& statName)
    {
        for (const char* prefix : { RAMW::RAStatKeys::InventoryPrefix, RAMW::RAStatKeys::LootPrefix })
            if (statName.rfind(prefix, 0) == 0)
                return statName.substr(std::strlen(prefix));

        return {};
    }

    std::vector<const RAMW::AchievementObjective*> collectionSlots(const RAMW::AchievementData& ach)
    {
        std::vector<const RAMW::AchievementObjective*> slots;
        std::vector<std::string> seen;

        for (const auto& obj : ach.objectives)
        {
            if (itemIdFromStat(obj.stat_name).empty())
                continue;

            if (!obj.group.empty())
            {
                if (std::find(seen.begin(), seen.end(), obj.group) != seen.end())
                    continue;
                seen.push_back(obj.group);
            }

            slots.push_back(&obj);
        }

        return slots;
    }

    constexpr int iconSize = 42;
    constexpr int iconGap = 4;
    constexpr int gridX = 85;
    constexpr int gridY = 80;
    constexpr int baseTileHeight = 130;
    constexpr int tileWidth = 420;
    constexpr int sidePanelWidth = 320;
    constexpr int gutter = 10;
    constexpr int miniIconSize = 24;
    constexpr int miniIconGap = 5;
    constexpr int miniIconMax = 5;

    int getTileHeight(const RAMW::AchievementData& ach, int tileWidth)
    {
        if (!isCollection(ach))
            return baseTileHeight;

        const int itemCount = static_cast<int>(collectionSlots(ach).size());
        const int perRow = std::max(1, (tileWidth - gridX - 20) / (iconSize + iconGap));
        const int rows = (itemCount + perRow - 1) / perRow;
        const int bottomReserve = ach.type != RAMW::Type_Normal ? 34 : 8;

        return std::max(baseTileHeight, gridY + rows * (iconSize + iconGap) + bottomReserve);
    }

    const RAMW::AchievementData* pickDisplayAchievement(
        const std::vector<const RAMW::AchievementData*>& group)
    {
        for (const RAMW::AchievementData* ach : group)
            if (!ach->unlocked)
                return ach;

        return group.back();
    }

    void createObjectiveLines(
        MyGUI::Widget* parent, const RAMW::AchievementData& ach, int left, int top, int width, int lineHeight)
    {
        int y = top;

        for (const auto& objective : ach.objectives)
        {
            if (!ach.isListedObjective(objective))
                continue;

            const bool completed = objective.group.empty()
                ? (ach.trigger && ach.trigger->isObjectiveCompleted(objective))
                : ach.isGroupCompleted(objective.group);

            MyGUI::TextBox* box = parent->createWidget<MyGUI::TextBox>(
                "SandText", MyGUI::IntCoord(left, y, 20, 18), MyGUI::Align::Default);
            box->setCaption(completed ? "[X]" : "[ ]");
            box->setTextColour(completed ? MyGUI::Colour(1.0f, 0.84f, 0.0f) : MyGUI::Colour(0.4f, 0.4f, 0.4f));
            box->setFontHeight(14);

            MyGUI::TextBox* label = parent->createWidget<MyGUI::TextBox>(
                "SandText", MyGUI::IntCoord(left + 26, y, width - 26, 18), MyGUI::Align::Default);

            const int stat = RAMW::RAStats::get(objective.stat_name);

            if (objective.stat_name.rfind(RAMW::RAStatKeys::CellClearPrefix, 0) == 0)
                label->setCaption(objective.stat_name.substr(std::strlen(RAMW::RAStatKeys::CellClearPrefix)));
            else
                label->setCaption(objective.display_label
                    + (objective.op == RAMW::Cmp_AtLeast && objective.amount > 1
                            ? " " + std::to_string(std::min(stat, objective.amount)) + "/"
                                + std::to_string(objective.amount)
                            : ""));

            if (objective.op == RAMW::Cmp_AtLeast && stat > 0 && objective.amount > 1 && !completed)
                label->setTextColour(MyGUI::Colour(1.0f, 0.84f, 0.0f, 0.5f));
            else
                label->setTextColour(completed ? MyGUI::Colour(1.0f, 0.84f, 0.0f) : MyGUI::Colour(0.4f, 0.4f, 0.4f));
            label->setFontHeight(14);

            y += lineHeight;
        }
    }

    struct ItemDisplay
    {
        std::string mIcon;
        std::string mName;
    };

    std::map<std::string, ItemDisplay> itemCache;

    // Records never change at runtime, collection icons resolved once
    void buildItemCache()
    {
        itemCache.clear();

        const MWWorld::ESMStore* store = MWBase::Environment::get().getESMStore();
        const Resource::ResourceSystem* resources = MWBase::Environment::get().getResourceSystem();
        if (!store || !resources)
            return;

        const VFS::Manager* vfs = resources->getVFS();

        for (const auto& [id, ach] : RAMW::RAAchievementManager::getAllAchievements())
        {
            if (!isCollection(ach))
                continue;

            for (const auto& objective : ach.objectives)
            {
                const std::string itemId = itemIdFromStat(objective.stat_name);
                if (itemId.empty() || itemCache.count(itemId) > 0)
                    continue;

                ItemDisplay display;

                try
                {
                    const MWWorld::ManualRef ref(*store, ESM::RefId::deserializeText(itemId));
                    const MWWorld::Ptr& ptr = ref.getPtr();

                    display.mName = std::string(ptr.getClass().getName(ptr));

                    const std::string_view icon = ptr.getClass().getInventoryIcon(ptr);
                    if (!icon.empty() && vfs)
                        display.mIcon = Misc::ResourceHelpers::correctIconPath(VFS::Path::toNormalized(icon), *vfs);
                }
                catch (const std::exception&)
                {
                    Log(Debug::Warning) << "[RAMW] " << ach.id << " references unknown item " << itemId;
                }

                if (display.mName.empty())
                    display.mName = itemId;

                itemCache.emplace(itemId, std::move(display));
            }
        }

        Log(Debug::Info) << "[RAMW] Cached " << itemCache.size() << " collection items";
    }

    struct TypeBadge
    {
        const char* mGlyph;
        const char* mLabel;
        MyGUI::Colour mColour;
    };

    // Indexed by RAMW::AchievementType, order follows the enum
    const TypeBadge typeBadges[] = {
        { "", "", MyGUI::Colour::White },
        { "!", "Missable", MyGUI::Colour(0.85f, 0.18f, 0.12f) },
        { "P", "Progression", MyGUI::Colour(0.33f, 0.62f, 0.92f) },
        { "W", "Win Condition", MyGUI::Colour(1.0f, 0.84f, 0.0f) },
    };
}

namespace RAMW
{
    AchievementListWindow::AchievementListWindow()
        : MWGui::WindowPinnableBase("ramw_achievement_list.layout")
        , mScrollView(nullptr)
        , mSearchBox(nullptr)
        , mWantVisible(false)
        , mCurrentCategory(RAAchievementManager::Category_All)
        , mNameFilter("")
        , mTooltipWidget(nullptr)
    {
        buildItemCache();
        createWidgets();
        updateCategoryButtonStates();
        updateCounter();
        mNeedsRelayout = true;

        RAAchievementManager::setOnUnlockCallback(
            [this](const std::string& id) { invalidateAchievement(id); });
        RAEventProcessor::setOnStatChangedCallback(
            [this](const std::string& statName) { invalidateStat(statName); });
        RAEventProcessor::setOntrackedCallback([this]() { mNeedsRefresh = true; });
    }

    void AchievementListWindow::createWidgets()
    {
        mLastViewSize = MyGUI::RenderManager::getInstance().getViewSize();
        restoreWindowRect(mLastViewSize.width, mLastViewSize.height);
        setPinned(Settings::windows().mRamwAchievementsPin);
        mMainWidget->setVisible(false);

        const int achievementWidth = mMainWidget->getWidth();

        const int titleBarHeight = 10;
        const int padding = 10;
        const int inset = 6;

        const MyGUI::IntSize client = mMainWidget->getClientCoord().size();
        const int contentY = padding;
        const int contentHeight = client.height - padding * 2;

        // left panel
        mSidePanel = mMainWidget->createWidget<MyGUI::Widget>("MW_Box",
            MyGUI::IntCoord(padding, contentY, sidePanelWidth, contentHeight), MyGUI::Align::Left | MyGUI::Align::Top);

        MyGUI::Widget* sideContent = mSidePanel->createWidget<MyGUI::Widget>("PanelEmpty",
            MyGUI::IntCoord(inset, inset, sidePanelWidth - inset * 2, contentHeight - inset * 2),
            MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);

        const int sideInnerWidth = sidePanelWidth - inset * 2;
        auto addSeparator = [&](int y) {
            MyGUI::ImageBox* line = sideContent->createWidget<MyGUI::ImageBox>("MW_HLine",
                MyGUI::IntCoord(padding, y + 8, sideInnerWidth - padding * 2, 10),
                MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
            line->setAlpha(0.6f);
            return y + 30;
        };

        // searchbox
        MyGUI::TextBox* searchLabel = sideContent->createWidget<MyGUI::TextBox>(
            "NormalText", MyGUI::IntCoord(padding, padding + 4, 60, 20), MyGUI::Align::Left | MyGUI::Align::Top);
        searchLabel->setCaption("search :");
        searchLabel->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);

        mSearchBox = sideContent->createWidget<MyGUI::EditBox>("MW_TextEdit",
            MyGUI::IntCoord(padding + 65, padding, sideInnerWidth - padding * 2 - 65, 22),
            MyGUI::Align::HStretch | MyGUI::Align::Top);
        mSearchBox->setCaption("");
        mSearchBox->eventEditTextChange += MyGUI::newDelegate(this, &AchievementListWindow::onSearchTextChange);

        // Filter row under the search box
        const int rowY = padding + 30;
        const int rowHeight = 24;
        const int toggleWidth = 48;

        // Show unlocked switch
        // label
        MyGUI::TextBox* showUnlockedLabel = sideContent->createWidget<MyGUI::TextBox>(
            "NormalText", MyGUI::IntCoord(padding, rowY + 4, 110, 18), MyGUI::Align::Left | MyGUI::Align::Top);
        showUnlockedLabel->setCaption("Show unlocked");
        showUnlockedLabel->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        // switch
        mShowUnlockedButton = sideContent->createWidget<MyGUI::Button>("MW_Button",
            MyGUI::IntCoord(padding + 115, rowY, toggleWidth, rowHeight), MyGUI::Align::Left | MyGUI::Align::Top);
        mShowUnlockedButton->eventMouseButtonClick
            += MyGUI::newDelegate(this, &AchievementListWindow::onShowUnlockedClick);
        updateShowUnlockedButtonState();

        // Filters removal btn
        mClearFiltersButton = sideContent->createWidget<MyGUI::Button>("MW_Button",
            MyGUI::IntCoord(sideInnerWidth - padding - 115, rowY, 115, rowHeight),
            MyGUI::Align::Right | MyGUI::Align::Top);
        mClearFiltersButton->setCaption("Remove filters");
        mClearFiltersButton->eventMouseButtonClick
            += MyGUI::newDelegate(this, &AchievementListWindow::onClearFiltersClick);

        mClearFiltersButtonColour = mClearFiltersButton->getTextColour();
        updateClearFiltersButtonState();

        int y = addSeparator(rowY + rowHeight + gutter);
        const int setY = y;

        MyGUI::TextBox* setLabel = sideContent->createWidget<MyGUI::TextBox>(
            "NormalText", MyGUI::IntCoord(padding, setY + 4, 40, 18), MyGUI::Align::Left | MyGUI::Align::Top);
        setLabel->setCaption("Set :");
        setLabel->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);

        mSetCombo = sideContent->createWidget<MyGUI::ComboBox>("MW_ComboBox",
            MyGUI::IntCoord(padding + 45, setY, sideInnerWidth - padding * 2 - 45, 24),
            MyGUI::Align::HStretch | MyGUI::Align::Top);
        mSetCombo->setEditStatic(true);
        mSetCombo->eventComboChangePosition += MyGUI::newDelegate(this, &AchievementListWindow::onSetComboChange);

        rebuildSetCombo();

        const int btnWidth = (sideInnerWidth - padding * 2 - gutter) / 2;
        const int btnHeight = 24;
        const int typeY = setY + 24 + 16;

        struct TypeDef
        {
            const char* mCaption;
            int mMask;
        };

        // Filters by type
        static const TypeDef types[] = {
            { "Progression", (1 << Type_Progression) | (1 << Type_WinCondition) },
            { "Missable", 1 << Type_Missable },
        };

        mTypeButtons.clear();

        for (size_t i = 0; i < std::size(types); ++i)
        {
            MyGUI::Button* btn = sideContent->createWidget<MyGUI::Button>("MW_Button",
                MyGUI::IntCoord(padding + static_cast<int>(i) * (btnWidth + gutter), typeY, btnWidth, btnHeight),
                MyGUI::Align::Left | MyGUI::Align::Top);
            btn->setCaption(types[i].mCaption);
            btn->eventMouseButtonClick += MyGUI::newDelegate(this, &AchievementListWindow::onTypeButtonClick);

            mTypeButtons.push_back({ btn, types[i].mMask, types[i].mCaption });
        }

        updateTypeButtonStates();

        y = addSeparator(typeY + btnHeight + gutter);
        const int catY = y;

        // Categories
        struct CategoryDef
        {
            const char* mCaption;
            int mFlag;
        };

        static const CategoryDef categories[] = {
            { "All", RAAchievementManager::Category_All },
            { "Quest", RAAchievementManager::Category_Quest },
            { "Combat", RAAchievementManager::Category_Combat },
            { "Explor.", RAAchievementManager::Category_Exploration },
            { "Loot", RAAchievementManager::Category_Loot },
            { "Character", RAAchievementManager::Category_Character },
            { "Collection", RAAchievementManager::Category_Collection },
            { "Misc", RAAchievementManager::Category_Misc },
        };

        const int catWidth = btnWidth;
        const int catHeight = btnHeight;

        mCategoryButtons.clear();

        for (size_t i = 0; i < std::size(categories); ++i)
        {
            const int col = static_cast<int>(i % 2);
            const int row = static_cast<int>(i / 2);

            MyGUI::Button* btn = sideContent->createWidget<MyGUI::Button>("MW_Button",
                MyGUI::IntCoord(
                    padding + col * (catWidth + gutter), catY + row * (catHeight + 4), catWidth, catHeight),
                MyGUI::Align::Left | MyGUI::Align::Top);
            btn->setCaption(categories[i].mCaption);
            btn->eventMouseButtonClick += MyGUI::newDelegate(this, &AchievementListWindow::onCategoryButtonClick);

            mCategoryButtons.push_back({ btn, categories[i].mFlag, categories[i].mCaption });
        }

        updateCategoryButtonStates();

        y = addSeparator(catY + 4 * (catHeight + 4) + gutter);

        const int regionY = y;

        MyGUI::TextBox* regionLabel = sideContent->createWidget<MyGUI::TextBox>(
            "NormalText", MyGUI::IntCoord(padding, regionY + 4, 60, 18), MyGUI::Align::Left | MyGUI::Align::Top);
        regionLabel->setCaption("Region :");
        regionLabel->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);

        mRegionCombo = sideContent->createWidget<MyGUI::ComboBox>("MW_ComboBox",
            MyGUI::IntCoord(padding + 65, regionY, sideInnerWidth - padding * 2 - 65, 24),
            MyGUI::Align::HStretch | MyGUI::Align::Top);
        mRegionCombo->setEditStatic(true);
        mRegionCombo->eventComboChangePosition += MyGUI::newDelegate(this, &AchievementListWindow::onRegionComboChange);

        const int factionY = regionY + 24 + gutter;

        MyGUI::TextBox* factionLabel = sideContent->createWidget<MyGUI::TextBox>(
            "NormalText", MyGUI::IntCoord(padding, factionY + 4, 60, 18), MyGUI::Align::Left | MyGUI::Align::Top);
        factionLabel->setCaption("Faction :");
        factionLabel->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);

        mFactionCombo = sideContent->createWidget<MyGUI::ComboBox>("MW_ComboBox",
            MyGUI::IntCoord(padding + 65, factionY, sideInnerWidth - padding * 2 - 65, 24),
            MyGUI::Align::HStretch | MyGUI::Align::Top);
        mFactionCombo->setEditStatic(true);
        mFactionCombo->eventComboChangePosition
            += MyGUI::newDelegate(this, &AchievementListWindow::onFactionComboChange);

        rebuildFilterCombos();

        const int filterHeight = factionY + 24 + inset + padding;
        mSidePanel->setSize(sidePanelWidth, filterHeight);
        sideContent->setSize(sideInnerWidth, filterHeight - inset * 2);

        createStatsPanel(MyGUI::IntCoord(
            padding, contentY + filterHeight + gutter, sidePanelWidth, contentHeight - filterHeight - gutter));

        // right panel
        const int rightX = padding + sidePanelWidth + padding;
        const int rightWidth = achievementWidth - rightX - padding;
        const int buttonWidth = (rightWidth - gutter * 4) / 5;

        // Achievements list placeholder
        mScrollView = mMainWidget->createWidget<MyGUI::ScrollView>(
            "MW_ScrollView", MyGUI::IntCoord(rightX, contentY, rightWidth, contentHeight), MyGUI::Align::Stretch);

        mMainWidget->castType<MyGUI::Window>()->eventWindowChangeCoord
            += MyGUI::newDelegate(this, &AchievementListWindow::onWindowResize);

        mScrollView->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
    }

    void AchievementListWindow::restoreWindowRect(int viewWidth, int viewHeight)
    {
        MyGUI::Window* window = mMainWidget->castType<MyGUI::Window>();

        window->setPosition(MyGUI::IntPoint(static_cast<int>(Settings::windows().mRamwAchievementsX * viewWidth),
            static_cast<int>(Settings::windows().mRamwAchievementsY * viewHeight)));
        window->setSize(MyGUI::IntSize(static_cast<int>(Settings::windows().mRamwAchievementsW * viewWidth),
            static_cast<int>(Settings::windows().mRamwAchievementsH * viewHeight)));

        clampWindowCoordinates(window);
    }

    void AchievementListWindow::onResChange(int width, int height)
    {
        mLastViewSize = MyGUI::IntSize(width, height);
        restoreWindowRect(width, height);
        mNeedsRelayout = true;
    }

    void AchievementListWindow::onSearchTextChange(MyGUI::EditBox* sender)
    {
        mNameFilter = sender->getCaption();

        // Avoid hover issues when destroying tiles
        mNeedsRelayout = true;
        updateClearFiltersButtonState();
    }

    void AchievementListWindow::rebuildSetCombo()
    {
        if (!mSetCombo)
            return;

        std::map<std::string, FilterCount> counts;

        for (const auto& [id, ach] : RAAchievementManager::getAllAchievements())
        {
            if (!facetVisible(ach, Axis_Set))
                continue;

            FilterCount& c = counts[ach.set];
            c.mTotal++;
            if (!ach.unlocked)
                c.mLocked++;
        }

        mSetCombo->removeAllItems();
        mSetCombo->addItem("All");

        size_t index = 0;

        // Load order, base set alkways first
        for (const std::string& name : RAAchievementManager::getSets())
        {
            const auto it = counts.find(name);
            if (it == counts.end() && name != mCurrentSet)
                continue;

            const FilterCount c = it != counts.end() ? it->second : FilterCount{};
            mSetCombo->addItem(filterCaption(name, c.mTotal, c.mLocked), name);

            if (name == mCurrentSet)
                index = mSetCombo->getItemCount() - 1;
        }

        mSetCombo->setIndexSelected(index);
    }

    void AchievementListWindow::onSetComboChange(MyGUI::ComboBox* sender, size_t index)
    {
        mCurrentSet = comboValueAt(sender, index);
        mCurrentRegion.clear();
        mCurrentFaction.clear();

        updateClearFiltersButtonState();
        mNeedsRelayout = true;
    }

    void AchievementListWindow::setCategory(int category)
    {
        mCurrentCategory = category;
        updateCategoryButtonStates();
        mNeedsRelayout = true;
        updateClearFiltersButtonState();
    }

    void AchievementListWindow::onCategoryButtonClick(MyGUI::Widget* sender)
    {
        for (const auto& entry : mCategoryButtons)
        {
            if (entry.mWidget == sender)
            {
                mCurrentCategory = entry.mFlag;
                break;
            }
        }

        updateCategoryButtonStates();
        updateClearFiltersButtonState();
        mNeedsRelayout = true;
    }

    void AchievementListWindow::updateCategoryButtonStates()
    {
        for (const auto& entry : mCategoryButtons)
        {
            const bool selected = entry.mFlag == mCurrentCategory;
            entry.mWidget->setStateSelected(selected);

            if (entry.mFlag == RAAchievementManager::Category_All)
                continue;

            FilterCount c;

            for (const auto& [id, ach] : RAAchievementManager::getAllAchievements())
            {
                if (!(ach.category & entry.mFlag) || !facetVisible(ach, Axis_Category))
                    continue;

                c.mTotal++;
                if (!ach.unlocked)
                    c.mLocked++;
            }

            entry.mWidget->setEnabled(c.mTotal > 0 || selected);
            entry.mWidget->setCaption(c.mTotal > 0 && c.mLocked == 0 ? entry.mCaption + " *" : entry.mCaption);
        }
    }

    void AchievementListWindow::onTypeButtonClick(MyGUI::Widget* sender)
    {
        for (const auto& entry : mTypeButtons)
        {
            if (entry.mWidget == sender)
            {
                // Exclusive filters
                mTypeFilter = mTypeFilter == entry.mMask ? 0 : entry.mMask;
                break;
            }
        }

        updateTypeButtonStates();
        updateClearFiltersButtonState();
        mNeedsRelayout = true;
    }

    void AchievementListWindow::updateTypeButtonStates()
    {
        for (const auto& entry : mTypeButtons)
        {
            const bool selected = mTypeFilter == entry.mMask;
            entry.mWidget->setStateSelected(selected);

            FilterCount c;

            for (const auto& [id, ach] : RAAchievementManager::getAllAchievements())
            {
                if (!(entry.mMask & (1 << ach.type)) || !facetVisible(ach, Axis_Type))
                    continue;

                c.mTotal++;
                if (!ach.unlocked)
                    c.mLocked++;
            }

            entry.mWidget->setEnabled(c.mTotal > 0 || selected);
            entry.mWidget->setCaption(c.mTotal > 0 && c.mLocked == 0 ? entry.mCaption + " *" : entry.mCaption);
        }
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

    void AchievementListWindow::buildTiles()
    {
        const auto started = std::chrono::steady_clock::now();

        mTiles.clear();
        mTileByAchievement.clear();
        mTilesByStat.clear();

        std::map<std::string, std::vector<const AchievementData*>> groups;

        for (const auto& [id, ach] : RAAchievementManager::getAllAchievements())
        {
            const std::string& groupKey = ach.parent_id.empty() ? ach.id : ach.parent_id;
            groups[groupKey].push_back(&ach);
        }

        for (auto& [key, achList] : groups)
            std::sort(achList.begin(), achList.end(),
                [](const AchievementData* a, const AchievementData* b) { return a->order < b->order; });

        std::vector<const std::vector<const AchievementData*>*> sortedGroups;
        sortedGroups.reserve(groups.size());
        for (const auto& [key, achList] : groups)
            sortedGroups.push_back(&achList);

        std::sort(sortedGroups.begin(), sortedGroups.end(), [](const auto* a, const auto* b) {
            const bool aCollection = isCollection(*(*a)[0]);
            if (aCollection != isCollection(*(*b)[0]))
                return !aCollection;

            return (*a)[0]->order < (*b)[0]->order;
        });

        mTiles.reserve(sortedGroups.size());

        for (const auto* achList : sortedGroups)
        {
            TileEntry entry;
            entry.mGroup = *achList;
            entry.mDisplay = pickDisplayAchievement(entry.mGroup);
            entry.mHeight = getTileHeight(*entry.mDisplay, tileWidth);
            mTiles.push_back(std::move(entry));
        }

        for (size_t i = 0; i < mTiles.size(); ++i)
        {
            TileEntry& entry = mTiles[i];
            entry.mTile = createAchievementTile(entry.mDisplay, entry.mGroup, 0, 0, entry.mHeight);
            entry.mTile->setVisible(false);

            for (const AchievementData* ach : entry.mGroup)
            {
                mTileByAchievement[ach->id] = i;

                if (!ach->tracked_stat.empty())
                    mTilesByStat[ach->tracked_stat].push_back(i);

                for (const auto& obj : ach->objectives)
                    mTilesByStat[obj.stat_name].push_back(i);
            }
        }

        mCollectionSeparator = mScrollView->createWidget<MyGUI::ImageBox>(
            "MW_HLine", MyGUI::IntCoord(0, 0, 10, 10), MyGUI::Align::Left | MyGUI::Align::Top);
        mCollectionSeparator->setNeedMouseFocus(false);
        mCollectionSeparator->setVisible(false);

        mTilesBuilt = true;

        using ms = std::chrono::duration<float, std::milli>;
        Log(Debug::Info) << "[RAMW] built " << mTiles.size() << " tiles in "
                         << ms(std::chrono::steady_clock::now() - started).count() << "ms";
    }

    void AchievementListWindow::layoutTiles()
    {
        const auto started = std::chrono::steady_clock::now();

        const int columns = computeColumnCount();
        mLastColumnCount = columns;

        std::vector<size_t> visible;
        visible.reserve(mTiles.size());

        for (size_t i = 0; i < mTiles.size(); ++i)
        {
            TileEntry& entry = mTiles[i];
            const bool shown
                = (mShowUnlocked || !entry.mDisplay->unlocked) && passesFilters(*entry.mDisplay, Axis_None);

            if (shown)
                visible.push_back(i);
            else if (entry.mShown)
                entry.mTile->setVisible(false);

            entry.mShown = shown;
        }

        size_t firstCollection = visible.size();
        for (size_t k = 0; k < visible.size(); ++k)
        {
            if (isCollection(*mTiles[visible[k]].mDisplay))
            {
                firstCollection = k;
                break;
            }
        }

        const bool splitCollections = mCurrentCategory == RAAchievementManager::Category_All
            && firstCollection > 0 && firstCollection < visible.size();

        mCollectionSeparator->setVisible(false);

        int yPos = gutter;

        for (size_t k = 0; k < visible.size();)
        {
            if (splitCollections && k == firstCollection)
            {
                const int gridWidth = columns * tileWidth + (columns - 1) * gutter;
                mCollectionSeparator->setCoord(gutter, yPos + 6, gridWidth, 10);
                mCollectionSeparator->setVisible(true);
                yPos += 26;
            }

            size_t end = std::min(k + static_cast<size_t>(columns), visible.size());
            if (splitCollections && k < firstCollection && end > firstCollection)
                end = firstCollection;

            int rowHeight = 0;
            for (size_t j = k; j < end; ++j)
                rowHeight = std::max(rowHeight, mTiles[visible[j]].mHeight);

            for (size_t j = k; j < end; ++j)
            {
                MyGUI::Widget* tile = mTiles[visible[j]].mTile;
                tile->setCoord(
                    gutter + static_cast<int>(j - k) * (tileWidth + gutter), yPos, tileWidth, rowHeight);
                tile->setVisible(true);
            }

            yPos += rowHeight + 12;
            k = end;
        }

        mScrollView->setCanvasSize(mScrollView->getWidth(), yPos + 30);
        updateCounter();

        using ms = std::chrono::duration<float, std::milli>;
        Log(Debug::Info) << "[RAMW] laid out " << visible.size() << "/" << mTiles.size() << " tiles in "
                         << ms(std::chrono::steady_clock::now() - started).count() << "ms";
    }

    void AchievementListWindow::refreshTile(size_t index)
    {
        TileEntry& entry = mTiles[index];

        // The tooltip holds widgets of this tile.
        if (mTooltipWidget)
            mTooltipWidget->setVisible(false);

        const MyGUI::IntCoord coord = entry.mTile->getCoord();
        MyGUI::Gui::getInstance().destroyWidget(entry.mTile);

        entry.mDisplay = pickDisplayAchievement(entry.mGroup);

        entry.mTile = createAchievementTile(entry.mDisplay, entry.mGroup, coord.left, coord.top, coord.height);
        entry.mTile->setVisible(entry.mShown);

        const int height = getTileHeight(*entry.mDisplay, tileWidth);
        if (height != entry.mHeight)
        {
            entry.mHeight = height;
            mNeedsRelayout = true;
        }
    }

    void AchievementListWindow::queueTileRefresh(size_t index)
    {
        if (std::find(mPendingTiles.begin(), mPendingTiles.end(), index) == mPendingTiles.end())
            mPendingTiles.push_back(index);
    }

    void AchievementListWindow::invalidateAchievement(const std::string& id)
    {
        if (!mTilesBuilt)
            return;

        const auto it = mTileByAchievement.find(id);
        if (it == mTileByAchievement.end())
            return;

        queueTileRefresh(it->second);
        mNeedsRefresh = true;

        // A hidden unlocked tile shifts everything after it.
        if (!mShowUnlocked)
            mNeedsRelayout = true;
    }

    void AchievementListWindow::invalidateStat(const std::string& statName)
    {
        if (!mTilesBuilt)
            return;

        const auto it = mTilesByStat.find(statName);
        if (it == mTilesByStat.end())
            return;

        for (size_t index : it->second)
            queueTileRefresh(index);

        mNeedsRefresh = true;
    }

    MyGUI::Widget* AchievementListWindow::createAchievementTile(const AchievementData* mainAchievement,
        const std::vector<const AchievementData*>& groupAchievements, int x, int yPos, int tileHeight)
    {
        const bool collection = isCollection(*mainAchievement);

        const std::vector<const AchievementObjective*> slots
            = collection ? collectionSlots(*mainAchievement) : std::vector<const AchievementObjective*>{};

        const int gridPerRow = std::max(1, (tileWidth - gridX - 20) / (iconSize + iconGap));
        const int inset = 5;

        MyGUI::Widget* tile = mScrollView->createWidget<MyGUI::Widget>(
            "PanelEmpty", MyGUI::IntCoord(x, yPos, tileWidth, tileHeight), MyGUI::Align::Left | MyGUI::Align::Top);

        if (mainAchievement->unlocked)
        {
            MyGUI::Widget* background = tile->createWidget<MyGUI::Widget>(
                "DialogBG", MyGUI::IntCoord(4, 4, tileWidth - 8, tileHeight - 8), MyGUI::Align::Stretch);
            background->setColour(MyGUI::Colour(1.0f, 0.84f, 0.0f));
            background->setAlpha(0.12f);
            background->setNeedMouseFocus(false);
        }

        MyGUI::Widget* frame = tile->createWidget<MyGUI::Widget>(
            mainAchievement->unlocked ? "MW_Button" : "MW_Box",
            MyGUI::IntCoord(0, 0, tileWidth, tileHeight), MyGUI::Align::Stretch);
        frame->setNeedMouseFocus(false);
        frame->setNeedKeyFocus(false);
        if (!mainAchievement->unlocked)
            frame->setAlpha(0.45f);

        MyGUI::Widget* content = tile->createWidget<MyGUI::Widget>("PanelEmpty",
            MyGUI::IntCoord(inset, inset, tileWidth - inset * 2, tileHeight - inset * 2), MyGUI::Align::Stretch);

        float alpha = mainAchievement->unlocked ? 1.0f : 0.28f;
        const MyGUI::Colour textColour
            = mainAchievement->unlocked ? MyGUI::Colour(1.0f, 0.84f, 0.0f) : MyGUI::Colour(0.82f, 0.60f, 0.26f);
        const MyGUI::Colour descColour(0.8f, 0.8f, 0.8f);
        MyGUI::Colour pointsColour
            = mainAchievement->unlocked ? MyGUI::Colour(0.7f, 0.7f, 0.7f) : MyGUI::Colour(0.4f, 0.4f, 0.4f);

        MyGUI::ImageBox* icon = content->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(10, 12, 64, 64), MyGUI::Align::Left | MyGUI::Align::Top);
        icon->setImageTexture(mainAchievement->iconPath);
        icon->setAlpha(alpha);

        MyGUI::Button* checkbox = content->createWidget<MyGUI::Button>(
            "MW_Button", MyGUI::IntCoord(tileWidth - 40, 5, 25, 25), MyGUI::Align::Left | MyGUI::Align::Top);
        checkbox->setCaption(mainAchievement->unlocked ? "X" : "");

        MyGUI::TextBox* name
            = content->createWidget<MyGUI::TextBox>("NormalText", MyGUI::IntCoord(85, 8, tileWidth - 150, 20),
                MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
        name->setCaption(mainAchievement->name);
        name->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        name->setFontHeight(16);
        name->setTextColour(textColour);

        MyGUI::EditBox* description = content->createWidget<MyGUI::EditBox>("NormalText",
            MyGUI::IntCoord(85, 30, tileWidth - 130, collection ? 46 : 80), MyGUI::Align::Left | MyGUI::Align::Top);
        description->setEditStatic(true);
        description->setEditMultiLine(true);
        description->setEditWordWrap(true);
        description->setCaption(mainAchievement->description);
        description->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        description->setMaxTextLength(200);
        description->setFontHeight(14);
        description->setTextColour(descColour);

        MyGUI::TextBox* points = content->createWidget<MyGUI::TextBox>(
            "NormalText", MyGUI::IntCoord(tileWidth - 70, 35, 55, 18), MyGUI::Align::Left | MyGUI::Align::Top);
        points->setCaption(std::to_string(mainAchievement->points) + " pts");
        points->setTextAlign(MyGUI::Align::Right | MyGUI::Align::Top);
        points->setFontHeight(15);
        points->setTextColour(pointsColour);

        int miniCount = 0;

        if (groupAchievements.size() > 1)
        {
            for (const AchievementData* ach : groupAchievements)
            {
                // The displayed one already owns the main icon.
                if (ach == mainAchievement)
                    continue;

                if (miniCount == miniIconMax)
                    break;

                MyGUI::ImageBox* miniIcon = content->createWidget<MyGUI::ImageBox>("ImageBox",
                    MyGUI::IntCoord(10 + miniCount * (miniIconSize + miniIconGap), 84, miniIconSize, miniIconSize),
                    MyGUI::Align::Default);

                if (!ach->iconPath.empty())
                    miniIcon->setImageTexture(ach->iconPath);

                miniIcon->setAlpha(ach->unlocked ? 1.0f : 0.3f);
                miniIcon->setNeedMouseFocus(true);
                miniIcon->setUserString("achievementId", ach->id);
                miniIcon->eventMouseSetFocus += MyGUI::newDelegate(this, &AchievementListWindow::onMiniIconMouseEnter);
                miniIcon->eventMouseLostFocus += MyGUI::newDelegate(this, &AchievementListWindow::onMiniIconMouseLeave);

                miniCount++;
            }
        }

        if (mainAchievement->hasProgressBar())
        {
            // The bar shares its row with the mini icons, so it starts after them.
            const int miniRight = miniCount > 0 ? 10 + miniCount * (miniIconSize + miniIconGap) - miniIconGap : 0;
            const int progressBarRight = 385;
            const int progressBarX = std::max(85, miniRight + 10);
            const MyGUI::IntCoord progressBarCoord(progressBarX, 84, progressBarRight - progressBarX, miniIconSize);

            MyGUI::ProgressBar* progressBar = content->createWidget<MyGUI::ProgressBar>(
                "MW_Progress_Blue", progressBarCoord, MyGUI::Align::Left | MyGUI::Align::Top);
            progressBar->setProgressRange(mainAchievement->target_value);
            progressBar->setProgressPosition(mainAchievement->getProgress());

            MyGUI::TextBox* progressText = content->createWidget<MyGUI::TextBox>(
                "SandText", progressBarCoord, MyGUI::Align::Left | MyGUI::Align::Top);
            progressText->setCaption(
                std::to_string(mainAchievement->getProgress()) + "/" + std::to_string(mainAchievement->target_value));
            progressText->setTextAlign(MyGUI::Align::Center);
            progressText->setFontHeight(15);
            progressText->setNeedMouseFocus(false);
        }

        if (mainAchievement->hasObjectives() && mainAchievement->getListedObjectivesCount() > 1
            && !mainAchievement->hasProgressBar())
        {
            int completedCount = 0;
            int listedCount = 0;

            for (const auto& objective : mainAchievement->objectives)
            {
                if (!mainAchievement->isListedObjective(objective))
                    continue;

                listedCount++;
                const bool done = objective.group.empty()
                    ? (mainAchievement->trigger && mainAchievement->trigger->isObjectiveCompleted(objective))
                    : mainAchievement->isGroupCompleted(objective.group);
                if (done)
                    completedCount++;
            }

            // Counter for collection achievements (left side)
            if (collection)
            {
                MyGUI::TextBox* objCount = content->createWidget<MyGUI::TextBox>(
                    "NormalText", MyGUI::IntCoord(5, 84, 64, 24), MyGUI::Align::Left | MyGUI::Align::Top);
                objCount->setCaption(std::to_string(completedCount) + "/" + std::to_string(listedCount));
                objCount->setTextAlign(MyGUI::Align::Center | MyGUI::Align::Top);
                objCount->setFontHeight(20);
                objCount->setTextColour(
                    completedCount > 0 ? MyGUI::Colour(1.0f, 0.84f, 0.0f) : MyGUI::Colour(0.5f, 0.5f, 0.5f));
            }
            else
            {
                const int objectivesWidth = 150;

                MyGUI::Button* seeObjectives = content->createWidget<MyGUI::Button>("MW_Button",
                    MyGUI::IntCoord(tileWidth - 45 - objectivesWidth, 84, objectivesWidth, 24),
                    MyGUI::Align::Left | MyGUI::Align::Top);
                seeObjectives->setCaption(
                    "Objectives: " + std::to_string(completedCount) + "/" + std::to_string(listedCount));
                seeObjectives->setUserString("achievementId", mainAchievement->id);
                seeObjectives->eventMouseSetFocus
                    += MyGUI::newDelegate(this, &AchievementListWindow::onObjectivesMouseEnter);
                seeObjectives->eventMouseLostFocus
                    += MyGUI::newDelegate(this, &AchievementListWindow::onMiniIconMouseLeave);
            }
        }

        std::function<void(MyGUI::Widget*)> wireWheel = [&](MyGUI::Widget* w) {
            w->eventMouseWheel.clear();
            w->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onMouseWheel);
            for (size_t i = 0; i < w->getChildCount(); ++i)
                wireWheel(w->getChildAt(i));
        };

        if (collection)
        {
            int index = 0;

            for (const AchievementObjective* slot : slots)
            {
                // always show the first variant of an item except if the player has only the second one
                const AchievementObjective* shown = slot;
                bool owned = mainAchievement->trigger && mainAchievement->trigger->isObjectiveCompleted(*slot);

                if (!slot->group.empty())
                {
                    owned = mainAchievement->isGroupCompleted(slot->group);

                    if (owned)
                        for (const auto& other : mainAchievement->objectives)
                            if (other.group == slot->group && mainAchievement->trigger
                                && mainAchievement->trigger->isObjectiveCompleted(other))
                            {
                                shown = &other;
                                break;
                            }
                }

                const std::string itemId = itemIdFromStat(shown->stat_name);
                const auto cached = itemCache.find(itemId);
                const int col = index % gridPerRow;
                const int row = index / gridPerRow;
                index++;

                MWGui::ItemWidget* item = content->createWidget<MWGui::ItemWidget>("MW_ItemIcon",
                    MyGUI::IntCoord(
                        gridX + col * (iconSize + iconGap), gridY + row * (iconSize + iconGap), iconSize, iconSize),
                    MyGUI::Align::Left | MyGUI::Align::Top);

                if (cached != itemCache.end() && !cached->second.mIcon.empty())
                    item->setIcon(cached->second.mIcon);

                item->setAlpha(owned ? 1.0f : 0.35f);

                item->setNeedMouseFocus(true);
                item->setUserString("tooltipText", cached != itemCache.end() ? cached->second.mName : itemId);
                item->eventMouseSetFocus += MyGUI::newDelegate(this, &AchievementListWindow::onTextTooltipMouseEnter);
                item->eventMouseLostFocus += MyGUI::newDelegate(this, &AchievementListWindow::onMiniIconMouseLeave);
            }
        }

        if (mainAchievement->type != Type_Normal)
        {
            const TypeBadge& badge = typeBadges[mainAchievement->type];
            const int badgeSize = 16;

            MyGUI::Widget* holder = content->createWidget<MyGUI::Widget>("PanelEmpty",
                MyGUI::IntCoord(tileWidth - 32, tileHeight - 32, badgeSize, badgeSize),
                MyGUI::Align::Right | MyGUI::Align::Bottom);

            const MyGUI::IntCoord edges[4] = { { 0, 0, badgeSize, 1 }, { 0, badgeSize - 1, badgeSize, 1 },
                { 0, 1, 1, badgeSize - 2 }, { badgeSize - 1, 1, 1, badgeSize - 2 } };
            for (const MyGUI::IntCoord& edge : edges)
            {
                MyGUI::Widget* side
                    = holder->createWidget<MyGUI::Widget>("MW_Progress_Drowning_Small", edge, MyGUI::Align::Default);
                side->setColour(badge.mColour);
                side->setNeedMouseFocus(false);
            }

            MyGUI::TextBox* mark = holder->createWidget<MyGUI::TextBox>(
                "NormalText", MyGUI::IntCoord(0, 0, badgeSize, badgeSize), MyGUI::Align::Default);
            mark->setCaption(badge.mGlyph);
            mark->setFontHeight(14);
            mark->setTextAlign(MyGUI::Align::Center);
            mark->setTextColour(badge.mColour);
            mark->setNeedMouseFocus(false);

            holder->setUserString("tooltipText", badge.mLabel);
            holder->eventMouseSetFocus += MyGUI::newDelegate(this, &AchievementListWindow::onTextTooltipMouseEnter);
            holder->eventMouseLostFocus += MyGUI::newDelegate(this, &AchievementListWindow::onMiniIconMouseLeave);
        }

        wireWheel(tile);

        return tile;
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

        if (mWantVisible && !pinned() && !MWBase::Environment::get().getWindowManager()->isGuiMode())
            mWantVisible = false;

        // as the window is not tracked by the windowmanager, we trigger onResChange here if necessary
        const MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        if (viewSize != mLastViewSize)
            onResChange(viewSize.width, viewSize.height);

        if (mMainWidget->getVisible() != mWantVisible)
            mMainWidget->setVisible(mWantVisible);

        // No rebuilt when closing the achievement windows to preserve perf
        if (!mWantVisible)
            return;

        if (!mTilesBuilt)
        {
            buildTiles();
            mNeedsRelayout = true;
        }

        if (!mPendingTiles.empty())
        {
            std::vector<size_t> pending;
            pending.swap(mPendingTiles);
            for (size_t index : pending)
                refreshTile(index);
        }

        if (mNeedsRefresh || mNeedsRelayout)
        {
            mNeedsRefresh = false;
            rebuildSetCombo();
            rebuildFilterCombos();
            updateCategoryButtonStates();
            updateTypeButtonStates();
            updateStatsList();
        }

        if (mNeedsRelayout)
        {
            mNeedsRelayout = false;
            layoutTiles();
        }
    }

    void AchievementListWindow::onPinToggled()
    {
        Settings::windows().mRamwAchievementsPin.set(mPinned);
    }

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

            mTooltipWidget->setNeedMouseFocus(false);
        }
        const bool withBar = ach->hasProgressBar();
        const int listedCount = ach->getListedObjectivesCount();
        const int objectiveLineHeight = 20;
        const int listTop = withBar ? 92 : 68;
        const int height = listedCount > 1 ? listTop + listedCount * objectiveLineHeight + 4 : listTop;

        mTooltipWidget->setAlpha(1.0f);
        mTooltipWidget->setSize(280, height);

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

        if (withBar)
        {
            const MyGUI::IntCoord barCoord(63, 66, 200, 16);

            MyGUI::ProgressBar* progressBar
                = mTooltipWidget->createWidget<MyGUI::ProgressBar>("MW_Progress_Blue", barCoord, MyGUI::Align::Default);
            progressBar->setProgressRange(ach->target_value);
            progressBar->setProgressPosition(ach->getProgress());

            MyGUI::TextBox* progressText
                = mTooltipWidget->createWidget<MyGUI::TextBox>("SandText", barCoord, MyGUI::Align::Default);
            progressText->setCaption(std::to_string(ach->getProgress()) + "/" + std::to_string(ach->target_value));
            progressText->setTextAlign(MyGUI::Align::Center);
            progressText->setFontHeight(13);
            progressText->setNeedMouseFocus(false);
        }

        if (listedCount > 1)
            createObjectiveLines(mTooltipWidget, *ach, 10, listTop, 260, objectiveLineHeight);

        MyGUI::IntPoint mousePos = MyGUI::InputManager::getInstance().getMousePosition();
        int tooltipX = mousePos.left;
        int tooltipY = mousePos.top + 30;

        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;
        int screenHeight = MyGUI::RenderManager::getInstance().getViewSize().height;

        if (tooltipX + 250 > screenWidth)
            tooltipX = screenWidth - 250 - 10;

        // Tooltip flips position if it's too big to keep it visible
        if (tooltipY + height > screenHeight)
            tooltipY = std::max(10, mousePos.top - height - 10);

        mTooltipWidget->setPosition(tooltipX, tooltipY);
        mTooltipWidget->setVisible(true);
    }

    void AchievementListWindow::onTextTooltipMouseEnter(MyGUI::Widget* sender, MyGUI::Widget* old)
    {
        const std::string text = std::string(sender->getUserString("tooltipText"));
        if (text.empty()) return;

        if (!mTooltipWidget)
        {
            mTooltipWidget = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>(
                "HUD_Box_NoTransp", MyGUI::IntCoord(0, 0, 280, 68), MyGUI::Align::Default, "Popup");

            mTooltipWidget->setNeedMouseFocus(false);
        }

        while (mTooltipWidget->getChildCount() > 0)
            MyGUI::Gui::getInstance().destroyWidget(mTooltipWidget->getChildAt(0));

        const int width = static_cast<int>(text.size()) * 9 + 24;
        mTooltipWidget->setAlpha(1.0f);
        mTooltipWidget->setSize(width, 32);

        MyGUI::TextBox* name = mTooltipWidget->createWidget<MyGUI::TextBox>(
            "NormalText", MyGUI::IntCoord(12, 7, width - 24, 18), MyGUI::Align::Default);
        name->setCaption(text);
        name->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        name->setFontHeight(14);
        name->setTextColour(MyGUI::Colour(0.8f, 0.8f, 0.8f));

        const MyGUI::IntPoint mousePos = MyGUI::InputManager::getInstance().getMousePosition();
        const int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;

        int tooltipX = mousePos.left;
        if (tooltipX + width > screenWidth)
            tooltipX = screenWidth - width - 10;

        mTooltipWidget->setPosition(tooltipX, mousePos.top + 30);
        mTooltipWidget->setVisible(true);
    }

    void AchievementListWindow::onObjectivesMouseEnter(MyGUI::Widget* sender, MyGUI::Widget* old)
    {
        std::string achId = std::string(sender->getUserString("achievementId"));
        const AchievementData* ach = RAAchievementManager::getAchievementData(achId);

        if (!ach)
            return;

        const int lineHeight = 20;
        const int pad = 10;
        const int width = 360;

        const int height = pad * 2 + ach->getListedObjectivesCount() * lineHeight;

        if (!mTooltipWidget)
        {
            mTooltipWidget = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>(
                "HUD_Box_NoTransp", MyGUI::IntCoord(0, 0, width, height), MyGUI::Align::Default, "Popup");

            mTooltipWidget->setNeedMouseFocus(false);
        }
        mTooltipWidget->setAlpha(1.0f);
        mTooltipWidget->setSize(width, height);

        while (mTooltipWidget->getChildCount() > 0)
            MyGUI::Gui::getInstance().destroyWidget(mTooltipWidget->getChildAt(0));

        createObjectiveLines(mTooltipWidget, *ach, pad, pad, width - pad * 2, lineHeight);

        MyGUI::IntPoint mousePos = MyGUI::InputManager::getInstance().getMousePosition();
        int screenWidth = MyGUI::RenderManager::getInstance().getViewSize().width;

        int tooltipX = mousePos.left;
        int tooltipY = mousePos.top - height - 10;

        if (tooltipX + width > screenWidth)
            tooltipX = screenWidth - width - 10;
        if (tooltipY < 10)
            tooltipY = 10;

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

    int AchievementListWindow::computeColumnCount() const
    {
        return std::max(1, (mScrollView->getWidth() - gutter) / (tileWidth + gutter));
    }

    void AchievementListWindow::onWindowResize(MyGUI::Window* sender)
    {
        clampWindowCoordinates(sender);

        const MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        Settings::windows().mRamwAchievementsX.set(sender->getPosition().left / static_cast<float>(viewSize.width));
        Settings::windows().mRamwAchievementsY.set(sender->getPosition().top / static_cast<float>(viewSize.height));
        Settings::windows().mRamwAchievementsW.set(sender->getSize().width / static_cast<float>(viewSize.width));
        Settings::windows().mRamwAchievementsH.set(sender->getSize().height / static_cast<float>(viewSize.height));

        // Rebuilding every widget is killing perf during resizing so only a change in column count is considred
        if (computeColumnCount() == mLastColumnCount)
            return;

        mNeedsRelayout = true;
    }

    void AchievementListWindow::onClearFiltersClick(MyGUI::Widget* sender)
    {
        mNameFilter.clear();
        mSearchBox->setCaption("");
        mShowUnlocked = true;
        mCurrentSet.clear();
        mCurrentCategory = RAAchievementManager::Category_All;
        mCurrentRegion.clear();
        mCurrentFaction.clear();
        mTypeFilter = 0;

        updateShowUnlockedButtonState();
        updateTypeButtonStates();
        updateClearFiltersButtonState();
        mNeedsRelayout = true;
    }

    void AchievementListWindow::onShowUnlockedClick(MyGUI::Widget* sender)
    {
        mShowUnlocked = !mShowUnlocked;
        updateShowUnlockedButtonState();
        updateClearFiltersButtonState();
        mNeedsRelayout = true;
    }

    void AchievementListWindow::updateShowUnlockedButtonState()
    {
        if (!mShowUnlockedButton)
            return;

        mShowUnlockedButton->setCaptionWithReplacing(mShowUnlocked ? "#{Interface:On}" : "#{Interface:Off}");
    }

    void AchievementListWindow::updateClearFiltersButtonState()
    {
        if (!mClearFiltersButton)
            return;

        const bool filtering = !mNameFilter.empty() || mCurrentCategory != RAAchievementManager::Category_All
            || !mShowUnlocked || !mCurrentSet.empty() || !mCurrentRegion.empty() || !mCurrentFaction.empty()
            || mTypeFilter != 0;

        mClearFiltersButton->setTextColour(filtering ? MyGUI::Colour(0.78f, 0.23f, 0.12f) : mClearFiltersButtonColour);
    }

    bool AchievementListWindow::matchesFilter(const AchievementData& ach) const
    {
        if (mNameFilter.empty())
            return true;

        const std::string needle = toLower(mNameFilter);

        if (matchesWordStart(toLower(ach.name), needle) || matchesWordStart(toLower(ach.description), needle))
            return true;

        for (const auto& obj : ach.objectives)
        {
            if (matchesWordStart(toLower(obj.display_label), needle)
                || matchesWordStart(toLower(obj.stat_name), needle))
                return true;
        }

        return false;
    }

    bool AchievementListWindow::passesFilters(const AchievementData& ach, FilterAxis except) const
    {
        if (except != Axis_Set && !mCurrentSet.empty() && ach.set != mCurrentSet)
            return false;

        if (except != Axis_Category && !(ach.category & mCurrentCategory))
            return false;

        if (except != Axis_Type && mTypeFilter != 0 && !(mTypeFilter & (1 << ach.type)))
            return false;

        if (except != Axis_Region && !mCurrentRegion.empty()
            && std::find(ach.regions.begin(), ach.regions.end(), mCurrentRegion) == ach.regions.end())
            return false;

        if (except != Axis_Faction && !mCurrentFaction.empty()
            && std::find(ach.factions.begin(), ach.factions.end(), mCurrentFaction) == ach.factions.end())
            return false;

        return matchesFilter(ach);
    }

    bool AchievementListWindow::facetVisible(const AchievementData& ach, FilterAxis except) const
    {
        return (mShowUnlocked || !ach.unlocked) && passesFilters(ach, except);
    }

    std::map<std::string, AchievementListWindow::FilterCount> AchievementListWindow::tallyFilter(bool isRegion) const
    {
        std::map<std::string, FilterCount> counts;

        for (const auto& [id, ach] : RAAchievementManager::getAllAchievements())
        {
            if (!facetVisible(ach, isRegion ? Axis_Region : Axis_Faction))
                continue;

            for (const std::string& value : isRegion ? ach.regions : ach.factions)
            {
                FilterCount& c = counts[value];
                c.mTotal++;
                if (!ach.unlocked)
                    c.mLocked++;
            }
        }

        return counts;
    }

    void AchievementListWindow::fillFilterCombo(
        MyGUI::ComboBox* combo, const std::map<std::string, FilterCount>& counts, const std::string& selected)
    {
        if (!combo)
            return;

        combo->removeAllItems();
        combo->addItem("All");

        size_t index = 0;

        for (const auto& [value, c] : counts)
        {
            combo->addItem(filterCaption(value, c.mTotal, c.mLocked), value);
            if (value == selected)
                index = combo->getItemCount() - 1;
        }

        if (!selected.empty() && counts.find(selected) == counts.end())
        {
            combo->addItem(filterCaption(selected, 0, 0), selected);
            index = combo->getItemCount() - 1;
        }

        combo->setIndexSelected(index);
    }

    void AchievementListWindow::rebuildFilterCombos()
    {
        fillFilterCombo(mRegionCombo, tallyFilter(true), mCurrentRegion);
        fillFilterCombo(mFactionCombo, tallyFilter(false), mCurrentFaction);
    }

    void AchievementListWindow::createStatsPanel(const MyGUI::IntCoord& coord)
    {
        const int border = 2;

        mStatsPanel
            = mMainWidget->createWidget<MyGUI::Widget>("MW_Box", coord, MyGUI::Align::Left | MyGUI::Align::VStretch);

        mStatsView = mStatsPanel->createWidget<MyGUI::ScrollView>("MW_ScrollView",
            MyGUI::IntCoord(border, border, coord.width, coord.height - border * 2), MyGUI::Align::Stretch);

        mStatsView->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        mStatsView->eventMouseWheel += MyGUI::newDelegate(this, &AchievementListWindow::onStatsMouseWheel);

        updateStatsList();
    }

    void AchievementListWindow::updateStatsList()
    {
        if (!mStatsView)
            return;

        const int width = mStatsView->getWidth() - 14;

        mStatsView->setCanvasSize(width, 0);
        while (mStatsView->getChildCount() > 0)
            MyGUI::Gui::getInstance().destroyWidget(mStatsView->getChildAt(0));

        int y = 6;

        auto header = [&](const char* caption) {
            MyGUI::TextBox* box = mStatsView->createWidget<MyGUI::TextBox>(
                "SandText", MyGUI::IntCoord(8, y, width - 16, 24), MyGUI::Align::Left | MyGUI::Align::Top);
            box->setCaption(caption);
            box->setTextColour(MyGUI::Colour(1.0f, 0.84f, 0.0f));
            box->setFontHeight(20);
            box->setNeedMouseFocus(false);
            y += 28;
        };

        auto line = [&](const std::string& text, bool dim) {
            MyGUI::TextBox* box = mStatsView->createWidget<MyGUI::TextBox>(
                "SandText", MyGUI::IntCoord(16, y, width - 24, 20), MyGUI::Align::Left | MyGUI::Align::Top);
            box->setCaption(text);
            box->setTextColour(dim ? MyGUI::Colour(0.5f, 0.5f, 0.5f) : MyGUI::Colour(0.8f, 0.8f, 0.8f));
            box->setNeedMouseFocus(false);
            y += 22;
        };

        auto completionRow = [&](const char* caption, int done, int total) {
            const int labelWidth = 100;
            const int barX = 16 + labelWidth;
            const int barWidth = std::max(40, width - barX - 16);

            MyGUI::TextBox* label = mStatsView->createWidget<MyGUI::TextBox>(
                "SandText", MyGUI::IntCoord(16, y, labelWidth, 20), MyGUI::Align::Left | MyGUI::Align::Top);
            label->setCaption(caption);
            label->setTextColour(MyGUI::Colour(0.8f, 0.8f, 0.8f));
            label->setNeedMouseFocus(false);

            MyGUI::ProgressBar* bar = mStatsView->createWidget<MyGUI::ProgressBar>("MW_Progress_Blue",
                MyGUI::IntCoord(barX, y + 1, barWidth, 18), MyGUI::Align::Left | MyGUI::Align::Top);
            bar->setProgressRange(std::max(1, total));
            bar->setProgressPosition(done);
            bar->setNeedMouseFocus(false);

            MyGUI::TextBox* count = mStatsView->createWidget<MyGUI::TextBox>("SandText",
                MyGUI::IntCoord(barX, y + 1, barWidth, 18), MyGUI::Align::Left | MyGUI::Align::Top);
            count->setCaption(std::to_string(done) + "/" + std::to_string(total));
            count->setTextAlign(MyGUI::Align::Center);
            count->setFontHeight(15);
            count->setNeedMouseFocus(false);

            y += 24;
        };

        const auto& allStats = RAStats::getAllStats();

        static const std::pair<const char*, const char*> statLabels[] = {
            { RAStatKeys::QuestsFinished, "Quests Finished" },
            { RAStatKeys::LocationsDiscovered, "Locations Discovered" },
            { RAStatKeys::KillCreatureTotal, "Creatures Killed" },
            { RAStatKeys::KillNpcTotal, "NPCs Killed" },
            { RAStatKeys::LockpickCount, "Container Lockpicked" },
            { RAStatKeys::ItemsLooted, "Items Looted" },
            { RAStatKeys::ItemsStolen, "Items Stolen" },
            { RAStatKeys::GoldLooted, "Gold Collected" },
            { RAStatKeys::HarvestCount, "Plants Harvested" },
            { RAStatKeys::BooksRead, "Books Read" },
            { RAStatKeys::SkillBooksRead, "Skill Books Read" },
        };

        struct CompletionDef
        {
            const char* mCaption;
            int mFlag;
        };

        static const CompletionDef completionDefs[] = {
            { "Total", RAAchievementManager::Category_All },
            { "Quest", RAAchievementManager::Category_Quest },
            { "Combat", RAAchievementManager::Category_Combat },
            { "Exploration", RAAchievementManager::Category_Exploration },
            { "Loot", RAAchievementManager::Category_Loot },
            { "Character", RAAchievementManager::Category_Character },
            { "Misc", RAAchievementManager::Category_Misc },
            { "Collection", RAAchievementManager::Category_Collection },
        };

        header("Completion");

        for (const auto& def : completionDefs)
        {
            const bool isTotal = def.mFlag == RAAchievementManager::Category_All;
            int done = 0;
            int total = 0;

            for (const auto& [id, ach] : RAAchievementManager::getAllAchievements())
            {
                if (!isTotal && (ach.category & def.mFlag) == 0)
                    continue;

                total++;
                if (ach.unlocked)
                    done++;
            }

            if (total > 0)
                completionRow(def.mCaption, done, total);
        }

        MyGUI::ImageBox* completionSep = mStatsView->createWidget<MyGUI::ImageBox>(
            "MW_HLine", MyGUI::IntCoord(8, y + 6, width - 16, 10), MyGUI::Align::Left | MyGUI::Align::Top);
        completionSep->setAlpha(0.6f);
        y += 30;

        header("Stats");

        bool any = false;

        for (const auto& [key, label] : statLabels)
        {
            const auto it = allStats.find(key);
            if (it == allStats.end())
                continue;

            any = true;
            line(std::string(label) + ": " + std::to_string(it->second), false);
        }

        if (!any)
            line("No stats recorded yet", true);

        MyGUI::ImageBox* separator = mStatsView->createWidget<MyGUI::ImageBox>(
            "MW_HLine", MyGUI::IntCoord(8, y + 6, width - 16, 10), MyGUI::Align::Left | MyGUI::Align::Top);
        separator->setAlpha(0.6f);
        separator->setNeedMouseFocus(false);
        y += 30;

        header("Killing");

        std::map<std::string, int> killsByName;
        const size_t prefixLength = std::strlen(RAStatKeys::CreatureKillPrefix);

        for (const auto& [statName, value] : allStats)
            if (statName.rfind(RAStatKeys::CreatureKillPrefix, 0) == 0)
                killsByName[creatureName(statName.substr(prefixLength))] += value;

        if (killsByName.empty())
            line("No creatures killed yet", true);
        else
            for (const auto& [name, kills] : killsByName)
                line(name + ": " + std::to_string(kills), false);

        mStatsView->setCanvasSize(width, y + 6);
    }

    void AchievementListWindow::onStatsMouseWheel(MyGUI::Widget* /*sender*/, int rel)
    {
        const int overflow = std::max(0, mStatsView->getCanvasSize().height - mStatsView->getViewCoord().height);
        const int top = mStatsView->getViewOffset().top + static_cast<int>(rel * 0.3f);
        mStatsView->setViewOffset(MyGUI::IntPoint(0, std::max(-overflow, std::min(0, top))));
    }

    void AchievementListWindow::onRegionComboChange(MyGUI::ComboBox* sender, size_t index)
    {
        mCurrentRegion = comboValueAt(sender, index);
        updateClearFiltersButtonState();
        mNeedsRelayout = true;
    }

    void AchievementListWindow::onFactionComboChange(MyGUI::ComboBox* sender, size_t index)
    {
        mCurrentFaction = comboValueAt(sender, index);
        updateClearFiltersButtonState();
        mNeedsRelayout = true;
    }
}
