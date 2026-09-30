#ifndef RAMW_RACLIENT_HPP
#define RAMW_RACLIENT_HPP

#include <memory>
#include <string>

namespace RAMW
{
    class RAClient
    {
    public:
        static RAClient& getInstance();
        static void createInstance();
        static void destroyInstance();

        bool authenticate();
        bool startSession(int gameId, const std::string& playerUsername);
        bool sendHeartbeat(int gameId, const std::string& playerUsername, const std::string& richPresence = "");
        static std::string downloadUserIcon(const std::string& username);

        const std::string& getPlayerUsername() const { return mPlayerUsername; }

        RAClient(const RAClient&) = delete;
        RAClient& operator=(const RAClient&) = delete;

        ~RAClient();

    private:
        RAClient();

        std::string mUsername;
        std::string mPassword;
        std::string mToken;
        std::string mPlayerUsername;

        static std::unique_ptr<RAClient> sInstance;
    };
}

#endif
