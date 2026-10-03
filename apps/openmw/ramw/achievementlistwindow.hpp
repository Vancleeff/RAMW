#ifndef RAMW_ACHIEVEMENTLISTWINDOW_HPP
#define RAMW_ACHIEVEMENTLISTWINDOW_HPP

#include <MyGUI_ScrollView.h>
#include <MyGUI_Window.h>
#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <string>
#include <vector>

#include "../mwgui/windowpinnablebase.hpp" 
#include "raachievement.hpp" 

namespace RAMW
{
    class RAStatsWindow; 

    class AchievementListWindow : public MWGui::WindowPinnableBase
    {
    public:
        AchievementListWindow();

        void toggle();
        void setVisible(bool visible);
        bool isVisible() const;
        void update();  
        void setCategory(int category);
        void setStatsWindow(RAStatsWindow* statsWindow); 

    private:
        void createWidgets();
        void updateAchievementList();
        void createAchievementTile(const AchievementData* mainAchievement,
            const std::vector<const AchievementData*>& groupAchievements, int& yPos);
        void onPinToggled() override;
        void onCategoryBtnClick(MyGUI::Widget* sender);
        void onSearchTextChange(MyGUI::EditBox* sender);
        void onStatsBtnClick(MyGUI::Widget* sender);
        void updateCategoryBtnStates();
        void updateStatsBtnState(); 
        void updateCounter();
        void onMiniIconMouseEnter(MyGUI::Widget* sender, MyGUI::Widget* old);
        void onMiniIconMouseLeave(MyGUI::Widget* sender, MyGUI::Widget* dest);
        void onMouseWheel(MyGUI::Widget* sender, int rel);

        MyGUI::ScrollView* mScrollView;
        MyGUI::EditBox* mSearchBox;
        bool mWantVisible; 
        int mCurrentCategory;
        std::string mNameFilter;

        MyGUI::Button* mButtonAll;
        MyGUI::Button* mButtonStory;
        MyGUI::Button* mButtonExploration;
        MyGUI::Button* mButtonCombat;
        MyGUI::Button* mButtonMisc;
        MyGUI::Button* mStatsButton;
        MyGUI::Widget* mTooltipWidget;

        RAStatsWindow* mStatsWindow; 
    };

}

#endif
