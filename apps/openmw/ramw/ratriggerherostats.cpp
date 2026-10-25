#include "ratriggerherostats.hpp"
#include "raachievement.hpp"
#include "raplayer.hpp"
#include "rastats.hpp"
#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include <components/esm/attr.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm3/loadspel.hpp>

#include <cstring>

namespace RAMW
{
    bool RATriggerHeroStats::isObjectiveCompleted(const AchievementObjective& obj) const
    {
        return false;
    }

    bool RATriggerHeroStats::evaluate(const RATriggerContext& ctx, const AchievementData& info) const
    {
        const std::string key = statKey(ctx.key);

        for (const auto& obj : info.objectives)
            if (statKey(obj.stat_name) == key && obj.matches(ctx.value))
                return true;
        return false;
    }

    // Read straight from the engine, so nothing has to be mirrored into RAStats
    int RATriggerHeroStats::getProgress(const AchievementData& info) const
    {
        const MWWorld::Ptr player = RAPlayer::ptr();
        if (player.isEmpty())
            return 0;

        const MWMechanics::NpcStats& stats = player.getClass().getNpcStats(player);

        if (info.tracked_stat == LevelKey)
            return stats.getLevel();

        if (info.tracked_stat == ReputationKey)
            return stats.getReputation();

        if (info.tracked_stat == SpellsKey)
        {
            int known = 0;

            for (const ESM::Spell* spell : stats.getSpells())
                if (spell->mData.mType == ESM::Spell::ST_Spell)
                    known++;

            return known;
        }

        if (info.tracked_stat.rfind(SkillPrefix, 0) == 0)
        {
            const ESM::RefId id
                = ESM::RefId::stringRefId(statKey(info.tracked_stat.substr(std::strlen(SkillPrefix))));

            if (MWBase::Environment::get().getESMStore()->get<ESM::Skill>().search(id))
                return stats.getSkill(id).getModified();
        }

        if (info.tracked_stat.rfind(AttributePrefix, 0) == 0)
        {
            const ESM::RefId id
                = ESM::RefId::stringRefId(statKey(info.tracked_stat.substr(std::strlen(AttributePrefix))));

            if (MWBase::Environment::get().getESMStore()->get<ESM::Attribute>().search(id))
                return stats.getAttribute(id).getModified();
        }

        return 0;
    }
}
