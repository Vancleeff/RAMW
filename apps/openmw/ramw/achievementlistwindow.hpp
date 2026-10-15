#ifndef RAMW_ACHIEVEMENTLISTWINDOW_HPP
#define RAMW_ACHIEVEMENTLISTWINDOW_HPP

#include <MyGUI_ScrollView.h>
#include <MyGUI_Window.h>
#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_ImageBox.h>
#include <map>
#include <string>
#include <vector>

#include "../mwgui/windowpinnablebase.hpp" 
#include "raachievement.hpp" 

namespace RAMW
{
    class AchievementListWindow : public MWGui::WindowPinnableBase
    {
    public:
        AchievementListWindow();

        void toggle();
        void setVisible(bool visible);
        bool isVisible() const;
        void update();
        void setCategory(int category);
        void onResChange(int width, int height) override;

        int mLastColumnCount = 0;
        MyGUI::Widget* mSidePanel = nullptr;
        MyGUI::Button* mClearFiltersButton = nullptr;
        MyGUI::Button* mShowUnlockedButton = nullptr;
        MyGUI::Colour mClearFiltersButtonColour;
        MyGUI::ComboBox* mSetCombo = nullptr;
        MyGUI::ComboBox* mRegionCombo = nullptr;
        MyGUI::ComboBox* mFactionCombo = nullptr;
        MyGUI::Widget* mStatsPanel = nullptr;
        MyGUI::ScrollView* mStatsView = nullptr;
        std::string mCurrentSet; // empty means All
        bool mShowUnlocked = true;
        bool mNeedsRelayout = false;
        bool mNeedsRefresh = false;

    private:
        void createWidgets();
        void restoreWindowRect(int viewWidth, int viewHeight);
        void buildTiles();
        void layoutTiles();
        void refreshTile(size_t index);
        void queueTileRefresh(size_t index);
        void invalidateAchievement(const std::string& id);
        void invalidateStat(const std::string& statName);
        MyGUI::Widget* createAchievementTile(const AchievementData* mainAchievement,
            const std::vector<const AchievementData*>& groupAchievements, int x, int yPos, int tileHeight);
        void onPinToggled() override;
        void onCategoryButtonClick(MyGUI::Widget* sender);
        void onSearchTextChange(MyGUI::EditBox* sender);
        void onWindowResize(MyGUI::Window* sender);
        void onSetComboChange(MyGUI::ComboBox* sender, size_t index);
        void rebuildSetCombo();
        int computeColumnCount() const;

        void updateCategoryButtonStates();
        void onTypeButtonClick(MyGUI::Widget* sender);
        void updateTypeButtonStates();
        void updateCounter();
        void onMiniIconMouseEnter(MyGUI::Widget* sender, MyGUI::Widget* old);
        void onObjectivesMouseEnter(MyGUI::Widget* sender, MyGUI::Widget* old);
        void onMiniIconMouseLeave(MyGUI::Widget* sender, MyGUI::Widget* dest);
        void onTextTooltipMouseEnter(MyGUI::Widget* sender, MyGUI::Widget* old);
        void onMouseWheel(MyGUI::Widget* sender, int rel);
        void onClearFiltersClick(MyGUI::Widget* sender);
        void onShowUnlockedClick(MyGUI::Widget* sender);
        void updateShowUnlockedButtonState();
        void updateClearFiltersButtonState();
        bool matchesFilter(const AchievementData& ach) const;

        MyGUI::ScrollView* mScrollView;
        MyGUI::EditBox* mSearchBox;
        MyGUI::IntSize mLastViewSize;
        bool mWantVisible; 
        int mCurrentCategory;
        int mTypeFilter = 0; // bitmask over AchievementType
        std::string mNameFilter;

        struct CategoryButton
        {
            MyGUI::Button* mWidget;
            int mFlag;
            std::string mCaption;
        };

        std::vector<CategoryButton> mCategoryButtons;

        struct TypeButton
        {
            MyGUI::Button* mWidget;
            int mMask;
            std::string mCaption;
        };

        std::vector<TypeButton> mTypeButtons;
        MyGUI::Widget* mTooltipWidget;

        enum FilterAxis
        {
            Axis_None,
            Axis_Set,
            Axis_Category,
            Axis_Type,
            Axis_Region,
            Axis_Faction
        };

        struct FilterCount
        {
            int mTotal = 0;
            int mLocked = 0;
        };

        bool passesFilters(const AchievementData& ach, FilterAxis except) const;
        bool facetVisible(const AchievementData& ach, FilterAxis except) const;
        std::map<std::string, FilterCount> tallyFilter(bool isRegion) const;
        void fillFilterCombo(
            MyGUI::ComboBox* combo, const std::map<std::string, FilterCount>& counts, const std::string& selected);
        void rebuildFilterCombos();
        void onRegionComboChange(MyGUI::ComboBox* sender, size_t index);
        void onFactionComboChange(MyGUI::ComboBox* sender, size_t index);

        void createStatsPanel(const MyGUI::IntCoord& coord);
        void updateStatsList();
        void onStatsMouseWheel(MyGUI::Widget* sender, int rel);

        std::string mCurrentRegion;
        std::string mCurrentFaction;

        // Tiles are built once, filters only move and hide them
        struct TileEntry
        {
            const AchievementData* mDisplay = nullptr;
            std::vector<const AchievementData*> mGroup;
            int mHeight = 0;
            MyGUI::Widget* mTile = nullptr;
            bool mShown = false;
        };

        std::vector<TileEntry> mTiles;
        std::map<std::string, size_t> mTileByAchievement;
        std::map<std::string, std::vector<size_t>> mTilesByStat;
        std::vector<size_t> mPendingTiles;
        MyGUI::ImageBox* mCollectionSeparator = nullptr;
        bool mTilesBuilt = false;
    };

}

#endif
