#pragma once
#include <vector>
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

class ProtocolHelper
{
public:
    static void ExtractRequestFields(std::string reference, std::string& protocol, std::string& serverIP,
    std::string& serverPort, std::string& resourceID, json* config);

    static int ConnectToServer(std::string serverIpAddress, int port);

    static void HandleWatchedValues(std::map<std::string, std::vector<int>> *watchedValues, std::string resourceID, json data, json* config);
};