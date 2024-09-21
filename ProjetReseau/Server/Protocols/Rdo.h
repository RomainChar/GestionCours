#pragma once
#include "../nlohmann/json.hpp"

#define BUFLEN 512

using json = nlohmann::json;

class Wrdo;

class Rdo
{
private:
    json HandleRead(json request, int clientSocket, Wrdo* wrdo);

    json HandleWrite(json request, int clientSocket, Wrdo* wrdo);

    json ExtractResources(json request, int clientSocket, Wrdo* wrdo);

    json ExtractResource(std::string reference, int clientSocket, Wrdo* wrdo);

public:
    std::map<std::string, json>& serverResources;
    std::map<std::string, std::vector<int>>& watchedValues;
    json* config;

    Rdo(std::map<std::string, json>& serverResources, std::map<std::string, std::vector<int>>& watchedValues, json* config);

    json HandleRequest(json request, int clientSocket, Wrdo* wrdo);
};