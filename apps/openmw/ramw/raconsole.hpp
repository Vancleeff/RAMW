#ifndef RAMW_RACONSOLE_HPP
#define RAMW_RACONSOLE_HPP

#include "../mwgui/console.hpp"
#include <set>
#include <string>

namespace RAMW
{
    class RAConsole
    {
    public:
        static bool isCommandAllowed(const std::string& command);
        static bool interceptCommand(MWGui::Console* console, const std::string& command);

    private:
        static const std::set<std::string> sAllowedCommands;
        static std::string extractCommandName(const std::string& command);
        static void execute(MWGui::Console* console, const std::string& command);
    };
}

#endif // RAMW_RACONSOLE_HPP
