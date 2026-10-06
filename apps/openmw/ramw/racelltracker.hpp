#ifndef RAMW_RACELLTRACKER_HPP
#define RAMW_RACELLTRACKER_HPP

#include "../mwgui/windowbase.hpp"

#include <string>
#include <vector>


namespace MyGUI
{
    class TextBox;
    class Widget;
}

namespace RAMW
{
    struct AchievementData;

    /// Non-intrusive HUD overlay listing the objectives of the achievement
    /// attached to the cell the player currently stands in.
    class RACellTracker : public MWGui::WindowBase
    {
    public:
        RACellTracker();

        /// Shows the tracker for the given cell, or fades it out when no
        /// achievement covers that cell.
        void setCell(const std::string& cellName);

        void update(float dt);
        void refresh();

        void onFrame(float dt) override { update(dt); }

    private:
        struct Line
        {
            std::string label;
            int current = 0;
            int target = 1;
        };

        void rebuildLines();
        void layoutWidgets();
        MyGUI::TextBox* addText(const std::string& caption, int x, int y, int w,
                                int fontHeight, const MyGUI::Colour& colour);

        std::string mCellName;
        const AchievementData* mTracked = nullptr;
        std::vector<Line> mLines;
        std::vector<MyGUI::Widget*> mCreatedWidgets;

        float mAlpha = 0.0f;
        bool mFadingIn = false;
    };
}

#endif
