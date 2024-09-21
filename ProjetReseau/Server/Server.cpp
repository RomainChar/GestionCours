#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <regex>
#include "nlohmann/json.hpp"
#include "Protocols/Rdo.h"
#include "Protocols/Wrdo.h"

using namespace std;
using json = nlohmann::json;

#define BUFLEN 512
const int MAX_CLIENTS = 10;

std::regex ipAddressPattern(R"(^(?:[0-9]{1,3}\.){3}[0-9]{1,3})");

map<string, json> values;

map<string, vector<int>> watchedValues;

json config;

Rdo *rdo = new Rdo(values, watchedValues, &config);
Wrdo *wrdo = new Wrdo(values, watchedValues, &config);

void AskServerInfo(json &config);

void SetupServerSocket(int &serverSocket, int &clientSocket);

void HandleClient(int clientSocket);

json HandleRequest(char message[], int clientSocket);

int main()
{
    AskServerInfo(config);

    int serverSocket, clientSocket;

    SetupServerSocket(serverSocket, clientSocket);

    while (true)
    {
        if ((clientSocket = accept(serverSocket, NULL, NULL)) == -1)
        {
            perror("\naccept failed with error\n");
            close(serverSocket);
            exit(EXIT_FAILURE);
        }
        printf("\nClient connecté\n");

        std::thread(HandleClient, clientSocket).detach();
    }

    printf("\nFermeture du serveur");
    close(serverSocket);
    return 0;
}

void HandleClient(int clientSocket)
{
    while (true)
    {
        char message[BUFLEN] = {};
        char key[BUFLEN] = {};
        char value[BUFLEN] = {};

        printf("\nEn attente des données...\n");
        fflush(stdout);

        // try to receive some data, this is a blocking call
        int message_len = recv(clientSocket, message, BUFLEN, 0);
        if (message_len <= 0)
        {
            // Handle connection closed or error
            perror("Connexion terminée");
            close(clientSocket);
            return;
        }

        std::string jsonResponse = HandleRequest(message, clientSocket).dump();

        printf("\nEnvoi de la réponse au client...\n");
        send(clientSocket, jsonResponse.c_str(), jsonResponse.size(), 0);
    }
}

json HandleRequest(char message[], int clientSocket)
{
    json response;

    json receivedData;

    try
    {
        // Parse the received JSON
        receivedData = json::parse(message);

        printf("\nDonnées reçues: \n");
        std::cout << receivedData.dump(4) << std::endl;

        // Check if the received JSON contains necessary fields
        if (!receivedData.contains("protocol") || !receivedData.contains("operation") || !receivedData.contains("rsrcId"))
        {

            printf("\nChamps requis manquants\n");
            // If the JSON is missing required fields, send an error response
            response = json::object({{"server", config["ServerIP"]},
                                     {"code", "400"},
                                     {"rsrcId", receivedData["rsrcId"]},
                                     {"message", "champs requis manquants"}});

            printf("\nRéponse générée: \n");
            std::cout << response.dump(4) << std::endl;
        }
        else
        {
            // Extract necessary fields
            std::string protocol = receivedData["protocol"];

            if (protocol == "rdo")
                response = rdo->HandleRequest(receivedData, clientSocket, wrdo);

            else if (protocol == "wrdo")
                response = wrdo->HandleRequest(receivedData, clientSocket, rdo);

            else
            {
                printf("\nProtocole inconnu : %s\n", protocol.c_str());

                response = json::object({{"server", config["ServerIP"]},
                                         {"code", "400"},
                                         {"rsrcId", receivedData["rsrcId"]},
                                         {"message", "protocole inconnu"}});
            }

            printf("\nRéponse générée: \n");
            std::cout << response.dump(4) << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        printf("\nErreur d'analyse du json\n");
        // If there's an error parsing the JSON, send an error response
        response = json::object({{"server", config["ServerIP"]},
                                 {"code", "400"},
                                 {"rsrcId", receivedData["rsrcId"]},
                                 {"message", "requête erronée"}});

        printf("\nRéponse générée : \n");
        std::cout << response.dump(4) << std::endl;
    }

    return response;
}

void AskServerInfo(json &config)
{
    std::string serverIP = "";
    int port = 0;

    do
    {
        cout << "Quelle est l'adresse ip du serveur ?" << endl;
        cin >> serverIP;
    } while (!std::regex_match(serverIP, ipAddressPattern));

    config["ServerIP"] = serverIP;

    while (port > 65535 || port < 1024)
    {
        cout << "Quel port utiliser ?" << endl;
        cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        cin >> port;
    }
    config["Port"] = port;
}

void SetupServerSocket(int &serverSocket, int &clientSocket)
{
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket failed with error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(config["Port"]);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Error binding socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, MAX_CLIENTS) == -1)
    {
        perror("Error listening for connections");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("\nLe serveur %s écoute sur le port %d\n", config["ServerIP"].get<std::string>().c_str(), config["Port"].get<int>());
}