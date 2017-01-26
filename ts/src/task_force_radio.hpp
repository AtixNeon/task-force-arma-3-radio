#pragma once
#include <stdint.h>
#include <string>
#include "settings.hpp"
#include "SignalSlot.hpp"
#include "serverData.hpp"

class PlaybackHandler;
class CommandProcessor;



struct FREQ_SETTINGS {
    int volume;
    stereoMode stereoMode;
    std::string radioClassname;
};

struct SPEAKER_DATA {
    std::string radio_id;
    std::weak_ptr<clientData> client;
    Position3D getPos() const {
        if (pos.isNull()) {
            auto owner = client.lock();
            if (owner)
                return owner->getClientPosition();
        }
        return pos;
    }
    //Prefer this if you already know the owner
    Position3D getPos(const std::shared_ptr<clientData>& owner) const {
        if (pos.isNull() && owner) {
                return owner->getClientPosition();
        }
        return pos;
    }
    Position3D pos;
    int volume;
    vehicleDescriptor vehicle;
    float waveZ;
};

//Data from game about the local player
class gameData {
public:

    void setFreqInfos(const std::vector<std::string>& tokens) {
        //FREQ, str(_freq), str(_freq_lr)
        //_alive, speakVolume, _nickname, 
        //waves, TF_terrain_interception_coefficient, _globalVolume,
        //_receivingDistanceMultiplicator, TF_speakerDistance
        LockGuard_exclusive<ReadWriteLock> lock(&m_lock);
        mySwFrequencies = helpers::parseFrequencies(tokens[1]);
        myLrFrequencies = helpers::parseFrequencies(tokens[2]);
        alive = tokens[3] == "true";


        //auto clientDataDir = TFAR::getServerDataDirectory()->getClientDataDirectory(serverConnectionHandlerID);
        //if (clientDataDir && clientDataDir->myClientData && clientDataDir->myClientData->clientTalkingNow) {
        //#TODO resend teamspeak VOLUME command if myVoiceVolume changed while talking
        //}

        myVoiceVolume = helpers::parseArmaNumberToInt(tokens[4]);
        wavesLevel = helpers::parseArmaNumber(tokens[6]);
        terrainIntersectionCoefficient = helpers::parseArmaNumber(tokens[7]);
        globalVolume = helpers::parseArmaNumber(tokens[8]);
        receivingDistanceMultiplicator = helpers::parseArmaNumber(tokens[9]);
        speakerDistance = helpers::parseArmaNumber(tokens[10]);
    }
    mutable ReadWriteLock m_lock;
    std::map<std::string, FREQ_SETTINGS> mySwFrequencies;
    std::map<std::string, FREQ_SETTINGS> myLrFrequencies;

    std::multimap<std::string, SPEAKER_DATA> speakers;

    int myVoiceVolume{ 0 };
    bool alive{ false };
    float wavesLevel{ 0 };
    float terrainIntersectionCoefficient{ 7.f };
    float globalVolume{ 1.f };
    float receivingDistanceMultiplicator{ 1.f };
    float speakerDistance{ 20.f };

    int currentDataFrame{ INVALID_DATA_FRAME };


    std::string getCurrentTransmittingRadio() const { LockGuard_shared<ReadWriteLock> lock(&m_lock); return currentTransmittingRadio; }
    void setCurrentTransmittingRadio(std::string val) { LockGuard_exclusive<ReadWriteLock> lock(&m_lock); currentTransmittingRadio = val; }
    bool tangentPressed{ false };
    std::chrono::milliseconds pttDelay{ 0ms };
private:
    std::string currentTransmittingRadio{ "" };//Used for half-duplex mode
};


class TFAR {
public:
    TFAR();
    ~TFAR();


    static TFAR& getInstance();

    static void trackPiwik(const std::vector<std::string>& piwikData);
    static void createCheckForUpdateThread();
    static std::shared_ptr<CommandProcessor>& getCommandProcessor();
    static std::shared_ptr<PlaybackHandler>& getPlaybackHandler();
    static std::shared_ptr<serverDataDirectory>& getServerDataDirectory();
    static settings config;//I'd like to use settings as the variable name. But... meh


    Signal<void()> onGameStart;
    Signal<void()> onGameEnd;

    Signal<void()> onGameConnected;//Called when Pipe connected to game
    Signal<void()> onGameDisconnected;//Called when Pipe disconnected from game

    Signal<void()> onShutdown;

    Signal<void(std::stringstream&)> doDiagReport;


    //Teamspeak events. They are here because Teamspeak class is fully static
    Signal<void(TSServerID serverID)> onTeamspeakServerConnect;
    Signal<void(TSServerID serverID)> onTeamspeakServerDisconnect;
    Signal<void(TSServerID serverID, TSClientID clientID, const std::string& clientNickname)> onTeamspeakClientJoined;
    Signal<void(TSServerID serverID, TSClientID clientID)> onTeamspeakClientLeft; //If clientID == -2 all clients Left (aka channel switched)
    Signal<void(TSServerID serverID, TSClientID clientID, const std::string& clientNickname)> onTeamspeakClientUpdated; //Some variable about him updated. Probably his nickname

    //Variable accessors
    std::string getPluginPath() const { return pluginPath; }
    void setPluginPath(const std::string& val) { pluginPath = val; }
    std::string getPluginID() const { return pluginID; }
    void setPluginID(const std::string& val) { pluginID = val; }
    bool getCurrentlyInGame() const { return currentlyInGame; }
    void setCurrentlyInGame(bool val) { currentlyInGame = val; }

    gameData m_gameData;


    struct Version {
        explicit Version(std::string versionStr) {
            auto split = helpers::split(versionStr, '.');
            for (auto& it : split) {
                versionNumbers.push_back(std::stoi(it));
            }
        }

        bool operator>(const Version &otherVersion) const {
            for (size_t i = 0; i < std::min(versionNumbers.size(), otherVersion.versionNumbers.size()); ++i) {
                if (versionNumbers[i] < otherVersion.versionNumbers[i])
                    return false;
                if (versionNumbers[i] > otherVersion.versionNumbers[i])
                    return true;
            }
            return false;
        }
        std::vector<int> versionNumbers;
    };

private:
    std::string pluginPath;
    std::string pluginID;
    static bool isUpdateAvailable();
    std::shared_ptr<PlaybackHandler> m_playbackHandler;
    std::shared_ptr<CommandProcessor> m_commandProcessor;
    std::shared_ptr<serverDataDirectory> m_serverData;
    bool currentlyInGame;


};
