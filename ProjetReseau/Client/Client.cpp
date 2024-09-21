#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <arpa/inet.h>
#include <fstream>
#include "nlohmann/json.hpp"
#include <thread>
#include <mutex>
#include <regex>


using json = nlohmann::json;

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)

#define BUFLEN 512

// ANSI escape codes for text colors
#define RESET   "\033[0m"    
#define CYAN    "\033[36m"      

mutex m;
vector<int> clientSockets;

std::regex urlPattern(R"(^(rdo|wrdo)://(?:[0-9]{1,3}\.){3}[0-9]{1,3}:[0-9]{1,5}/[\w-]+$)");

int mainSocketID;


void ExtractFields(string resourceURL, string& protocol, string& serverIP, string& serverPort, string& resourceID);

void ReadJSONFromFile(json& jsonWrite);

void ConnectToServer(string serverIpAddress, int port, int socketID);

void PrintMessage(string message, bool isColored = false);

void WrdoResponseWaitingThread(int clientSocket);

void PrintJSON(string message, json json, bool isColored = false);

void HandleServerExchange(string protocol, json jsonData);


int main()
{
    string operation;
    string resourceURL;

    string protocol;
    string serverIP;
    string serverPort;
    string resourceID;

    while (true)
    {
        do
        {
            PrintMessage("Quelle est l'identifiant de la ressource? (protocole://adresse_ip_serveur:port/id_ressource)\n");
            cin >> resourceURL;
        } while (!std::regex_match(resourceURL, urlPattern));

        ExtractFields(resourceURL, protocol, serverIP, serverPort, resourceID);

        mainSocketID = clientSockets.size();

        ConnectToServer(serverIP, stoi(serverPort), mainSocketID);

        PrintMessage("\nQuelle est l'opération à effectuer ? \nGET = Consulter une ressource\nPOST = Ajouter une ressource\n");
        cin >> operation;

        if (operation == "POST")
        {
            json jsonWrite;

            ReadJSONFromFile(jsonWrite);

            // Ajouter l'ID de la ressource à l'objet JSON
            jsonWrite["rsrcId"] = resourceID;
            jsonWrite["protocol"] = protocol;
            jsonWrite["operation"] = operation;

            PrintJSON("Envoi de l'objet suivant au serveur : ", jsonWrite);

            HandleServerExchange(protocol, jsonWrite);
        }
        else if (operation == "GET")
        {
            string jsonStr;

            json jsonRead = json::object({
                {"rsrcId", resourceID},
                {"protocol", protocol},
                {"operation", operation}
            });

            PrintJSON("Envoi de l'objet suivant au serveur : ", jsonRead);

            HandleServerExchange(protocol, jsonRead);
        }
        else
        {
            PrintMessage("\nErreur: opération inconnue\n");
        }
        m.lock();
        printf("\n");
        m.unlock();

        if (protocol == "rdo")
            close(clientSockets[mainSocketID]);
    }

    for (int socket : clientSockets)
    {
        close(socket);
    }
}

void ExtractFields(string resourceURL, string &protocol, string &serverIP, string &serverPort, string &resourceID)
{
    stringstream ss(resourceURL);

        // Parse the other informations
        getline(ss, protocol, ':');

        // Ignore the //
        ss.ignore(2);

        getline(ss, serverIP, ':');
        getline(ss, serverPort, '/');
        getline(ss, resourceID);
}

void ReadJSONFromFile(json& jsonWrite)
{
    ifstream jsonFileWrite;
    string filename = "";

            do
            {
                if (filename != "")
                {
                    PrintMessage("\nIncapable d'ouvrir le fichier");
                }
                PrintMessage("\nSaisir le nom du fichier contenant les données de la ressource (dans le dossier ressources) : \n");

                cin >> filename;
                jsonFileWrite.open("ressources/" + filename);
            } while (!jsonFileWrite.is_open());

            jsonFileWrite >> jsonWrite["data"];
            jsonFileWrite.close();

            PrintJSON("Données du fichier : ", jsonWrite["data"]);
}

void ConnectToServer(string serverIpAddress, int port, int socketID)
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
        PrintMessage("Échec de la connexion avec le serveur... \n");
        exit(0);
    }
    else
    {
        PrintMessage("Connexion réussie\n");
    }

    clientSockets.push_back(clientSocket);
}

void PrintMessage(string message, bool isColored)
{
    m.lock();
    cout << (isColored ? CYAN + message + RESET : message) << std::endl;
    m.unlock();
}

void WrdoResponseWaitingThread(int clientSocket)
{
    while (true)
    {
        char response[BUFLEN] = {};
        read(clientSocket, response, BUFLEN);

        PrintJSON("Donnée modifiée par un autre client : ", json::parse(response), true);
    }
}

void PrintJSON(string message, json json, bool isColored)
{
    string output = json.dump(8);

    output.insert(output.find_last_not_of(" \n\r\t"), "\t");

    // Afficher la réponse du serveur
    PrintMessage("\n\t" + message + '\n', isColored);
    PrintMessage('\t' + output + '\n', isColored);
}

void HandleServerExchange(string protocol, json jsonData)
{
    json jsonResponse;
    string jsonStr;

    char message[BUFLEN] = {};
    jsonStr = jsonData.dump();
    strncpy(message, jsonStr.c_str(), min(jsonStr.size(), static_cast<size_t>(BUFLEN - 1)));

    write(clientSockets[mainSocketID], message, BUFLEN);

    PrintMessage("\tEnvoi terminé");

    char response[BUFLEN] = {};
    read(clientSockets[mainSocketID], response, BUFLEN);

    // Afficher la réponse du serveur
    jsonResponse = json::parse(response);
    PrintJSON("Réponse du serveur : ", jsonResponse);

    if (protocol == "wrdo")
    {
        PrintMessage("\n\tCréation d'un thread pour recevoir les mises à jour de la donnée\n");
        thread(WrdoResponseWaitingThread, clientSockets[mainSocketID]).detach();
    }
}
