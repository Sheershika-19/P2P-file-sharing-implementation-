#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

#define SERVER_PORT 4444
#define BUFFER_SIZE 1024

unordered_map<string,vector<string>> file_db;
mutex db_mutex;

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
            cout << "Client disconnected: " << peer_ip << "\n";
            close(client_socket);
            return;
        }
        string request(buffer);
        request.erase(request.find_last_not_of(" \t\n\r") + 1); 
        if (request.substr(0, 9) == "Register ") {
            string files = request.substr(9);
            lock_guard<mutex> lock(db_mutex);
            size_t pos = 0;
            while ((pos = files.find(',')) != string::npos) {
                string filename = files.substr(0, pos);
                file_db[filename].push_back(peer_ip);
                files.erase(0, pos + 1);
            }
            if (!files.empty()) {
                file_db[files].push_back(peer_ip);
            }

            send(client_socket, "Registration successful\n", 23, 0);
        } else if (request.substr(0, 5) == "Send ") {
            string filename = request.substr(5);
            string response;

            lock_guard<mutex> lock(db_mutex);
            response = filename + " ";
            if (file_db.find(filename) != file_db.end()) {
                for (const auto& ip : file_db[filename]) {
                    response += ip + " ";
                }
                response += "\n";
            } else {
                response = "NOT_FOUND\n";
            }
            send(client_socket, response.c_str(), response.length(), 0);
        } else {
            send(client_socket, "INVALID_COMMAND\n", 16, 0);
        }
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
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
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    cout << "Server listening on port no." << SERVER_PORT << endl;
    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket == -1) {
            perror("Accept failed");
            continue;
        }
        thread(handle_client, client_socket).detach();
    }
    close(server_fd);
    return 0;
}
