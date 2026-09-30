#include "raclient.hpp"
#include "achievementnotification.hpp"
#include "raconfig.hpp"
#include <components/debug/debuglog.hpp>
#include <components/files/configurationmanager.hpp>
#include <curl/curl.h>
#include <fstream>
#include <iostream>

namespace RAMW
{
    // Currently, RAClient requires a local RAWeb instance to be working or the TEST_MODE==true in raconfig
    std::unique_ptr<RAClient> RAClient::sInstance = nullptr;

    RAClient& RAClient::getInstance()
    {
        if (!sInstance)
        {
            createInstance();
        }
        return *sInstance;
    }

    void RAClient::createInstance()
    {
        if (sInstance)
            return;
        sInstance = std::unique_ptr<RAClient>(new RAClient());
    }

    void RAClient::destroyInstance()
    {
        sInstance.reset();
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
    {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    RAClient::RAClient()
        : mUsername("xxxx")
        , mPassword("xxxx")
    {
    }

    RAClient::~RAClient() {}

    bool RAClient::authenticate()
    {
        if (RAConfig::TEST_MODE)
        {
            AchievementNotification::getInstance().showCustom(
                "RAMW - Authentication - TEST MODE", "Authentication succeed", "Connected as TEST USER", "");
            return true;
        }
        CURL* curl = curl_easy_init();
        if (!curl)
        {
            Log(Debug::Error) << "[RAMW] ERROR: can't init libcurl";
            return false;
        }

        std::string url = "http://localhost:64000/dorequest.php?u=" + mUsername + "&p=" + mPassword + "&r=login2";
        std::string response;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "OpenMW-RA/0.1.0 (Client)");

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
        {
            Log(Debug::Error) << "[RAMW] Authentication failed : " << curl_easy_strerror(res);
            return false;
        }

        if (response.find("\"Success\":true") != std::string::npos)
        {
            Log(Debug::Info) << "[RAMW] Authentication succeed";

            size_t tokenPos = response.find("\"Token\":\"");
            if (tokenPos != std::string::npos)
            {
                size_t startPos = tokenPos + 9;
                size_t endPos = response.find("\"", startPos);
                if (endPos != std::string::npos)
                {
                    mToken = response.substr(startPos, endPos - startPos);
                }
            }

            size_t userPos = response.find("\"User\":\"");
            if (userPos != std::string::npos)
            {
                size_t startPos = userPos + 8;
                size_t endPos = response.find("\"", startPos);
                if (endPos != std::string::npos)
                {
                    mPlayerUsername = response.substr(startPos, endPos - startPos);
                }
            }

            std::string iconPath = RAClient::downloadUserIcon(mPlayerUsername);
            AchievementNotification::getInstance().showCustom(
                "[RAMW] Authentication", "Authentication succeed", "Connected as " + mPlayerUsername, iconPath);

            return true;
        }
        else
        {
            Log(Debug::Error) << "[RAMW] Error - Auth failed : " << response;
            return false;
        }
    }

    std::string RAClient::downloadUserIcon(const std::string& username)
    {
        CURL* curl = curl_easy_init();
        if (!curl)
        {
            Log(Debug::Error) << "[RAMW] Failed to init curl for user icon";
            return "";
        }

        std::string iconUrl = "http://localhost:64000/UserPic/" + username + ".png";
        std::vector<unsigned char> pngData;

        auto writeFunc = [](void* ptr, size_t size, size_t nmemb, void* stream) -> size_t {
            std::vector<unsigned char>* vec = static_cast<std::vector<unsigned char>*>(stream);
            unsigned char* data = static_cast<unsigned char*>(ptr);
            vec->insert(vec->end(), data, data + (size * nmemb));
            return size * nmemb;
        };

        curl_easy_setopt(curl, CURLOPT_URL, iconUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +writeFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &pngData);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "OpenMW-RA/0.1.0 (Client)");

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK || pngData.empty())
        {
            Log(Debug::Error) << "[RAMW] Failed to download user icon: " << curl_easy_strerror(res);
            return "";
        }

        Log(Debug::Info) << "[RAMW] User icon downloaded : " << pngData.size() << " bytes";

        return AchievementNotification::createTextureFromPNG("ra_usericon_" + username, pngData);
    }

    bool RAClient::startSession(int gameId, const std::string& playerUsername)
    {
        if (RAConfig::TEST_MODE)
        {
            Log(Debug::Info) << "[RAMW] TEST USER Session started for game " << gameId;
            return true;
        }

        if (mToken.empty())
        {
            Log(Debug::Error) << "[RAMW] Cannot start session: token empty";
            return false;
        }

        CURL* curl = curl_easy_init();
        if (!curl)
        {
            Log(Debug::Error) << "[RAMW] Failed to init curl";
            return false;
        }

        std::string url = "http://localhost:64000/dorequest.php?u=" + mUsername + "&t=" + mToken + "&r=startsession"
            + "&g=" + std::to_string(gameId) + "&k=" + playerUsername;

        std::string response;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "OpenMW-RA/0.1.0 (Client)");

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
        {
            Log(Debug::Error) << "[RAMW] Start session failed: " << curl_easy_strerror(res);
            return false;
        }

        if (response.find("\"Success\":true") != std::string::npos)
        {
            Log(Debug::Info) << "[RAMW] Session started for game " << gameId;
            return true;
        }
        else
        {
            Log(Debug::Error) << "[RAMW] Start session failed - no success in response";
            return false;
        }
    }

    bool RAClient::sendHeartbeat(int gameId, const std::string& playerUsername, const std::string& richPresence)
    {
        if (RAConfig::TEST_MODE)
        {
            return true;
        }

        if (mToken.empty())
        {
            Log(Debug::Error) << "[RAMW] Cannot send heartbeat: not authenticated";
            return false;
        }

        CURL* curl = curl_easy_init();
        if (!curl)
        {
            Log(Debug::Error) << "[RAMW] Failed to init curl for heartbeat";
            return false;
        }

        std::string url = "http://localhost:64000/dorequest.php?u=" + mUsername + "&t=" + mToken + "&r=ping"
            + "&g=" + std::to_string(gameId) + "&k=" + playerUsername;

        std::string response;

        curl_mime* mime = curl_mime_init(curl);
        curl_mimepart* part = curl_mime_addpart(mime);
        curl_mime_name(part, "m");
        curl_mime_data(part, richPresence.c_str(), CURL_ZERO_TERMINATED);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "OpenMW-RA/0.1.0 (Client)");

        CURLcode res = curl_easy_perform(curl);

        curl_mime_free(mime);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
        {
            Log(Debug::Error) << "[RAMW] Heartbeat failed: " << curl_easy_strerror(res);
            return false;
        }

        if (response.find("\"Success\":true") != std::string::npos)
        {
            Log(Debug::Info) << "[RAMW] Heartbeat sent successfully" << response;
            return true;
        }
        else
        {
            Log(Debug::Error) << "[RAMW] Heartbeat failed: " << response;
            return false;
        }
    }

}
