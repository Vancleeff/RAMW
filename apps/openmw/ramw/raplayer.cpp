#include "raplayer.hpp"
#include "raachievement.hpp"
#include "ratriggerherostats.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/ptr.hpp"

#include <components/esm/attr.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm3/loadspel.hpp>

#include <algorithm>
#include <cctype>
#include <cstring>

namespace RAMW
{
    namespace
    {
        constexpr const char* sBounty = "bounty";
        constexpr const char* sWerewolf = "werewolf";
        constexpr const char* sVampire = "vampire";
        constexpr const char* sAttributePrefix = "attribute_";
        constexpr const char* sRace = "race";
        constexpr const char* sClass = "class";
        constexpr const char* sBirthsign = "birthsign";
        constexpr const char* sSex = "sex";

        std::string lowerCase(std::string text)
        {
            std::transform(text.begin(), text.end(), text.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            return text;
        }

        int countSpells(const MWMechanics::NpcStats& stats)
        {
            int known = 0;

            for (const ESM::Spell* spell : stats.getSpells())
                if (spell->mData.mType == ESM::Spell::ST_Spell)
                    known++;

            return known;
        }

        int readStat(const std::string& key, const MWMechanics::NpcStats& stats)
        {
            if (key == RATriggerHeroStats::LevelKey)
                return stats.getLevel();

            if (key == RATriggerHeroStats::ReputationKey)
                return stats.getReputation();

            if (key == RATriggerHeroStats::SpellsKey)
                return countSpells(stats);

            if (key == sBounty)
                return stats.getBounty();

            if (key == sWerewolf)
                return stats.isWerewolf() ? 1 : 0;

            if (key == sVampire)
                return stats.getMagicEffects().getOrDefault(ESM::MagicEffect::Vampirism).getMagnitude() > 0 ? 1 : 0;

            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

            if (key.rfind(RATriggerHeroStats::SkillPrefix, 0) == 0)
            {
                const ESM::RefId id
                    = ESM::RefId::stringRefId(statKey(key.substr(std::strlen(RATriggerHeroStats::SkillPrefix))));

                if (store.get<ESM::Skill>().search(id))
                    return stats.getSkill(id).getModified();
            }

            if (key.rfind(sAttributePrefix, 0) == 0)
            {
                const ESM::RefId id = ESM::RefId::stringRefId(statKey(key.substr(std::strlen(sAttributePrefix))));

                if (store.get<ESM::Attribute>().search(id))
                    return stats.getAttribute(id).getModified();
            }

            return 0;
        }

        std::string readTrait(const std::string& key, const MWWorld::Ptr& player)
        {
            const ESM::NPC* record = player.get<ESM::NPC>()->mBase;

            if (key == sRace)
                return lowerCase(record->mRace.toString());

            if (key == sClass)
                return lowerCase(record->mClass.toString());

            if (key == sSex)
                return record->isMale() ? "male" : "female";

            if (key == sBirthsign)
                return lowerCase(MWBase::Environment::get().getWorld()->getPlayer().getBirthSign().toString());

            return {};
        }
    }

    std::string statKey(std::string_view name)
    {
        std::string key;
        key.reserve(name.size());

        for (const char c : name)
            if (c != '_')
                key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

        return key;
    }

    namespace RAPlayer
    {
        MWWorld::Ptr ptr()
        {
            return MWBase::Environment::get().getWorld()->getPlayerPtr();
        }

        const MWMechanics::NpcStats* stats()
        {
            const MWWorld::Ptr player = ptr();
            if (player.isEmpty())
                return nullptr;

            return &player.getClass().getNpcStats(player);
        }

        bool checkRequirements(const AchievementData& info)
        {
            if (info.requirements.empty())
                return true;

            const MWWorld::Ptr player = ptr();
            if (player.isEmpty())
                return false;

            const MWMechanics::NpcStats& stats = player.getClass().getNpcStats(player);

            for (const PlayerRequirement& requirement : info.requirements)
            {
                if (requirement.values.empty())
                {
                    if (!requirement.matches(readStat(requirement.key, stats)))
                        return false;
                }
                else
                {
                    const std::string trait = readTrait(requirement.key, player);

                    if (std::find(requirement.values.begin(), requirement.values.end(), trait)
                        == requirement.values.end())
                        return false;
                }
            }

            return true;
        }
    }
}
