#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>

class Client {
private:
    int sock;
    std::string currentUser;

    void displayMenu();
    void sendMessage(const std::string &msg);
    std::string readResponse();
    void listenForMessages();
    std::vector<std::string> split(const std::string &s, char delimiter);
    std::string getPassword();
    void handleUnauthenticatedMenu(int choice);
    void handleAuthenticatedMenu(int choice);

public:
    Client();
    void run();
};

#endif