#include "server.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <sstream>



Server::Server(int port) : opt(1), addrlen(sizeof(address)) {
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port << std::endl;
}

void Server::start() {
    while (true) {
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        std::thread clientThread(&Server::handleClient, this, new_socket);
        clientThread.detach();
    }
}

void Server::handleClient(int socket) {
    char buffer[1024] = {0};
    std::string currentUser;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(socket, buffer, 1024);
        if (valread <= 0) {
            if (!currentUser.empty()) {
                activeUsers.erase(currentUser);
                socketToUser.erase(socket);
                std::cout << "User " << currentUser << " disconnected" << std::endl;
            }
            close(socket);
            return;
        }

        std::string message(buffer);
        std::cout << "Received: " << message << std::endl;

        std::vector<std::string> tokens = split(message, '|');
        if (tokens.empty()) continue;

        std::string response;
        if (tokens[0] == "REGISTER") {
            if (tokens.size() >= 3) {
                bool success = db.registerUser(tokens[1], tokens[2]);
                response = success ? "REGISTER_SUCCESS" : "REGISTER_FAILED";
            }
        } else if (tokens[0] == "LOGIN") {
            if (tokens.size() >= 3) {
                bool authenticated = db.authenticateUser(tokens[1], tokens[2]);
                if (authenticated) {
                    currentUser = tokens[1];
                    activeUsers[currentUser] = socket;
                    socketToUser[socket] = currentUser;
                    response = "LOGIN_SUCCESS";
                } else {
                    response = "LOGIN_FAILED";
                }
            }
        } else if (tokens[0] == "SEND_MSG") {
            if (tokens.size() >= 3 && !currentUser.empty()) {
                db.saveMessage(currentUser, tokens[1], tokens[2], false);
                if (activeUsers.count(tokens[1])) {
                    int recipientSocket = activeUsers[tokens[1]];
                    std::string forwardMsg = "NEW_MSG|" + currentUser + "|" + tokens[2];
                    send(recipientSocket, forwardMsg.c_str(), forwardMsg.length(), 0);
                }
                response = "MSG_SENT";
            }
        } else if (tokens[0] == "SEND_GROUP_MSG") {
            if (tokens.size() >= 3 && !currentUser.empty()) {
                db.saveMessage(currentUser, tokens[1], tokens[2], true);
                std::vector<std::string> members = db.getGroupMembers(tokens[1]);
                for (const std::string &member : members) {
                    if (member != currentUser && activeUsers.count(member)) {
                        int recipientSocket = activeUsers[member];
                        std::string forwardMsg = "NEW_GROUP_MSG|" + tokens[1] + "|" + currentUser + "|" + tokens[2];
                        send(recipientSocket, forwardMsg.c_str(), forwardMsg.length(), 0);
                    }
                }
                response = "GROUP_MSG_SENT";
            }
        } else if (tokens[0] == "GET_HISTORY") {
            if (tokens.size() >= 2 && !currentUser.empty()) {
                bool isGroup = (tokens.size() >= 3 && tokens[2] == "GROUP");
                std::vector<std::string> history = db.getChatHistory(currentUser, tokens[1], isGroup);
                for (const std::string &msg : history) {
                    send(socket, msg.c_str(), msg.length(), 0);
                    usleep(10000);
                }
                response = "HISTORY_END";
            }
        } else if (tokens[0] == "CREATE_GROUP") {
            if (tokens.size() >= 2 && !currentUser.empty()) {
                bool success = db.createGroup(tokens[1]);
                if (success) {
                    db.addUserToGroup(tokens[1], currentUser);
                    response = "GROUP_CREATED";
                } else {
                    response = "GROUP_CREATE_FAILED";
                }
            }
        } else if (tokens[0] == "JOIN_GROUP") {
            if (tokens.size() >= 2 && !currentUser.empty()) {
                bool success = db.addUserToGroup(tokens[1], currentUser);
                response = success ? "JOINED_GROUP" : "JOIN_GROUP_FAILED";
            }
        } else if (tokens[0] == "LIST_GROUPS") {
            if (!currentUser.empty()) {
                std::vector<std::string> groups = db.getUserGroups(currentUser);
                for (const std::string &group : groups) {
                    std::string groupMsg = "GROUP|" + group;
                    send(socket, groupMsg.c_str(), groupMsg.length(), 0);
                    usleep(10000);
                }
                response = "GROUPS_LIST_END";
            }
        }

        if (!response.empty()) {
            send(socket, response.c_str(), response.length(), 0);
        }
    }
}

std::vector<std::string> Server::split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}