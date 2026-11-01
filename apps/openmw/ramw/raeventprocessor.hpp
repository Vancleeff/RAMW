#ifndef RAMW_RAEVENTPROCESSOR_HPP
#define RAMW_RAEVENTPROCESSOR_HPP

#include <string>
#include <unordered_set>
#include "../mwworld/ptr.hpp"
#include "ratriggers.hpp"

#include "../mwbase/environment.hpp"
#include <components/esm3/loadbook.hpp>

 namespace MWWorld
{
    class Ptr;
    class CellStore;
    class esmStore;
    template <typename T>
    class LiveCellRef;
}
namespace ESM
{
    struct Book;
}

namespace RAMW
{
    class RAEventProcessor
    {
    public:
        static void init();

        //Hooks
        static void onGameSaved(ESM::ESMWriter& writer);
        static void onGameLoaded(ESM::ESMReader& reader);
        static void onGameCleanup();

        static void onTopic(const std::string& topicId, const std::string& npcId);
        static void onQuestProgress(const std::string& questId, int stage);
        static void onQuestFinished(const std::string& questId);
        static void onLevelUp(int newLevel);
        static void onSkillLevelUp(const std::string& skillId, int newValue, const void* statsPtr);
        static void onReputationChanged(int newValue, const void* statsPtr);
        static void onOpenLock(int lock_str);
        static void onActorKilled(const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker);
        static void onCreatureKilled(const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker);
        static void onNpcKilled(const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker);
        static void onSoulTrapped(const MWWorld::Ptr& creature);
        static void onObjectEnabled(const MWWorld::Ptr& reference);
        static void onAddItem(const MWWorld::Ptr& item, size_t count);
        static void onItemStolen(const MWWorld::Ptr& item, size_t count);
        static void onPickpocketing(const MWWorld::Ptr& victim, int count);
        static void onLoadCell(MWWorld::CellStore& Cell);
        static void onCellEntered(MWWorld::CellStore& cell);
        static void onReadBook(MWWorld::LiveCellRef<ESM::Book>* book);
        static void onHarvest(const std::string& plantId);
        static void onContainerClosed(const MWWorld::Ptr& container);
        static void onLocationDiscovered(const std::string& locationName);
        static void onGlobalChanged(std::string_view name, int value);
        static void onStatChanged(const std::string& statName, int value);
        static void onTraining(const std::string& skill);
        static void onSpellCast(const std::string& spellId, const std::string& school);
        static void onTravel(const MWWorld::Ptr& service, const std::string& destination);
        static void updateSpellCount(float dt);

        static void evaluateCellClear(const RATriggerContext& ctx);
        static void setOnEventCallback(std::function<void()> callback);
        static void setOntrackedCallback(std::function<void()> callback);
        static void setOnStatChangedCallback(std::function<void(const std::string&)> callback);

        // rebuild the set of globals we need to track, based on the achievements json
        static void rebuildTrackedGlobals();

        // True from the start of a load until the first gameframe
        static bool isLoading();
        static void setLoading(bool loading);

    private:
        static std::function<void()> mOnEventCallback;
        static std::function<void()> mOntrackedCallback;
        static std::function<void(const std::string&)> mOnStatChangedCallback;
        static std::unordered_set<std::string> sTrackedGlobals;
        static std::unordered_set<std::string> sTrackedStats;
        static std::unordered_set<std::string> sTrackedObjects;
        static bool sLoading;
    };

}

#endif
