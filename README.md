# P2P File Sharing System

This project is a **Peer-to-Peer (P2P) File Sharing System** implemented in **C++** using **Socket Programming** and **Multithreading**. It consists of a **server** and multiple **client** programs, enabling clients to register files and request files from other peers.

---

## 🚀 Features

- **Client Registration**:  
  Clients can register files they possess using the `Register` command.  
  Example:
     Register file1.txt,file2.txt
- **File Requesting**:  
  Clients can request one or more files from the network using the `Send` command.  
  Example:
     Send file1.txt
          or
     Send file1.txt,file2.txt,file3.txt ( for requesting multiple files)
- **Peer Discovery**:  
Upon a file request, the server returns a list of peers that have the file(s).

- **File Download**:  
The client attempts to download the file by connecting to peers one by one from the list until the file is received.

- **File Naming**:  
Received files are saved with a `received_` prefix.  
For example:
   file1.txt -> received_file1.txt

## 🧪 How to run

1. **Start the Server**:  
   Compile and run `server.cpp` using below commands. It will continuously listen for client connections.

   g++ server.cpp -o server -pthread
   ./server

2. **Start a Client**:  
   Compile and run `client.cpp` using below commands. Use the `Register` command to register your files.
   
   g++ client_tcp.cpp -o client -pthread
   ./client

3. **Request Files**:  
   Use the `Send` command to request files. The client will:
   - Ask the server for peers with the requested file(s)
   - Try connecting to each peer in order
   - Download the file from the first responsive peer

4. **File Save Format**:  
   Each successfully downloaded file is saved with the format: received_<original_filename>

   
      
