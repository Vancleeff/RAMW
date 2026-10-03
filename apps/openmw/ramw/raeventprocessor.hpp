#ifndef RAMW_RAEVENTPROCESSOR_HPP
#define RAMW_RAEVENTPROCESSOR_HPP

#include <string>
#include "../mwworld/ptr.hpp"

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

        static void onDialogueStarted(const std::string& npcId);
        static void onQuestProgress(const std::string& questId, int stage);
        static void onQuestFinished(const std::string& questId);
        static void onLevelUp(int newLevel);
        static void onSkillLevelUp(const std::string& skillId, int newValue, const void* statsPtr);
        static void onOpenLock(int lock_str);
        static void onCreatureKilled(const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker);
        static void onNpcKilled(const MWWorld::Ptr& victim, const MWWorld::Ptr& attacker);
        static void onAddItem(const MWWorld::Ptr& item, size_t count);
        static void onItemStolen(const MWWorld::Ptr& item, size_t count);
        static void onPickpocketing(const MWWorld::Ptr& victim, int count);
        static void onLoadCell(MWWorld::CellStore& Cell);
        static void onReadBook(MWWorld::LiveCellRef<ESM::Book>* book);

        static void setOnEventCallback(std::function<void()> callback);
        static void setOntrackedCallback(std::function<void()> callback);

        static void countEnemiesInCell(MWWorld::CellStore* cell);


    private:
        static std::function<void()> mOnEventCallback;
        static std::function<void()> mOntrackedCallback;
    };

}

#endif
