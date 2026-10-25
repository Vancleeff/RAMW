#include "raeventprocessor.hpp"
#include "../mwbase/soundmanager.hpp"
#include "raachievement.hpp"
#include "racelltracker.hpp"
#include "racore.hpp"
#include "radiscoverynotification.hpp"
#include "rastats.hpp"
#include "ratriggerHerostats.hpp"
#include "ratriggercellclear.hpp"
#include "ratriggerdiscovery.hpp"
#include "ratriggerglobal.hpp"
#include "ratriggerharvest.hpp"
#include "ratriggerkill.hpp"
#include "ratriggersoultrap.hpp"
#include "ratriggerlocation.hpp"
#include "ratriggerlockpick.hpp"
#include "ratriggerloot.hpp"
#include "ratriggerquestcomplete.hpp"
#include "ratriggerquestprogress.hpp"
#include "ratriggerreadbook.hpp"
#include "ratriggerdialog.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loaddoor.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadspel.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwclass/nameorid.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwworld/cellref.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/ptr.hpp"

#include <algorithm>, <cctype>

namespace
{
    std::string normalizeName(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }
}

namespace RAMW
{
    std::function<void()> RAEventProcessor::mOnEventCallback = nullptr;
    std::function<void()> RAEventProcessor::mOntrackedCallback = nullptr;
    std::function<void(const std::string&)> RAEventProcessor::mOnStatChangedCallback = nullptr;
    std::unordered_set<std::string> RAEventProcessor::sTrackedGlobals;
    std::unordered_set<std::string> RAEventProcessor::sTrackedStats;
    std::unordered_set<std::string> RAEventProcessor::sTrackedObjects;
    bool RAEventProcessor::sLoading = false;

    void RAEventProcessor::init()
    {
        Log(Debug::Info) << "[RAMW Events] Event processor initialized";
    }

    void RAEventProcessor::onGameSaved(ESM::ESMWriter& writer)
    {
        RAStats::write(writer);
    }

    bool RAEventProcessor::isLoading()
    {
        return sLoading;
    }

    void RAEventProcessor::setLoading(bool loading)
    {
        sLoading = loading;
    }

    void RAEventProcessor::onGameCleanup()
    {
        sLoading = true;
        RAStats::init();
    }

    void RAEventProcessor::onGameLoaded(ESM::ESMReader& reader)
    {
        RAStats::readRecord(reader);
        RAAchievementManager::restoreFromStats();

        RACore::onGameReady();
        if (mOnEventCallback && RAMW::RACore::isInitialized())
            mOnEventCallback();
        if (mOntrackedCallback && RAMW::RACore::isInitialized())
            mOntrackedCallback();
    }

    void RAEventProcessor::onTopic(const std::string& topicId, const std::string& npcId)
    {
        if (isLoading())
            return;

        RATriggerContext ctx{ RAStatKeys::TopicPrefix + normalizeName(topicId) + "@" + normalizeName(npcId) };

        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerDialog>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
    }

    void RAEventProcessor::onQuestProgress(const std::string& questId, int stage)
    {
        Log(Debug::Info) << "[RA Events] Quest progressed - " << questId << " stage " << stage;

        RATriggerContext ctx{ "quest_stage_" + questId, stage };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerQuestProgress>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onQuestFinished(const std::string& questId)
    {
        Log(Debug::Info) << "[RAMW Events] Quest completed - " << questId;

        const std::string statKey = "quest_finished_" + questId;
        RAStats::set(statKey, 1);
        RAStats::increment(RAStatKeys::QuestsFinished);

        RATriggerContext ctx{ statKey };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerQuestComplete>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onLevelUp(int newLevel)
    {
        Log(Debug::Info) << "[RAMW Events] Level up to: " << newLevel;

        RATriggerContext ctx{ RATriggerHeroStats::LevelKey, newLevel };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerHeroStats>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onSkillLevelUp(const std::string& skillId, int newValue, const void* statsPtr)
    {
        Log(Debug::Info) << "[RAMW Events] Skill level up: " << skillId << " to " << std::to_string(newValue);

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        if (player.isEmpty() || &player.getClass().getNpcStats(player) != statsPtr)
            return;

        RATriggerContext ctx{ RATriggerHeroStats::SkillPrefix + skillId, newValue };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerHeroStats>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onReputationChanged(int newValue, const void* statsPtr)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        if (player.isEmpty() || &player.getClass().getNpcStats(player) != statsPtr)
            return;

        Log(Debug::Info) << "[RAMW Events] Reputation: " << std::to_string(newValue);

        RATriggerContext ctx{ RATriggerHeroStats::ReputationKey, newValue };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerHeroStats>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        RAStats::notifyProgress(RATriggerHeroStats::ReputationKey, newValue);

        onStatChanged(RATriggerHeroStats::ReputationKey, newValue);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::updateSpellCount(float dt)
    {
        static float elapsed = 0.0f;
        static int known = -1;

        if (isLoading())
        {
            known = -1;
            return;
        }

        elapsed += dt;
        if (elapsed < 1.0f)
            return;
        elapsed = 0.0f;

        const MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        if (player.isEmpty())
            return;

        int count = 0;

        for (const ESM::Spell* spell : player.getClass().getCreatureStats(player).getSpells())
            if (spell->mData.mType == ESM::Spell::ST_Spell)
                count++;

        if (count == known)
            return;

        const bool step1 = known < 0;
        known = count;

        RATriggerContext ctx{ RATriggerHeroStats::SpellsKey, count };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerHeroStats>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        // avoid useless notification during loading
        if (!step1)
            RAStats::notifyProgress(RATriggerHeroStats::SpellsKey, count);

        onStatChanged(RATriggerHeroStats::SpellsKey, count);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onOpenLock(int lockStrength)
    {
        Log(Debug::Info) << "[RA Events] Lock picked - strength " << lockStrength;

        RAStats::increment(RAStatKeys::LockpickCount);

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::CellStore* currentCell = player.getCell();

        if (currentCell && !currentCell->isExterior() && RACore::getCellTracker()
            && RACore::getCellTracker()->countLockedContainers(currentCell).remaining == 0)
        {
            const std::string cellKey
                = RAStatKeys::CellUnlockedPrefix + std::string(currentCell->getCell()->getDisplayName());
            RAStats::set(cellKey, 1);
        }

        RATriggerContext ctx{ "lock_difficulty", lockStrength };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerLockpick>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    // The engine reports every actor death here, creatures included
    void RAEventProcessor::onActorKilled(const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker)
    {
        if (victim.isEmpty())
            return;

        if (victim.getType() == ESM::NPC::sRecordId)
            onNpcKilled(victim, attacker);
        else if (victim.getType() == ESM::Creature::sRecordId)
            onCreatureKilled(victim, attacker);
    }

    void RAEventProcessor::onCreatureKilled(const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker)
    {
        Log(Debug::Info) << "[RAMW Events] Creature Killed : " << victim.getCellRef().getRefId();
        std::string creature_refid = normalizeName(std::string(victim.getClass().getName(victim)));
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::CellStore* currentCell = player.getCell();

        if (currentCell && !currentCell->isExterior() && RACore::getCellTracker()
            && RACore::getCellTracker()->countLivingActors(currentCell).remaining == 0)
        {
            const std::string cellKey
                = RAStatKeys::CellClearPrefix + std::string(currentCell->getCell()->getDisplayName());
            RAStats::set(cellKey, 1);
        }

        const MWWorld::LiveCellRef<ESM::Creature>* creatureRef = victim.get<ESM::Creature>();
        RAStats::kill(creature_refid, false, creatureRef ? creatureRef->mBase->mData.mType : -1);

        RATriggerContext ctx{ "creature_kill_" + creature_refid };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerKill>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        evaluateCellClear(ctx);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onNpcKilled(const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker)
    {
        Log(Debug::Info) << "[RAMW Events] NPC Killed : " << victim.getCellRef().getRefId();

        std::string npc_name = normalizeName(std::string(victim.getClass().getName(victim)));
        RAStats::kill(npc_name, true);

        //tracking by race/faction
        if (const MWWorld::LiveCellRef<ESM::NPC>* npcRef = victim.get<ESM::NPC>())
        {
            if (!npcRef->mBase->mRace.empty())
                RAStats::increment(
                    RAStatKeys::RaceKillPrefix + normalizeName(npcRef->mBase->mRace.serializeText()));

            if (!npcRef->mBase->mFaction.empty())
                RAStats::increment(
                    RAStatKeys::FactionKillPrefix + normalizeName(npcRef->mBase->mFaction.serializeText()));

            // Vampire NPCs run one of the three clan scripts
            if (normalizeName(npcRef->mBase->mScript.serializeText()).rfind("vampire_", 0) == 0)
                RAStats::increment(RAStatKeys::KillVampireTotal);
        }

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::CellStore* currentCell = player.getCell();

        if (currentCell && !currentCell->isExterior() && RACore::getCellTracker()
            && RACore::getCellTracker()->countLivingActors(currentCell).remaining == 0)
        {
            const std::string cellKey
                = RAStatKeys::CellClearPrefix + std::string(currentCell->getCell()->getDisplayName());
            RAStats::set(cellKey, 1);
        }

        RATriggerContext ctx{ "npc_kill_" + npc_name };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerKill>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        evaluateCellClear(ctx);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onSoulTrapped(const MWWorld::Ptr& creature)
    {
        const std::string soul = normalizeName(std::string(creature.getClass().getName(creature)));

        Log(Debug::Info) << "[RAMW Events] Soul trapped : " << soul;

        RAStats::increment(RAStatKeys::SoulsTrapped);
        RAStats::increment(RAStatKeys::SoulPrefix + soul);

        RATriggerContext ctx{ RAStatKeys::SoulPrefix + soul };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerSoulTrap>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        onStatChanged(RAStatKeys::SoulsTrapped, RAStats::get(RAStatKeys::SoulsTrapped));

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    // Scripts reveal museum pieces, shrines and props with Enable
    void RAEventProcessor::onObjectEnabled(const MWWorld::Ptr& reference)
    {
        if (sTrackedObjects.empty() || isLoading())
            return;

        const std::string key = RAStatKeys::EnabledPrefix + normalizeName(reference.getCellRef().getRefId().serializeText());

        if (sTrackedObjects.find(key) == sTrackedObjects.end())
            return;

        Log(Debug::Info) << "[RAMW Events] Object enabled : " << key;

        RAStats::set(key, 1);

        RATriggerContext ctx{ key };
        evaluateCellClear(ctx);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onAddItem(const MWWorld::Ptr& item, size_t count)
    {
        const std::string itemId = item.getCellRef().getRefId().serializeText();
        const std::string lootKey = RAStatKeys::LootPrefix + itemId;

        RAStats::increment(lootKey, static_cast<int>(count));

        if (item.getCellRef().getRefId().getRefIdString() == "gold_001")
            RAStats::increment(RAStatKeys::GoldLooted, count);
        else
            RAStats::increment(RAStatKeys::ItemsLooted, count);

        RATriggerContext ctx{ lootKey };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerLoot>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::CellStore* cell = player.getCell();

        if (cell && !cell->isExterior())
        {
            const std::string cellItemKey = RAStatKeys::CellLootPrefix
                + normalizeName(std::string(cell->getCell()->getDisplayName())) + "_" + itemId;

            if (RACore::getCellTracker() && RACore::getCellTracker()->inTrackedCell(cellItemKey))
                RAStats::increment(cellItemKey, static_cast<int>(count));
        }

        evaluateCellClear(ctx);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    // Very clunky atm, needs to find a better way to distinct the initial action from a fake one
    // We can easily drop a stolen object inside a chest and steal it again, making it trivial
    void RAEventProcessor::onItemStolen(const MWWorld::Ptr& item, size_t count)
    {
        if (item.getCellRef().getRefId().getRefIdString() != "gold_001")
            RAStats::increment(RAStatKeys::ItemsStolen, count);
    }

    void RAEventProcessor::onPickpocketing(const MWWorld::Ptr& item, int count)
    {
        Log(Debug::Info) << "[RAMW Events] Pickpocketing succeed : " << item.getCellRef().getRefId() << " x" << count;
        if (item.getCellRef().getRefId().getRefIdString() == "gold_001")
        {
            RAAchievementManager::unlock("MW_PICK_GOLD");
            // todo - migrate to ratriggers
        }
    }

    void RAEventProcessor::onLoadCell(MWWorld::CellStore& cell)
    {
        const MWWorld::Cell* new_cell = cell.getCell();

        if (!new_cell->isExterior())
        {
            Log(Debug::Info) << "[RAMW Events] Interior Cell Loaded : " << new_cell->getDisplayName();
            const std::string cellName(new_cell->getDisplayName());

            // Dungeons/caves/tombs,etc have no comma in their display name like urban buildings
            if (!cellName.empty() && cellName.find(',') == std::string::npos)
            {
                const std::string key = RAStatKeys::DiscoveryPrefix + normalizeName(cellName);
                if (RAStats::get(key) == 0)
                    onLocationDiscovered(cellName);
            }
        }

        if (RACore::getCellTracker())
            RACore::getCellTracker()->setCell(std::string(new_cell->getDisplayName()), &cell);
    }

    void RAEventProcessor::onCellEntered(MWWorld::CellStore& cell)
    {
        const std::string cellName(cell.getCell()->getDisplayName());
        if (cellName.empty())
            return;

        const std::string key = RATriggerLocation::Prefix + normalizeName(cellName);

        RATriggerContext ctx{ key };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerLocation>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onReadBook(MWWorld::LiveCellRef<ESM::Book>* book)
    {
        const std::string bookId = normalizeName(book->mBase->mId.toString());
        const bool isSkillBook = book->mBase->mData.mSkillId && book->mBase->mData.mSkillId != 0xffffffff;
        const bool isScroll = book->mBase->mData.mIsScroll != 0;

        if (!RAStats::readBook(bookId, isSkillBook, isScroll))
            return;

        Log(Debug::Info) << "[RA Events] Read Book - " << book->mBase->mName << " (" << bookId << ")";

        if (isSkillBook)
            RAAchievementManager::unlock("MW_SKILLBOOK_1"); // todo - migrate to ratriggers

        const std::string key = RAStatKeys::BookReadPrefix + bookId;

        RATriggerContext ctx{ key };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerReadBook>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    // Handle harvesting for graphic herbalism mode (one clic harvesting)
    void RAEventProcessor::onHarvest(const std::string& plantId)
    {
        Log(Debug::Info) << "[RA Events] Harvested - " << plantId;

        const std::string spotKey = RAStatKeys::HarvestPrefix + plantId;

        RAStats::increment(RAStatKeys::HarvestCount);
        RAStats::increment(spotKey);

        RATriggerContext ctx{ spotKey };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerHarvest>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    // To handle harvesting on legacy mode, we need to check if a container is organic and fully empty when we close
    // the container. It's done in order to avoid exploit by splitting the pile and increase the counter artificially
    // and to align with the graphic herbalism mode : harvest == empty plant container
    void RAEventProcessor::onContainerClosed(const MWWorld::Ptr& container)
    {
        if (container.getType() != ESM::Container::sRecordId)
            return;

        const MWWorld::LiveCellRef<ESM::Container>* ref = container.get<ESM::Container>();
        if (!(ref->mBase->mFlags & ESM::Container::Organic))
            return;

        if (container.getClass().getContainerStore(container).hasVisibleItems())
            return; // plant is not fully harvested

        onHarvest(container.getCellRef().getRefId().serializeText());
    }

    void RAEventProcessor::onLocationDiscovered(const std::string& locationName)
    {
        // keep the place only, not region
        const std::string shortName = locationName.substr(0, locationName.find(','));
        const std::string key = RAStatKeys::DiscoveryPrefix + normalizeName(shortName);

        RAStats::increment(RAStatKeys::LocationsDiscovered);
        RAStats::set(key, 1);

        if (RADiscoveryNotification::hasInstance())
            RADiscoveryNotification::getInstance().show(shortName);

        MWBase::Environment::get().getSoundManager()->playSound(ESM::RefId::stringRefId("wind calm5"), 2.0f, 1.0f);

        RATriggerContext ctx{ key };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerDiscovery>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onGlobalChanged(std::string_view name, int value)
    {
        const std::string key = RATriggerGlobal::Prefix + normalizeName(std::string(name));

        if (sTrackedGlobals.find(key) == sTrackedGlobals.end())
            return;

        RAStats::notifyProgress(key, value);
        onStatChanged(key, value);

        RATriggerContext ctx{ key, value };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerGlobal>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::rebuildTrackedGlobals()
    {
        sTrackedGlobals.clear();
        sTrackedStats.clear();
        sTrackedObjects.clear();

        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerCellClear>())
            for (const AchievementObjective& obj : info->objectives)
                if (obj.stat_name.starts_with(RAStatKeys::EnabledPrefix))
                    sTrackedObjects.insert(obj.stat_name);

        const std::string_view prefix(RATriggerGlobal::Prefix);

        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerGlobal>())
        {
            for (const AchievementObjective& obj : info->objectives)
            {
                if (obj.stat_name.starts_with(prefix))
                    sTrackedGlobals.insert(normalizeName(obj.stat_name));
                else if (!obj.stat_name.empty())
                    sTrackedStats.insert(obj.stat_name);
            }
        }
    }

    void RAEventProcessor::onStatChanged(const std::string& statName, int value)
    {
        if (mOnStatChangedCallback)
            mOnStatChangedCallback(statName);

        if (sTrackedStats.find(statName) == sTrackedStats.end())
            return;

        static bool evaluating = false;
        if (evaluating)
            return;

        evaluating = true;
        RATriggerContext ctx{ statName, value };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerGlobal>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);
        evaluating = false;
    }

    void RAEventProcessor::setOnEventCallback(std::function<void()> callback)
    {
        mOnEventCallback = callback;
    }

    void RAEventProcessor::setOntrackedCallback(std::function<void()> callback)
    {
        mOntrackedCallback = callback;
    }

    void RAEventProcessor::setOnStatChangedCallback(std::function<void(const std::string&)> callback)
    {
        mOnStatChangedCallback = callback;
    }

    void RAEventProcessor::evaluateCellClear(const RATriggerContext& ctx)
    {
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerCellClear>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (RACore::getCellTracker())
            RACore::getCellTracker()->refresh();
    }
}
