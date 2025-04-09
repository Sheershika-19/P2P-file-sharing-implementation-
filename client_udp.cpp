#include <iostream>
#include <thread>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <vector>

using namespace std;

#define SERVER_PORT 4444
#define PEER_SERVER_PORT 5555
#define BUFFER_SIZE 1024

void peer_server() {
    int sockfd;
    struct sockaddr_in peer_addr, client_addr;
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = INADDR_ANY;
    peer_addr.sin_port = htons(PEER_SERVER_PORT);

    if (bind(sockfd, (const struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0) {
        perror("UDP bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    cout << "Peer (UDP) server started on port " << PEER_SERVER_PORT << endl;

    while (true) {
        socklen_t len = sizeof(client_addr);
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                      (struct sockaddr*)&client_addr, &len);
        if (bytes_received < 0) {
            perror("recvfrom failed");
            continue;
        }

        string filename(buffer, bytes_received);
        ifstream file(filename, ios::binary);

        if (!file) {
            cerr << "File not found: " << filename << endl;
            string error_msg = "ERROR: File not found";
            sendto(sockfd, error_msg.c_str(), error_msg.size(), 0,
                   (struct sockaddr*)&client_addr, len);
        } else {
            char file_buffer[BUFFER_SIZE];
            while (file.read(file_buffer, BUFFER_SIZE)) {
                sendto(sockfd, file_buffer, file.gcount(), 0,
                       (struct sockaddr*)&client_addr, len);
            }
            if (file.gcount() > 0) {
                sendto(sockfd, file_buffer, file.gcount(), 0,
                       (struct sockaddr*)&client_addr, len);
            }
            file.close();
        }
    }
    close(sockfd);
}
/*
bool fetch_file_from_peer(const string& peer_ip, const string& filename) {
    int sockfd;
    struct sockaddr_in peer_addr;
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket creation failed");
        return false;
    }

    struct timeval tv;
    tv.tv_sec = 5;  
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(PEER_SERVER_PORT);
    inet_pton(AF_INET, peer_ip.c_str(), &peer_addr.sin_addr);

    sendto(sockfd, filename.c_str(), filename.length(), 0,
           (struct sockaddr*)&peer_addr, sizeof(peer_addr));

    ofstream outfile("received_" + filename, ios::binary);
    if (!outfile) {
        cerr << "Failed to create file: received_" << filename << endl;
        close(sockfd);
        return false;
    }

    socklen_t len = sizeof(peer_addr);
    int bytes_received;

    while ((bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                      (struct sockaddr*)&peer_addr, &len)) > 0) {
        if (strncmp(buffer, "ERROR", 5) == 0) {
            cerr << "Error receiving file from peer: " << peer_ip << endl;
            outfile.close();
            close(sockfd);
            return false;
        }
        outfile.write(buffer, bytes_received);
    }

    cout << "File "+filename+" received successfully from " << peer_ip << endl;
    outfile.close();
    close(sockfd);
    return true;
}*/

bool fetch_file_from_peer(const string& peer_ip, const string& filename) {
    int sockfd;
    struct sockaddr_in peer_addr;
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket creation failed");
        return false;
    }

    struct timeval tv;
    tv.tv_sec = 5;  
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(PEER_SERVER_PORT);
    inet_pton(AF_INET, peer_ip.c_str(), &peer_addr.sin_addr);

    sendto(sockfd, filename.c_str(), filename.length(), 0,
           (struct sockaddr*)&peer_addr, sizeof(peer_addr));

    socklen_t len = sizeof(peer_addr);
    int bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                  (struct sockaddr*)&peer_addr, &len);
    
    if (bytes_received <= 0) {
        cerr << "No response from peer: " << peer_ip << endl;
        close(sockfd);
        return false;
    }

    buffer[bytes_received] = '\0';
    string response(buffer);

    if (response.find("ERROR: File not found") != string::npos) {
        cerr << "Peer " << peer_ip << " does not have the file.\n";
        close(sockfd);
        return false;
    }

    ofstream outfile("received_" + filename, ios::binary);
    if (!outfile) {
        cerr << "Failed to create output file: " << filename << endl;
        close(sockfd);
        return false;
    }

    outfile.write(buffer, bytes_received);

    while ((bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                      (struct sockaddr*)&peer_addr, &len)) > 0) {
        outfile.write(buffer, bytes_received);
    }

    cout << "File " << filename << " received successfully from " << peer_ip << endl;
    outfile.close();
    close(sockfd);
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <server_ip>\n";
        return 1;
    }
    string server_ip = argv[1];

    thread(peer_server).detach();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("TCP socket creation failed");
        return 1;
    }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Server connection failed");
        close(sock);
        return 1;
    }

    cout << "Enter commands: " << endl;
    string command;

    while (true) {
        getline(cin, command);

        if (command.rfind("Register ", 0) == 0) {
            send(sock, command.c_str(), command.length(), 0);
            char response[BUFFER_SIZE];
            int bytes_read = read(sock, response, BUFFER_SIZE - 1);
            response[bytes_read] = '\0';
            cout << response << endl << endl;
        }

        if (command.rfind("Send ", 0) == 0) {
            string file_list = command.substr(5);
            istringstream ss(file_list);
            string filename;

            while (getline(ss, filename, ',')) {
                filename.erase(0, filename.find_first_not_of(" "));
                filename.erase(filename.find_last_not_of(" ") + 1);

                string new_command = "Send " + filename;
                send(sock, new_command.c_str(), new_command.length(), 0);

                char response[BUFFER_SIZE];
                int bytes_read = read(sock, response, BUFFER_SIZE - 1);
                response[bytes_read] = '\0';

                cout << "Peer details for " << response << endl;
                istringstream iss(response);
                string received_filename;
                vector<string> peer_ips;
                iss >> received_filename;

                string peer_ip;
                while (iss >> peer_ip) {
                    peer_ips.push_back(peer_ip);
                }

                bool file_downloaded = false;
                for (const auto& peer : peer_ips) {
                    if (!received_filename.empty() && !peer.empty()) {
                        cout << "Trying peer: " << peer << " for file: " << received_filename << endl;
                        if (fetch_file_from_peer(peer, received_filename)) {
                            file_downloaded = true;
                            break;
                        } else {
                            cout << "Failed to fetch from peer: " << peer << ", trying next...\n";
                        }
                    }
                }

                if (!file_downloaded) {
                    cout << "Failed to download from all available peers.\n";
                }
            }
        }
    }

    close(sock);
    return 0;
}
