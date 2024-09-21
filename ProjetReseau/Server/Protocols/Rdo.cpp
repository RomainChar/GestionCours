#include "Rdo.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "ProtocolHelper.h"
#include "Wrdo.h"

Rdo::Rdo(std::map<std::string, json>& serverResources, std::map<std::string, std::vector<int>>& watchedValues, json* config)
    : serverResources(serverResources), watchedValues(watchedValues), config(config) {}

json Rdo::HandleRequest(json request, int clientSocket, Wrdo* wrdo)
{
    json response;

    std::string operation = request["operation"];

    if (operation == "GET")
        response = HandleRead(request, clientSocket, wrdo);

    else if (operation == "POST")
        response = HandleWrite(request, clientSocket, wrdo);

    return response;
}

json Rdo::HandleRead(json request, int clientSocket, Wrdo* wrdo)
{
    std::string resourceId = request["rsrcId"];

    json response;

    // Resource not found
    if (!serverResources.count(resourceId))
    {

        printf("\nRessource %s non trouvée.\n", resourceId.c_str());
        response = json::object({{"server", (*config)["ServerIP"]},
                                 {"code", "404"},
                                 {"rsrcId", resourceId},
                                 {"data", ""},
                                 {"message", "ressource inconnue"}});
    }
    else
    {
        printf("\nRessource %s trouvée.\n", resourceId.c_str());
        json data = ExtractResources(serverResources.at(resourceId), clientSocket, wrdo);

        response = json::object({{"server", (*config)["ServerIP"]},
                                 {"code", "200"},
                                 {"rsrcId", resourceId},
                                 {"data", data},
                                 {"message", "ressource trouvée"}});
    }

    return response;

    // TODO: Handle resource request by reference
}

json Rdo::HandleWrite(json request, int clientSocket, Wrdo* wrdo)
{
    std::string resourceId = request["rsrcId"];

    json response;

    // Resource not found

    if (!serverResources.count(resourceId))
    {
        // We add the resource to the server
        printf("\nCréation de la ressource %s\n", resourceId.c_str());
        serverResources.insert({resourceId, request["data"]});

        response = json::object({{"server", (*config)["ServerIP"]},
                                 {"code", "201"},
                                 {"rsrcId", resourceId},
                                 {"message", "ressource créée"}});
    }
    else
    {
        printf("\nModification de la ressource %s existante\n", resourceId.c_str());
        serverResources.at(resourceId) = request["data"];

        response = json::object({{"server", (*config)["ServerIP"]},
                                 {"code", "211"},
                                 {"rsrcId", resourceId},
                                 {"message", "ressource modifiée"}});
    }

    ProtocolHelper::HandleWatchedValues(&watchedValues, resourceId, request["data"], config);
    
    return response;
}

json Rdo::ExtractResources(json request, int clientSocket, Wrdo* wrdo)
{
    printf("\nExtraction des ressources...\n");
    std::vector<json> referencedResources;

    std::string currentReference;

    std::string dataString = request.dump();

    bool foundSpecialChar = false;

    for (char c : dataString)
    {
        if (foundSpecialChar)
        {
            if (c == '"')
            {
                printf("\nRéférence %s trouvée\n", currentReference.c_str());
                //Get the resource by reference
                int pos;

                json resource = ExtractResource(currentReference, clientSocket, wrdo);

                std::string test = resource.dump();

                //Replace the reference string by the json string of the resource
                pos = dataString.find(currentReference) - 1;

                dataString.replace(pos, currentReference.length() + 2, test);

                currentReference.clear();

                foundSpecialChar = false;
            }
            else
                currentReference += c;
        }
        else
        {
            if (c == '$')
            {
                foundSpecialChar = true;

                currentReference += c;
            }
        }
    }
    return json::parse(dataString);
}

json Rdo::ExtractResource(std::string reference, int clientSocket, Wrdo* wrdo)
{

    printf("\nExtraction de la ressource %s\n", reference.c_str());
    std::string protocol;
    std::string serverIP;
    std::string serverPort;
    std::string resourceID;

    json resource;
    json response;
    json request;

    ProtocolHelper::ExtractRequestFields(reference, protocol, serverIP, serverPort, resourceID, config);

    // Check if the reference is directed at the server
    if (serverIP == (*config)["ServerIP"].get<std::string>() && stoi(serverPort) == (*config)["Port"])
    {
        // Formulate the request to send
        request = json::object({{"operation", "GET"},
                                {"rsrcId", resourceID}});

        // Request according to the protocol
        if (protocol == "rdo")
            response = HandleRequest(request, clientSocket, wrdo);
        else if (protocol == "wrdo")
            response = wrdo->HandleRequest(request, clientSocket, this);
        else
        {
            resource = json::object({{"server", (*config)["ServerIP"]},
                                     {"code", "400"},
                                     {"rsrcId", resourceID},
                                     {"message", "protocole inconnu"}});
        }
        // Handle the response
        if (response["code"] == "404") 
        {
            resource = json::object({{"server", (*config)["ServerIP"]},
                                 {"code", "404"},
                                 {"data", nullptr}});
        }
        else 
        {
            resource = json::object({{"server", (*config)["ServerIP"]},
                                 {"code", "200"},
                                 {"data", response["data"]}});
        }
    }
    // The reference is not directed at the server
    else 
    {
        int socket = ProtocolHelper::ConnectToServer(serverIP, stoi(serverPort));

        //Successful connection
        if (socket)
        {
            char message[BUFLEN] = {};
            char responseFromServer[BUFLEN] = {};

            json messageJson = json::object({
                {"protocol", protocol},
                {"operation", "GET"},
                {"rsrcId", resourceID}
            });

            std::string jsonStr = messageJson.dump();

            strncpy(message, jsonStr.c_str(), std::min(jsonStr.size(), static_cast<size_t>(BUFLEN - 1)));

            write(socket, message, BUFLEN);

            printf("\nDemande de la ressource %s a l'adresse suivante %s\n", resourceID.c_str(), (serverIP + ":" + serverPort).c_str());

            read(socket, responseFromServer, BUFLEN);

            resource = json::parse(responseFromServer);
        }
        else
        {
            resource = json::object({{"server", (*config)["ServerIP"]},
                                     {"code", "404"},
                                     {"data", nullptr}});
        }
        close(socket);
    }

    return resource;
}