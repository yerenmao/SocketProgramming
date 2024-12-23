#include "authentication.hpp"
#include <sstream>
#include <functional>

std::unordered_map<std::string, std::string> Authentication::user_data;
std::string Authentication::data_file_path = "user_data.txt";
std::mutex Authentication::auth_mutex;

std::string auth_result_to_string(AuthResult result) {
    switch (result) {
        case AuthResult::Success:
            return "Success";
        case AuthResult::UsernameExists:
            return "Username already exists";
        case AuthResult::UsernameNotFound:
            return "Username not found";
        case AuthResult::WrongPassword:
            return "Wrong password";
        default:
            return "Unknown result";
    }
}

void Authentication::load_user_data() {
    std::ifstream file(data_file_path);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string username, password;
        iss >> username >> password;
        user_data[username] = password;
    }
    file.close();
}

void Authentication::save_user_data() {
    std::ofstream file(data_file_path, std::ios::trunc);
    if (!file.is_open()) return;

    for (const auto& [username, password] : user_data) {
        file << username << " " << password << "\n";
    }
    file.close();
}

std::string Authentication::hash_password(const std::string& password) {
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

AuthResult Authentication::register_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(auth_mutex);

    if (user_data.find(username) != user_data.end()) {
        return AuthResult::UsernameExists; // Username exists
    }

    user_data[username] = hash_password(password);
    save_user_data();
    return AuthResult::Success; // Success
}

AuthResult Authentication::login_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(auth_mutex);

    auto it = user_data.find(username);
    if (it == user_data.end()) {
        return AuthResult::UsernameNotFound; // Username not found
    }

    if (it->second != hash_password(password)) {
        return AuthResult::WrongPassword; // Wrong password
    }

    return AuthResult::Success; // Success
}

AuthResult Authentication::logout_user(const std::string& username) {
    std::lock_guard<std::mutex> lock(auth_mutex);
    // Logic for logout if needed (e.g., maintain online status)
    auto it = user_data.find(username);
    if (it == user_data.end()) {
        return AuthResult::Success;
    }
    return AuthResult::Success;
}
