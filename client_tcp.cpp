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
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Peer server socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PEER_SERVER_PORT);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Peer server bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("Peer server listen failed");
        exit(EXIT_FAILURE);
    }
    cout << "Peer (TCP) server started on port " << PEER_SERVER_PORT << endl;
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Peer server accept failed");
            continue;
        }
        memset(buffer, 0, BUFFER_SIZE);
        read(new_socket, buffer, BUFFER_SIZE);
        string filename(buffer);
        ifstream file(filename, std::ios::binary);
        if (!file) {
            cerr << "File not found: " << filename << endl;
            string error_msg = "ERROR: File not found\n";
            send(new_socket, error_msg.c_str(), error_msg.size(), 0);
        } else {
            char file_buffer[BUFFER_SIZE];
            while (!file.eof()) {
                file.read(file_buffer, BUFFER_SIZE);
                send(new_socket, file_buffer, file.gcount(), 0);
            }
        }
        close(new_socket);
    }
}
 
bool fetch_file_from_peer(const string& peer_ip, const string& filename) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        return false;
    }
    struct sockaddr_in peer_addr{};
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(PEER_SERVER_PORT);
    inet_pton(AF_INET, peer_ip.c_str(), &peer_addr.sin_addr);
    if (connect(sock, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) == -1) {
        perror("Failed to connect to peer");
        close(sock);
        return false;
    }
    send(sock, filename.c_str(), filename.length(), 0);
    cout << "Requesting file: " << filename << " from " << peer_ip << endl;
    char buffer[BUFFER_SIZE];
    int bytes_received = read(sock, buffer, BUFFER_SIZE - 1);    
    if (bytes_received <= 0) {
        cout << "No response from peer: " << peer_ip << endl;
        close(sock);
        return false;
    }   
    buffer[bytes_received] = '\0';
    string response(buffer);
    if (response.find("ERROR: File not found") != string::npos) {
        cout << "Peer " << peer_ip << " does not have the file.\n";
        close(sock);
        return false;
    }
    ofstream outfile("received_" + filename, std::ios::binary);
    if (!outfile) {
        cout << "Failed to create output file: " << filename << endl;
        close(sock);
        return false;
    }
    outfile.write(buffer, bytes_received);
    while ((bytes_received = read(sock, buffer, BUFFER_SIZE)) > 0) {
        outfile.write(buffer, bytes_received);
    }
    cout << "File " << filename << " received successfully from peer " << peer_ip << endl;
    outfile.close();
    cout << "Connection to port 5555 closed\n\n";
    close(sock);
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
    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Server connection failed");
        return 1;
    }
    cout << "Enter commands: "<<endl;
    string command;
    while (true) {
        getline(cin, command);
        if(command.rfind("Register ", 0) == 0){
            send(sock, command.c_str(), command.length(), 0);
            char response[BUFFER_SIZE];
            int bytes_read = read(sock, response, BUFFER_SIZE - 1);
            response[bytes_read] = '\0';
            cout <<response << endl<< endl;
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
               if(response=="NOT FOUND"){
                  cout<< "File "+filename+" not available"<<endl;
               }
               else {   
               cout << "Peer details for "<< response;
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
           continue;  
        }
      } 
    }
    close(sock);
    return 0;
}
