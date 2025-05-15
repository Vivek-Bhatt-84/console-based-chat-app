#include "client.h"
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <termios.h>
#include <cstring>
#include <cstdlib>

Client::Client() : sock(0) {
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::thread listenerThread(&Client::listenForMessages, this);
    listenerThread.detach();
}

void Client::displayMenu() {
    std::cout << "\n===== Chat Application =====\n";
    if (currentUser.empty()) {
        std::cout << "1. Register\n";
        std::cout << "2. Login\n";
        std::cout << "3. Exit\n";
    } else {
        std::cout << "Logged in as: " << currentUser << "\n";
        std::cout << "1. Send private message\n";
        std::cout << "2. Send group message\n";
        std::cout << "3. View chat history\n";
        std::cout << "4. Create group\n";
        std::cout << "5. Join group\n";
        std::cout << "6. List my groups\n";
        std::cout << "7. Logout\n";
    }
    std::cout << "Enter choice: ";
}

void Client::sendMessage(const std::string &msg) {
    send(sock, msg.c_str(), msg.length(), 0);
}

std::string Client::readResponse() {
    char buffer[1024] = {0};
    read(sock, buffer, 1024);
    return std::string(buffer);
}

void Client::listenForMessages() {
    char buffer[1024] = {0};
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, 1024);
        if (valread <= 0) {
            std::cout << "\nDisconnected from server\n";
            exit(0);
        }

        std::string message(buffer);
        if (message == "HISTORY_END" || message == "GROUPS_LIST_END") {
            continue;
        }

        std::vector<std::string> tokens = split(message, '|');
        if (tokens[0] == "NEW_MSG") {
            std::cout << "\n[Private from " << tokens[1] << "]: " << tokens[2] << "\n";
            displayMenu();
        } else if (tokens[0] == "NEW_GROUP_MSG") {
            std::cout << "\n[Group " << tokens[1] << " from " << tokens[2] << "]: " << tokens[3] << "\n";
            displayMenu();
        } else if (tokens[0] == "GROUP") {
            std::cout << " - " << tokens[1] << "\n";
        } else {
            std::cout << "\n" << message << "\n";
            displayMenu();
        }
    }
}

std::vector<std::string> Client::split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string Client::getPassword() {
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::string password;
    std::getline(std::cin, password);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return password;
}

void Client::handleUnauthenticatedMenu(int choice) {
    std::string username, password;
    switch (choice) {
        case 1: {
            std::cout << "Enter username: ";
            std::getline(std::cin, username);
            std::cout << "Enter password: ";
            password = getPassword();
            sendMessage("REGISTER|" + username + "|" + password);
            std::cout << "\n" << readResponse() << std::endl;
            break;
        }
        case 2: {
            std::cout << "Enter username: ";
            std::getline(std::cin, username);
            std::cout << "Enter password: ";
            password = getPassword();
            sendMessage("LOGIN|" + username + "|" + password);
            std::string response = readResponse();
            if (response == "LOGIN_SUCCESS") {
                currentUser = username;
            }
            std::cout << "\n" << response << std::endl;
            break;
        }
        case 3: {
            exit(0);
        }
        default: {
            std::cout << "Invalid choice" << std::endl;
        }
    }
}

void Client::handleAuthenticatedMenu(int choice) {
    std::string recipient, message, groupName;
    switch (choice) {
        case 1: {
            std::cout << "Enter recipient username: ";
            std::getline(std::cin, recipient);
            std::cout << "Enter message: ";
            std::getline(std::cin, message);
            sendMessage("SEND_MSG|" + recipient + "|" + message);
            std::cout << readResponse() << std::endl;
            break;
        }
        case 2: {
            std::cout << "Enter group name: ";
            std::getline(std::cin, recipient);
            std::cout << "Enter message: ";
            std::getline(std::cin, message);
            sendMessage("SEND_GROUP_MSG|" + recipient + "|" + message);
            std::cout << readResponse() << std::endl;
            break;
        }
        case 3: {
            std::cout << "Enter username/group name: ";
            std::getline(std::cin, recipient);
            std::cout << "Is this a group? (y/n): ";
            std::string isGroup;
            std::getline(std::cin, isGroup);
            sendMessage("GET_HISTORY|" + recipient + "|" + (isGroup == "y" ? "GROUP" : ""));
            break;
        }
        case 4: {
            std::cout << "Enter group name: ";
            std::getline(std::cin, groupName);
            sendMessage("CREATE_GROUP|" + groupName);
            std::cout << readResponse() << std::endl;
            break;
        }
        case 5: {
            std::cout << "Enter group name to join: ";
            std::getline(std::cin, groupName);
            sendMessage("JOIN_GROUP|" + groupName);
            std::cout << readResponse() << std::endl;
            break;
        }
        case 6: {
            sendMessage("LIST_GROUPS");
            break;
        }
        case 7: {
            currentUser.clear();
            break;
        }
        default: {
            std::cout << "Invalid choice" << std::endl;
        }
    }
}

void Client::run() {
    while (true) {
        displayMenu();
        int choice;
        std::cin >> choice;
        std::cin.ignore();

        if (currentUser.empty()) {
            handleUnauthenticatedMenu(choice);
        } else {
            handleAuthenticatedMenu(choice);
        }
    }
}