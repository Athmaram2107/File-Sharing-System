// ===================== server/server.cpp =====================

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <fstream>
#include <dirent.h>
#include <map>
#include <sstream>
#include <pthread.h>
#include "../shared/config.h"

using namespace std;

// Stores registered users in memory
map<string, string> users;

// Mutexes for safe concurrent access
pthread_mutex_t userMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;

// Function declarations
void loadUsers();
void saveUser(const string &username, const string &password);
void handleUpload(int new_socket);
void handleDownload(int new_socket);
void handleListFiles(int new_socket);
void handleDeleteFile(int new_socket);
void *handleClient(void *clientSocket);

int main() {
    loadUsers(); // Load existing user credentials

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Reuse address and port
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    // Bind server socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    cout << "Server is listening on port " << PORT << "..." << endl;

    while (true) {
        // Accept client connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        cout << "\nNew client connected!" << endl;

        // Create a thread for each client
        pthread_t threadId;
        int *clientSocket = new int(new_socket);
        pthread_create(&threadId, NULL, handleClient, (void *)clientSocket);
        pthread_detach(threadId); // Auto clean up thread after execution
    }

    close(server_fd);
    return 0;
}

// ================= Handle Each Client Thread =================

void *handleClient(void *clientSocket) {
    int sock = *(int *)clientSocket;
    delete (int *)clientSocket;
    char buffer[BUFFER_SIZE] = {0};

    while (true) {
        int userChoice = 0;
        int bytesRead = read(sock, &userChoice, sizeof(userChoice));

        if (bytesRead <= 0) {
            perror("Client disconnected or socket error");
            break;
        }

        if (userChoice == 1) { // Register
            char username[BUFFER_SIZE] = {0};
            char password[BUFFER_SIZE] = {0};

            if (read(sock, username, BUFFER_SIZE) <= 0 || read(sock, password, BUFFER_SIZE) <= 0) {
                perror("Socket read failed during registration");
                break;
            }

            pthread_mutex_lock(&userMutex);
            users[string(username)] = string(password);
            saveUser(username, password);
            pthread_mutex_unlock(&userMutex);

            string response = "Registration successful!";
            send(sock, response.c_str(), response.size(), 0);

        } else if (userChoice == 2) { // Login
            char username[BUFFER_SIZE] = {0};
            char password[BUFFER_SIZE] = {0};

            if (read(sock, username, BUFFER_SIZE) <= 0 || read(sock, password, BUFFER_SIZE) <= 0) {
                perror("Socket read failed during login");
                break;
            }

            string user(username);
            string pass(password);
            bool isAuthenticated = false;

            pthread_mutex_lock(&userMutex);
            if (users.find(user) != users.end() && users[user] == pass) {
                isAuthenticated = true;
            }
            pthread_mutex_unlock(&userMutex);

            if (isAuthenticated) {
                string success = "Login successful!";
                send(sock, success.c_str(), success.size(), 0);

                while (true) {
                    int fileChoice = 0;
                    int bytes = read(sock, &fileChoice, sizeof(fileChoice));
                    if (bytes <= 0) break;

                    if (fileChoice == 1) handleUpload(sock);
                    else if (fileChoice == 2) handleDownload(sock);
                    else if (fileChoice == 3) handleListFiles(sock);
                    else if (fileChoice == 4) handleDeleteFile(sock);
                    else if (fileChoice == 5) break;
                }
            } else {
                string fail = "Login failed!";
                send(sock, fail.c_str(), fail.size(), 0);
            }
        } else {
            break;
        }
    }

    close(sock);
    pthread_exit(NULL);
}

// ================= File and User Handlers =================

void loadUsers() {
    ifstream inFile("server/users.txt");
    string line;
    while (getline(inFile, line)) {
        stringstream ss(line);
        string username, password;
        ss >> username >> password;
        users[username] = password;
    }
    inFile.close();
}

void saveUser(const string &username, const string &password) {
    ofstream outFile("server/users.txt", ios::app);
    outFile << username << " " << password << endl;
    outFile.close();
}

void handleUpload(int sock) {
    char buffer[BUFFER_SIZE] = {0};
    if (read(sock, buffer, BUFFER_SIZE) <= 0) return;

    string fileName = "files/" + string(buffer);
    if (fileName.find("../") != string::npos) {
        string response = "Error: Invalid file path.";
        send(sock, response.c_str(), response.size(), 0);
        return;
    }

    ifstream checkFile(fileName);
    if (checkFile) {
        string response = "Error: File already exists.";
        send(sock, response.c_str(), response.size(), 0);
        checkFile.close();
        return;
    }

    pthread_mutex_lock(&fileMutex);
    ofstream outFile(fileName, ios::binary);
    if (!outFile) {
        string response = "Error: Unable to create file.";
        send(sock, response.c_str(), response.size(), 0);
        pthread_mutex_unlock(&fileMutex);
        return;
    }

    int bytesRead;
    while ((bytesRead = read(sock, buffer, BUFFER_SIZE)) > 0) {
        outFile.write(buffer, bytesRead);
        if (bytesRead < BUFFER_SIZE) break;
    }

    outFile.close();
    pthread_mutex_unlock(&fileMutex);

    string response = "File uploaded successfully!";
    send(sock, response.c_str(), response.size(), 0);
}

void handleDownload(int sock) {
    char buffer[BUFFER_SIZE] = {0};
    if (read(sock, buffer, BUFFER_SIZE) <= 0) return;

    string fileName = "files/" + string(buffer);
    if (fileName.find("../") != string::npos) {
        string response = "Error: Invalid file path.";
        send(sock, response.c_str(), response.size(), 0);
        return;
    }

    pthread_mutex_lock(&fileMutex);
    ifstream inFile(fileName, ios::binary);
    if (!inFile) {
        string response = "Error: File not found.";
        send(sock, response.c_str(), response.size(), 0);
        pthread_mutex_unlock(&fileMutex);
        return;
    }

    while (!inFile.eof()) {
        inFile.read(buffer, BUFFER_SIZE);
        int bytesRead = inFile.gcount();
        send(sock, buffer, bytesRead, 0);
    }

    inFile.close();
    pthread_mutex_unlock(&fileMutex);

    string completeMsg = "\nFile download completed!";
    send(sock, completeMsg.c_str(), completeMsg.size(), 0);
}

void handleListFiles(int sock) {
    DIR *dir;
    struct dirent *ent;
    string fileList = "";

    if ((dir = opendir("files")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                fileList += string(ent->d_name) + "\n";
            }
        }
        closedir(dir);
    } else {
        fileList = "Error opening directory.\n";
    }

    send(sock, fileList.c_str(), fileList.size(), 0);
}

void handleDeleteFile(int sock) {
    char buffer[BUFFER_SIZE] = {0};
    if (read(sock, buffer, BUFFER_SIZE) <= 0) return;

    string fileName = "files/" + string(buffer);
    if (fileName.find("../") != string::npos) {
        string response = "Error: Invalid file path.";
        send(sock, response.c_str(), response.size(), 0);
        return;
    }

    pthread_mutex_lock(&fileMutex);
    int status = remove(fileName.c_str());
    pthread_mutex_unlock(&fileMutex);

    string response = (status == 0) ? "File deleted successfully." : "Error deleting file.";
    send(sock, response.c_str(), response.size(), 0);
}
