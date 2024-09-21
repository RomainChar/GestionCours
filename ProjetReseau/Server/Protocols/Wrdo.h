#pragma once
#include "../nlohmann/json.hpp"

#define BUFLEN 512

using json = nlohmann::json;

class Rdo;

class Wrdo
{
private:
    json HandleRead(json request, int clientSocket, Rdo* rdo);

    json HandleWrite(json request, int clientSocket, Rdo* rdo);

    json ExtractResources(json request, int clientSocket, Rdo* rdo);

    json ExtractResource(std::string reference, int clientSocket, Rdo* rdo);

public:
    std::map<std::string, json>& serverResources;
    std::map<std::string, std::vector<int>>& watchedValues;
    json* config;

    Wrdo(std::map<std::string, json>& serverResources, std::map<std::string, std::vector<int>>& watchedValues, json* config);

    json HandleRequest(json request, int clientSocket, Rdo* rdo);
};