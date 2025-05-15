#include "database.h"
#include <iostream>
#include <sstream>

Database::Database() {
    driver = sql::mysql::get_mysql_driver_instance();
    con = driver->connect("tcp://127.0.0.1:3306", "root", "Bhatt@20092003");
    con->setSchema("simple_chat");
}

Database::~Database() {
    delete con;
}

bool Database::registerUser(const std::string &username, const std::string &password) {
    try {
        sql::PreparedStatement *pstmt = con->prepareStatement(
            "INSERT INTO users(username, password) VALUES(?, ?)");
        pstmt->setString(1, username);
        pstmt->setString(2, password);
        pstmt->execute();
        delete pstmt;
        return true;
    } catch (sql::SQLException &e) {
        std::cerr << "SQL Error: " << e.what() << std::endl;
        return false;
    }
}

bool Database::authenticateUser(const std::string &username, const std::string &password) {
    try {
        sql::PreparedStatement *pstmt = con->prepareStatement(
            "SELECT password FROM users WHERE username = ?");
        pstmt->setString(1, username);
        sql::ResultSet *res = pstmt->executeQuery();
        
        bool authenticated = false;
        if (res->next()) {
            std::string dbPassword = res->getString("password");
            authenticated = (dbPassword == password);
        }
        
        delete res;
        delete pstmt;
        return authenticated;
    } catch (sql::SQLException &e) {
        std::cerr << "SQL Error: " << e.what() << std::endl;
        return false;
    }
}

void Database::saveMessage(const std::string &sender, const std::string &receiver, const std::string &message, bool isGroup) {
    try {
        sql::PreparedStatement *pstmt = con->prepareStatement(
            "INSERT INTO messages(sender, receiver, message, is_group) VALUES(?, ?, ?, ?)");
        pstmt->setString(1, sender);
        pstmt->setString(2, receiver);
        pstmt->setString(3, message);
        pstmt->setBoolean(4, isGroup);
        pstmt->execute();
        delete pstmt;
    } catch (sql::SQLException &e) {
        std::cerr << "SQL Error: " << e.what() << std::endl;
    }
}

std::vector<std::string> Database::getChatHistory(const std::string &user1, const std::string &user2, bool isGroup) {
    std::vector<std::string> history;
    try {
        sql::PreparedStatement *pstmt;
        if (isGroup) {
            pstmt = con->prepareStatement(
                "SELECT sender, message, timestamp FROM messages WHERE receiver = ? AND is_group = 1 ORDER BY timestamp");
            pstmt->setString(1, user2);
        } else {
            pstmt = con->prepareStatement(
                "SELECT sender, message, timestamp FROM messages WHERE "
                "((sender = ? AND receiver = ?) OR (sender = ? AND receiver = ?)) "
                "AND is_group = 0 ORDER BY timestamp");
            pstmt->setString(1, user1);
            pstmt->setString(2, user2);
            pstmt->setString(3, user2);
            pstmt->setString(4, user1);
        }

        sql::ResultSet *res = pstmt->executeQuery();
        while (res->next()) {
            std::stringstream ss;
            ss << res->getString("sender") << " [" << res->getString("timestamp") << "]: "
               << res->getString("message");
            history.push_back(ss.str());
        }

        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        std::cerr << "SQL Error: " << e.what() << std::endl;
    }
    return history;
}

bool Database::createGroup(const std::string &groupName) {
    try {
        sql::PreparedStatement *pstmt = con->prepareStatement(
            "INSERT INTO chat_groups(name) VALUES(?)");
        pstmt->setString(1, groupName);
        pstmt->execute();
        delete pstmt;
        return true;
    } catch (sql::SQLException &e) {
        std::cerr << "SQL Error: " << e.what() << std::endl;
        return false;
    }
}

bool Database::addUserToGroup(const std::string &groupName, const std::string &username) {
    try {
        sql::PreparedStatement *pstmt = con->prepareStatement(
            "SELECT id FROM chat_groups WHERE name = ?");
        pstmt->setString(1, groupName);
        sql::ResultSet *res = pstmt->executeQuery();
        
        if (!res->next()) {
            delete res;
            delete pstmt;
            return false;
        }
        
        int groupId = res->getInt("id");
        delete res;
        delete pstmt;
        
        pstmt = con->prepareStatement(
            "INSERT INTO group_members(group_id, username) VALUES(?, ?)");
        pstmt->setInt(1, groupId);
        pstmt->setString(2, username);
        pstmt->execute();
        delete pstmt;
        return true;
    } catch (sql::SQLException &e) {
        std::cerr << "SQL Error: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> Database::getGroupMembers(const std::string &groupName) {
    std::vector<std::string> members;
    try {
        sql::PreparedStatement *pstmt = con->prepareStatement(
            "SELECT gm.username FROM group_members gm "
            "JOIN chat_groups cg ON gm.group_id = cg.id "
            "WHERE cg.name = ?");
        pstmt->setString(1, groupName);
        sql::ResultSet *res = pstmt->executeQuery();
        
        while (res->next()) {
            members.push_back(res->getString("username"));
        }
        
        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        std::cerr << "SQL Error: " << e.what() << std::endl;
    }
    return members;
}

std::vector<std::string> Database::getUserGroups(const std::string &username) {
    std::vector<std::string> groups;
    try {
        sql::PreparedStatement *pstmt = con->prepareStatement(
            "SELECT cg.name FROM chat_groups cg "
            "JOIN group_members gm ON cg.id = gm.group_id "
            "WHERE gm.username = ?");
        pstmt->setString(1, username);
        sql::ResultSet *res = pstmt->executeQuery();
        
        while (res->next()) {
            groups.push_back(res->getString("name"));
        }
        
        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        std::cerr << "SQL Error: " << e.what() << std::endl;
    }
    return groups;
}