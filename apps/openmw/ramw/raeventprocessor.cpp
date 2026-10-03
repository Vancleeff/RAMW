#include "raeventprocessor.hpp"
#include "raachievement.hpp"
#include "racore.hpp"
#include "rastats.hpp"
#include "ratriggerHerostats.hpp"
#include "ratriggerQuestComplete.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadnpc.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwclass/nameorid.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwworld/cellref.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

namespace RAMW
{
    std::function<void()> RAEventProcessor::mOnEventCallback = nullptr;
    std::function<void()> RAEventProcessor::mOntrackedCallback = nullptr;

    void RAEventProcessor::init()
    {
        Log(Debug::Info) << "[RAMW Events] Event processor initialized";
    }

    void RAEventProcessor::onGameSaved(ESM::ESMWriter& writer)
    {
        RAStats::write(writer);
    }

    void RAEventProcessor::onGameCleanup()
    {
        RAStats::init();
    }

    void RAEventProcessor::onGameLoaded(ESM::ESMReader& reader)
    {
        RAStats::readRecord(reader);
        RACore::onGameReady();
        if (mOnEventCallback && RAMW::RACore::isInitialized())
            mOnEventCallback();
        if (mOntrackedCallback && RAMW::RACore::isInitialized())
            mOntrackedCallback();
    }

    void RAEventProcessor::onDialogueStarted(const std::string& npcId)
    {
        if (npcId == "caius cosades")
        {
            RAAchievementManager::unlock("MQ_1");
        }
        // todo - migrate to ratriggers
    }

    void RAEventProcessor::onQuestProgress(const std::string& questId, int stage)
    {
        Log(Debug::Info) << "[RAMW Events] Quest progressed - Topic: " << questId << " Index: " << stage;

        if (questId == "MV_DeadTaxman" && (stage >= 90))
        {
            RAAchievementManager::unlock("QUEST_TAXMAN");
        }
        // todo - migrate to ratriggers
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

        RATriggerContext ctx{ "level", newLevel };
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

        RATriggerContext ctx{ "skill_" + skillId, newValue };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerHeroStats>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onOpenLock(int lock_str)
    {
        Log(Debug::Info) << "[RAMW Events] Lock succeeded - lock level : " << std::to_string(lock_str);
        RAStats::increment(RAStatKeys::LockpickCount);
        if (mOnEventCallback)
            mOnEventCallback();
        if (lock_str >= 100)
            RAAchievementManager::unlock("LOCK_100");
        if (lock_str >= 50)
            RAAchievementManager::unlock("LOCK_50");
        if (lock_str >= 25)
            RAAchievementManager::unlock("LOCK_25");
        if (lock_str > 0)
            RAAchievementManager::unlock("LOCK_10");
        // todo - migrate to ratriggers
    }

    void RAEventProcessor::onCreatureKilled(const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker)
    {
        Log(Debug::Info) << "[RAMW Events] Creature Killed : " << victim.getCellRef().getRefId();
        std::string creature_refid = victim.getCellRef().getRefId().toString();
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::CellStore* currentCell = player.getCell();

        if (currentCell && !currentCell->isExterior())
            RAEventProcessor::countEnemiesInCell(currentCell);

        RAStats::kill(creature_refid, false);

        RATriggerContext ctx{ "creature_kill_" + creature_refid };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerKill>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onNpcKilled(const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker)
    {
        Log(Debug::Info) << "[RAMW Events] NPC Killed : " << victim.getCellRef().getRefId();

        std::string npc_name = victim.getCellRef().getRefId().toString();
        RAStats::kill(npc_name, true);

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::CellStore* currentCell = player.getCell();
        if (currentCell && !currentCell->isExterior())
            RAEventProcessor::countEnemiesInCell(currentCell);

        RATriggerContext ctx{ "npc_kill_" + npc_name };
        for (AchievementData* info : RAAchievementManager::getAchievementsByTrigger<RATriggerKill>())
            if (info->trigger->evaluate(ctx, *info))
                RAAchievementManager::unlock(info->id);

        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();
    }

    void RAEventProcessor::onAddItem(const MWWorld::Ptr& item, size_t count)
    {
        Log(Debug::Info) << "[RAMW Events] Add item : " << item.getCellRef().getRefId() << " x" << count
                         << " from Owner : " << item.getCellRef().getOwner();

        if (item.getCellRef().getOwner() == ESM::RefId::stringRefId("Player"))
            return;
        if (item.getCellRef().getRefId().getRefIdString() == "gold_001")
            RAStats::increment(RAStatKeys::GoldLooted, count);
        else
            RAStats::increment(RAStatKeys::ItemsLooted, count);
        if (mOnEventCallback)
            mOnEventCallback();
        if (mOntrackedCallback)
            mOntrackedCallback();

        if (item.getCellRef().getRefId().getRefIdString() == "bk_falljournal_unique")
        {
            RAAchievementManager::unlock("LOOT_TARHIEL_JOURNAL");
            // todo - migrate to ratriggers
        }

        if (item.getCellRef().getRefId().getRefIdString() == "chargen statssheet")
        {
            RAAchievementManager::unlock("MQ_0");
            // todo - migrate to ratriggers
        }
    }

    // Very clunky atm, needs to find a better way to distinct the initial action from a fake one
    // We can easily drop a stolen object inside a chest and steal it again, making it trivial
    void RAEventProcessor::onItemStolen(const MWWorld::Ptr& item, size_t count)
    {
        int init_value = RAStats::get(RAStatKeys::ItemsLooted);
        if (item.getCellRef().getRefId().getRefIdString() != "gold_001")
            RAStats::increment(RAStatKeys::ItemsStolen, count);

        if (init_value < RAAchievementManager::getAchievementData("STEAL_100")->target_value
            && RAStats::get(RAStatKeys::ItemsLooted)
                >= RAAchievementManager::getAchievementData("STEAL_100")->target_value)
        {
            RAAchievementManager::unlock("STEAL_100");
            // todo - migrate to ratriggers
        }
    }

    void RAEventProcessor::onPickpocketing(const MWWorld::Ptr& item, int count)
    {
        Log(Debug::Info) << "[RAMW Events] Pickpocketing succeed : " << item.getCellRef().getRefId() << " x" << count;
        if (item.getCellRef().getRefId().getRefIdString() == "gold_001")
        {
            RAAchievementManager::unlock("PICK_GOLD");
            // todo - migrate to ratriggers
        }
    }

    void RAEventProcessor::onLoadCell(MWWorld::CellStore& cell)
    {
        const MWWorld::Cell* new_cell = cell.getCell();
        if (!new_cell->isExterior())
        {
            Log(Debug::Info) << "[RAMW Events] Interior Cell Loaded : " << cell.getCell()->getDisplayName();
            RAEventProcessor::countEnemiesInCell(&cell);
        }
    }

    void RAEventProcessor::onReadBook(MWWorld::LiveCellRef<ESM::Book>* book)
    {
        Log(Debug::Info) << "[RAMW Events] Read Book : " << book->mBase->mName
                         << " (SKILL ID : " << book->mBase->mData.mSkillId;
        std::string bookId = book->mBase->mId.toString();
        if (book->mBase->mData.mSkillId && book->mBase->mData.mSkillId != 0xffffffff)
        {
            RAStats::readBook(bookId, true);
            RAAchievementManager::unlock("SKILLBOOK_1");
            // todo - migrate to ratriggers
        }
        else
            RAStats::readBook(bookId);
        if (mOnEventCallback)
            mOnEventCallback();
    }

    void RAEventProcessor::setOnEventCallback(std::function<void()> callback)
    {
        mOnEventCallback = callback;
    }

    void RAEventProcessor::setOntrackedCallback(std::function<void()> callback)
    {
        mOntrackedCallback = callback;
    }

    // Seems very reliable now with levListVisitor included. To test massively.
    // Could be used to define "clean location x" kind of achievement when no other strict condition is possible.
    void RAEventProcessor::countEnemiesInCell(MWWorld::CellStore* cell)
    {
        int aliveCreatures = 0;
        int deadCreatures = 0;

        auto creatureVisitor = [&](const MWWorld::Ptr& ptr) {
            if (!MWWorld::CellStore::isAccessible(ptr.getRefData(), ptr.getCellRef()))
                return true;
            const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
            if (stats.isDead())
                deadCreatures++;
            else
                aliveCreatures++;
            return true;
        };

        auto levListVisitor = [&](const MWWorld::Ptr& ptr) {
            if (!MWWorld::CellStore::isAccessible(ptr.getRefData(), ptr.getCellRef()))
                return true;
            if (ptr.getRefData().getCustomData() == nullptr)
                aliveCreatures++;
            return true;
        };

        cell->forEachType<ESM::Creature>(creatureVisitor, true);
        cell->forEachType<ESM::NPC>(creatureVisitor, true);
        cell->forEachType<ESM::CreatureLevList>(levListVisitor, true);

        Log(Debug::Info) << "[RAMW] Cell " << cell->getCell()->getDisplayName() << " - Alive: " << aliveCreatures
                         << ", Dead: " << deadCreatures;
    }

}
