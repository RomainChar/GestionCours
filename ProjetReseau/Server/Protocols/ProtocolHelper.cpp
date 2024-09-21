#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ProtocolHelper.h"
#include <iostream>

void ProtocolHelper::ExtractRequestFields(std::string reference, std::string& protocol, std::string& serverIP,
std::string& serverPort, std::string& resourceID, json* config)
{
    std::stringstream ss(reference);

    //Ignore the $
    ss.ignore(1);
    
    // Parse the other informations
    std::getline(ss, protocol, ':');

    //Ignore the //
    ss.ignore(2);

    std::getline(ss, serverIP, ':');
    std::getline(ss, serverPort, '/');
    std::getline(ss, resourceID);

    std::string configIp = (*config)["ServerIP"];
    std::string configPort = (*config)["Port"].dump();
}

int ProtocolHelper::ConnectToServer(std::string serverIpAddress, int port)
{
    int clientSocket;

    // create socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) // <<< TCP socket
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // setup address structure
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(serverIpAddress.c_str());

    if (connect(clientSocket, (struct sockaddr *)&server, sizeof(server)) != 0)
    {
        return 0;
    }

    return clientSocket;
}

void ProtocolHelper::HandleWatchedValues(std::map<std::string, std::vector<int>> *watchedValues, std::string resourceID, json data, json* config)
{
    if (watchedValues->count(resourceID))
    {
        for (int socket : watchedValues->at(resourceID))
        {
            json message = json::object({{"server", (*config)["ServerIP"]},
                                 {"code", "211"},
                                 {"rsrcId", resourceID},
                                 {"data", data},
                                 {"message", "ressource modifiée par un client"}});

            std::string jsonResponse = message.dump();

            send(socket, jsonResponse.c_str(), jsonResponse.size(), 0);

            std::cout << "\nEnvoie de la mise a jour au client\n";
        }
    }
}
