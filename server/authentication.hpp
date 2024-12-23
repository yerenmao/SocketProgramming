#ifndef AUTHENTICATION_HPP
#define AUTHENTICATION_HPP

#include <string>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include <iostream>

enum class AuthResult {
    Success = 0,
    UsernameExists = -1,
    UsernameNotFound = -2,
    WrongPassword = -3
};

std::string auth_result_to_string(AuthResult result);

class Authentication {
public:
    // Result Codes: 0 -> Success, -1 -> Username exists, -2 -> Username not found, -3 -> Wrong password
    static AuthResult register_user(const std::string& username, const std::string& password);
    static AuthResult login_user(const std::string& username, const std::string& password);
    static AuthResult logout_user(const std::string& username);
    static void load_user_data();

private:
    static std::unordered_map<std::string, std::string> user_data; // username -> hashed password
    static std::string data_file_path;
    static std::mutex auth_mutex;

    static void save_user_data();
    static std::string hash_password(const std::string& password);
};

#endif // AUTHENTICATION_HPP