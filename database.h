#ifndef DATABASE_H
#define DATABASE_H

#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <vector>
#include <string>


class Database {
private:
    sql::mysql::MySQL_Driver *driver;
    sql::Connection *con;

public:
    Database();
    ~Database();
    bool registerUser(const std::string &username, const std::string &password);
    bool authenticateUser(const std::string &username, const std::string &password);
    void saveMessage(const std::string &sender, const std::string &receiver, const std::string &message, bool isGroup);
    std::vector<std::string> getChatHistory(const std::string &user1, const std::string &user2, bool isGroup);
    bool createGroup(const std::string &groupName);
    bool addUserToGroup(const std::string &groupName, const std::string &username);
    std::vector<std::string> getGroupMembers(const std::string &groupName);
    std::vector<std::string> getUserGroups(const std::string &username);
};

#endif