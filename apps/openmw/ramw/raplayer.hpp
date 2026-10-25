#ifndef RAMW_RAPLAYER_HPP
#define RAMW_RAPLAYER_HPP

#include <string>
#include <string_view>

namespace MWMechanics
{
    class NpcStats;
}

namespace MWWorld
{
    class Ptr;
}

namespace RAMW
{
    struct AchievementData;
    std::string statKey(std::string_view name);

    namespace RAPlayer
    {
        MWWorld::Ptr ptr();

        const MWMechanics::NpcStats* stats();
        bool checkRequirements(const AchievementData& info);
    }
}

#endif
