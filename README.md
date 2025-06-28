# 📁 File Sharing System (C++ Client-Server)

A multithreaded file sharing system written in C++ using TCP sockets and POSIX threads. It allows multiple clients to register/login and perform file operations (upload, download, list, delete) on the server concurrently.

---

## 🚀 Features

- ✅ User Registration & Login (credentials stored in `server/users.txt`)
- ✅ Upload files from client to server
- ✅ Download files from server to client
- ✅ List all files available on the server
- ✅ Delete files from server
- ✅ Multithreaded: handles multiple clients using `pthreads`
- ✅ Thread safety using `pthread_mutex` locks
- ✅ Error handling for invalid paths, duplicate files, missing files
- ✅ Clean folder structure and modular design

---

## 🛠️ Technologies Used

- **C++**
- **POSIX Sockets** (TCP)
- **Multithreading (pthreads)**
- **File I/O**
- **Linux system calls**
- **Mutex Locks for synchronization**

---

## 📁 Folder Structure

FILE-SHARING-SYSTEM/
├── client/ # client-side source code
│ └── client.cpp
├── server/ # server-side source code
│ ├── server.cpp
│ └── users.txt # stores registered users
├── shared/ # shared configuration
│ └── config.h # PORT and BUFFER_SIZE
├── downloads/ # ⛔ client downloads (runtime only)
├── files/ # ⛔ server file storage (runtime only)
├── client_app # ⛔ compiled client binary
├── server_app # ⛔ compiled server binary
├── .gitignore # files/folders excluded from Git
└── README.md # you're reading it :


---

## ⚙️ Compilation & Execution

## step 1: Compile

```bash
g++ server/server.cpp -o server_app 
g++ client/client.cpp -o client_app

## Step 2: Run server and client
./server_app
./client_app

🧪 Testing Scenarios:
Register new users
Handle login success/failure
Upload existing and new files
Download valid and invalid filenames
Simulate multiple clients uploading simultaneously
Try deleting a non-existent file
Check that user credentials are preserved in users.txt
Ensure thread safety using mutex locks


## Client UI:
1. Register
2. Login
3. Exit
Enter choice: 2
Enter username: testuser
Enter password: 1234
Login successful!

1. Upload File
2. Download File
3. List Files
4. Delete File
5. Logout
Enter choice: 1
Enter file path to send: /home/user/test.pdf
Server Response: File uploaded successfully!


