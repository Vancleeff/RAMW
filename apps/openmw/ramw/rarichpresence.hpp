#ifndef RAMW_RARICHPRESENCE_HPP
#define RAMW_RARICHPRESENCE_HPP

#include <string>

namespace RAMW
{

    class RARichPresence
    {
    public:
        static void init();
        static void update(); 

        static std::string getRP();
        static void display();

    private:
        static std::string format();

        static std::string mCachedRP;
        static float mUpdTimer;
        static const float UPDATE_INTERVAL; 
    };

}

#endif
