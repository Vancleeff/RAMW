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

namespace MWWorld
{
    class CellStore;
}

namespace RAMW
{
    struct AchievementData;

    /// HUD overlay listing the objectives of location achievement
    class RACellTracker : public MWGui::WindowBase
    {
    public:
        RACellTracker();

        void setCell(const std::string& cellName, MWWorld::CellStore* cell);

        void update(float dt);
        void refresh();

        void onFrame(float dt) override { update(dt); }
        bool inTrackedCell(const std::string& statName) const;

        /// How much of a cell objective is left, and how much there was to begin
        /// with, so the overlay can show a done/total.
        struct CellCount
        {
            int remaining = 0;
            int total = 0;
        };

        CellCount countLivingActors(MWWorld::CellStore* cell) const;
        CellCount countLockedContainers(MWWorld::CellStore* cell) const;

    private:
        struct Line
        {
            std::string label;
            int current = 0;
            int target = 1;
            int remaining = -1;
            int cellTotal = 0;
        };

        void rebuildLines();
        void layoutWidgets();
        MyGUI::TextBox* addText(const std::string& caption, int x, int y, int w,
                                int fontHeight, const MyGUI::Colour& colour);

        std::string mCellName;
        MWWorld::CellStore* mCell = nullptr;
        const AchievementData* mTracked = nullptr;
        std::vector<Line> mLines;
        std::vector<MyGUI::Widget*> mCreatedWidgets;

        float mAlpha = 0.0f;
        bool mFadingIn = false;
    };
}

#endif
