#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <fstream>
#include "../shared/config.h"

using namespace std;

// Function declarations for file operations
void sendFile(int sock);
void receiveFile(int sock);
void listFiles(int sock);
void deleteFile(int sock);

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Socket creation error" << endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        cout << "Invalid address / Address not supported" << endl;
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Connection Failed" << endl;
        return -1;
    }

    // Main menu loop
    while (true) {
        cout << "\n1. Register\n2. Login\n3. Exit\nEnter choice: ";
        int userChoice;
        cin >> userChoice;

        send(sock, &userChoice, sizeof(userChoice), 0);

        if (userChoice == 1) { // Registration
            string username, password;
            cout << "Enter username: ";
            cin >> username;
            cout << "Enter password: ";
            cin >> password;

            send(sock, username.c_str(), username.size(), 0);
            usleep(500000); // Avoid overlap
            send(sock, password.c_str(), password.size(), 0);

            int bytesRead = read(sock, buffer, BUFFER_SIZE);
            buffer[bytesRead] = '\0';
            cout << buffer << endl;

            cout << "Please login now.\n";

        } else if (userChoice == 2) { // Login
            bool loginSuccess = false;

            while (!loginSuccess) {
                string username, password;
                cout << "Enter username: ";
                cin >> username;
                cout << "Enter password: ";
                cin >> password;

                send(sock, username.c_str(), username.size(), 0);
                usleep(500000);
                send(sock, password.c_str(), password.size(), 0);

                int bytesRead = read(sock, buffer, BUFFER_SIZE);
                buffer[bytesRead] = '\0';
                cout << buffer << endl;

                if (strcmp(buffer, "Login successful!") == 0) {
                    loginSuccess = true;

                    // File operations menu after successful login
                    while (true) {
                        cout << "\n1. Upload File\n2. Download File\n3. List Files\n4. Delete File\n5. Logout\nEnter choice: ";
                        int fileChoice;
                        cin >> fileChoice;

                        send(sock, &fileChoice, sizeof(fileChoice), 0);

                        if (fileChoice == 1) sendFile(sock);
                        else if (fileChoice == 2) receiveFile(sock);
                        else if (fileChoice == 3) listFiles(sock);
                        else if (fileChoice == 4) deleteFile(sock);
                        else if (fileChoice == 5) {
                            cout << "Logging out..." << endl;
                            break;
                        } else {
                            cout << "Invalid choice." << endl;
                        }
                    }
                } else {
                    cout << "Login failed! Try again.\n";
                }
            }

        } else if (userChoice == 3) {
            cout << "Exiting..." << endl;
            break;
        } else {
            cout << "Invalid option. Try again." << endl;
        }
    }

    close(sock); // Close connection
    return 0;
}

// ================= File Operations =================

// Upload file to server
void sendFile(int sock) {
    string filePath;
    cout << "Enter file path to send: ";
    cin >> filePath;

    ifstream inFile(filePath, ios::binary);
    if (!inFile) {
        cerr << "Error opening file!" << endl;
        return;
    }

    // Extract filename from path
    size_t pos = filePath.find_last_of('/');
    string fileName = (pos == string::npos) ? filePath : filePath.substr(pos + 1);

    send(sock, fileName.c_str(), fileName.size(), 0);
    usleep(500000); // Give server time to prepare

    char fileBuffer[BUFFER_SIZE];
    while (!inFile.eof()) {
        inFile.read(fileBuffer, BUFFER_SIZE);
        int bytesRead = inFile.gcount();
        send(sock, fileBuffer, bytesRead, 0);
    }

    inFile.close();

    // Receive confirmation from server
    char buffer[BUFFER_SIZE] = {0};
    int bytesRead = read(sock, buffer, BUFFER_SIZE);
    buffer[bytesRead] = '\0';
    cout << "Server Response: " << buffer << endl;
}

// Download file from server
void receiveFile(int sock) {
    string fileName;
    cout << "Enter file name to download: ";
    cin >> fileName;

    send(sock, fileName.c_str(), fileName.size(), 0);

    char buffer[BUFFER_SIZE] = {0};
    ofstream outFile("downloads/" + fileName, ios::binary);
    if (!outFile) {
        cerr << "Error creating file!" << endl;
        return;
    }

    int bytesReceived;
    bool isError = false;
    bool isFileDone = false;

    // Receive file data
    while (!isFileDone && (bytesReceived = read(sock, buffer, BUFFER_SIZE)) > 0) {
        if ((strncmp(buffer, "File download completed!", 24) == 0) ||
            (strncmp(buffer, "Error:", 6) == 0)) {

            buffer[bytesReceived] = '\0';
            cout << "Server Response: " << buffer << endl;

            if (strncmp(buffer, "Error:", 6) == 0)
                isError = true;

            isFileDone = true;
            break;
        }

        outFile.write(buffer, bytesReceived);
        if (bytesReceived < BUFFER_SIZE) break;
    }

    outFile.close();

    if (!isError && !isFileDone) {
        bytesReceived = read(sock, buffer, BUFFER_SIZE);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            cout << "Server Response: " << buffer << endl;
        } else {
            cout << "File downloaded, but no final confirmation received." << endl;
        }
    }
}

// Request list of files from server
void listFiles(int sock) {
    char buffer[BUFFER_SIZE] = {0};
    int bytesRead = read(sock, buffer, BUFFER_SIZE);
    buffer[bytesRead] = '\0';

    cout << "\nFiles on server:\n" << buffer << endl;
}

// Delete a file from server
void deleteFile(int sock) {
    string fileName;
    cout << "Enter file name to delete: ";
    cin >> fileName;

    send(sock, fileName.c_str(), fileName.size(), 0);

    char buffer[BUFFER_SIZE] = {0};
    int bytesRead = read(sock, buffer, BUFFER_SIZE);
    buffer[bytesRead] = '\0';

    cout << "Server Response: " << buffer << endl;
}
