#ifndef RAMW_RAMODS_HPP
#define RAMW_RAMODS_HPP

#include <set>
#include <string>
#include <vector>

namespace RAMW
{
    class RAMods
    {
    public:
        static bool checkLoadedMods(const std::vector<std::string>& loadedMods);
        static bool isModAllowed(const std::string& modName);

        static std::vector<std::string> getUnauthorizedMods(const std::vector<std::string>& loadedMods);

    private:
        static const std::set<std::string> sAllowedMods;

        static std::string cleanModName(const std::string& modName);
    };
}

#endif // RAMW_RAMODS_HPP
