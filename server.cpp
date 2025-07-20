#include <iostream>
#include <unordered_map>
#include <vector>
#include <bits/stdc++.h>
#include <thread>
#include <mutex>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

using namespace std;

#define SERVER_PORT 4444
#define BUFFER_SIZE 1024

unordered_map<string, vector<string>> file_db;
mutex db_mutex;

int server_fd = -1;  

void handle_signal(int signum) {
    cout << "\n[INFO] Caught signal " << signum << ", shutting down server..." << endl;
    if (server_fd != -1) {
        close(server_fd);  
        cout << "[INFO] Closed server socket." << endl;
    }
    exit(0);  
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in peer_addr;
    socklen_t addr_len = sizeof(peer_addr);
    getpeername(client_socket, (struct sockaddr*)&peer_addr, &addr_len);
    string peer_ip = inet_ntoa(peer_addr.sin_addr);

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            cout << "[INFO] Client disconnected: " << peer_ip << endl;
            close(client_socket);
            return;
        }

        string request(buffer);
        request.erase(0, request.find_first_not_of(" \t\n\r"));
        request.erase(request.find_last_not_of(" \t\n\r") + 1);

        if (request.substr(0, 9) == "Register ") {
            string files = request.substr(9);
            lock_guard<mutex> lock(db_mutex);
            size_t pos = 0;
            while ((pos = files.find(',')) != string::npos) {
                string filename = files.substr(0, pos);
                auto& vec = file_db[filename];
                if (find(vec.begin(), vec.end(), peer_ip) == vec.end()) {
                    vec.push_back(peer_ip);
                }
                files.erase(0, pos + 1);
            }
            if (!files.empty()) {
                auto& vec = file_db[files];
                if (find(vec.begin(), vec.end(), peer_ip) == vec.end()) {
                    vec.push_back(peer_ip);
                }
            }

            send(client_socket, "Registration successful\n", 23, 0);
        } else if (request.substr(0, 5) == "Send ") {
            string filename = request.substr(5);
            string response;
            {
                lock_guard<mutex> lock(db_mutex);
                auto it = file_db.find(filename);
                response = filename + " ";
                if (it != file_db.end()) {
                    for (const auto& ip : it->second) {
                        response += ip + " ";
                    }
                    response += "\n";
                } else {
                    response = "NOT_FOUND\n";
                }
            }
            send(client_socket, response.c_str(), response.length(), 0);
        } else {
            send(client_socket, "INVALID_COMMAND\n", 16, 0);
        }
    }
}

int main() {
    signal(SIGINT, handle_signal);  

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Server socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    cout << "[INFO] Server listening on port " << SERVER_PORT << endl;

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket == -1) {
            perror("[ERROR] Accept failed");
            continue;
        }
        thread(handle_client, client_socket).detach();
    }

    return 0;  
}
