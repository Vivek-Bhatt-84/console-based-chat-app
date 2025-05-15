#ifndef SERVER_H
#define SERVER_H

#include "database.h"
#include <map>
#include <vector>
#include <string>
#include <thread>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


class Server {
private:
    int server_fd;
    struct sockaddr_in address;
    int opt;
    int addrlen;
    Database db;
    std::map<std::string, int> activeUsers;
    std::map<int, std::string> socketToUser;

    void handleClient(int socket);
    std::vector<std::string> split(const std::string &s, char delimiter);

public:
    Server(int port);
    void start();
};

#endif