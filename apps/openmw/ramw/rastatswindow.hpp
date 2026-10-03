#ifndef RAMW_RASTATSWINDOW_HPP
#define RAMW_RASTATSWINDOW_HPP

#include <MyGUI_ScrollView.h>
#include <MyGUI_Window.h>
#include <map>
#include <string>

#include "../mwgui/windowpinnablebase.hpp"

namespace RAMW
{
    class RAStatsWindow : public MWGui::WindowPinnableBase
    {
    public:
        RAStatsWindow();

        void toggle();
        void setVisible(bool visible);
        bool isVisible() const;
        void update(float dt);

    private:
        void createWidgets();
        void updateStatsList();
        void createKillSection(int& yPos);
        void createStatsSection(int& yPos);
        void createSeparator(int& yPos);
        void onPinToggled() override;
        void onMouseWheel(MyGUI::Widget* sender, int rel);

        MyGUI::ScrollView* mScrollView;
        bool mWantVisible;
    };
}

#endif
