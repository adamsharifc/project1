# Project 1: File Transport Protocol (FTP)
**Author:** Adam Sharif  
**Date:** 11-10-2024

## Tasks

### 1. FTP Server Responsibilities
- [ ] Maintain FTP sessions
- [ ] Provide file access
- [ ] Support concurrent connections
  - [ ] Handle multiple simultaneous requests from different or the same clients
  - [ ] Use `select()` and `fork()` for handling concurrent connections
  - [ ] For just control connection, `select()` is sufficient
  - [ ] Avoid using only `fork()` for data transfer to avoid a performance penalty
- [ ] Follow RFC 959 for FTP protocol implementation

### 2. FTP Overview
- [ ] FTP uses separate connections for data and control.
- [ ] The FTP client connects from a random unprivileged port to the server on port 21 (control channel).
- [ ] For each data transfer initiated by the client:
  - [ ] Client sends the `PORT N+1` command to the server.
  - [ ] Client listens on port `N+1` for the data transfer.
  - [ ] Server connects from port 20 to the client to establish the data channel.
  - [ ] Data transfer begins after the TCP connection is established.
  - [ ] Data transfer ends by closing the data channel.
  - [ ] Repeat the above steps for each transfer using `N+2`, `N+3`, etc., ports, until they can be reused.

### 3. FTP Commands
- [ ] `PORT h1, h2, h3, h4, p1, p2`
  - [ ] Specifies client IP and port for the data channel.
  - [ ] Client sends before issuing `RETR`, `STOR`, or `LIST` commands.
  - [ ] Server replies with `200 PORT command successful` before starting data transfer.
- [ ] `USER username`
  - [ ] Identifies the user attempting to log in.
  - [ ] Server replies with `331 Username OK, need password` if successful.
  - [ ] Server replies with `530 Not logged in` if unsuccessful.
- [ ] `PASS password`
  - [ ] Authenticates the user with the given password.
  - [ ] Server replies with `230 User logged in, proceed` if correct.
  - [ ] Server replies with `530 Not logged in` if incorrect.
  - [ ] Usernames and passwords should be saved in `user.txt`.
- [ ] `STOR filename`
  - [ ] Upload a local file from client to server.
- [ ] `RETR filename`
  - [ ] Download a file from server to client.
- [ ] `LIST`
  - [ ] List all files in the current server directory.
- [ ] `!LIST`
  - [ ] List all files in the current client directory (local execution).
- [ ] `CWD foldername`
  - [ ] Change the current server directory.
  - [ ] Server replies with `200 directory changed to pathname/foldername`.
- [ ] `!CWD foldername`
  - [ ] Change the current client directory (local execution).
- [ ] `PWD`
  - [ ] Display the current server directory.
  - [ ] Server replies with `257 pathname`.
- [ ] `!PWD`
  - [ ] Display the current client directory (local execution).
- [ ] `QUIT`
  - [ ] End the FTP session and close the control TCP connection.
  - [ ] Server replies with `221 Service closing control connection`.

### 4. Notes
- [ ] Commands `RETR`, `STOR`, and `LIST` trigger data transfer between client and server.
- [ ] For data transfer, client creates socket on port `N+1`.
- [ ] Server connects from port 20 to the client's specified port (`N+i`) for transfer.
- [ ] Overwriting existing files is allowed during transfer.
- [ ] FTP will use stream transfer mode, with the sender closing the data connection upon completion.
- [ ] Server sends `226 Transfer completed` after successful data transfer.
- [ ] The mode is inherently unreliable since clients can't tell if the connection closed prematurely.
- [ ] Session resets if the client disconnects.
- [ ] Support for concurrent file transfer for multiple clients.
- [ ] Use temporary files (`rand#.tmp`) for concurrent downloads.
- [ ] Invalid commands should trigger `202 Command not implemented`.
- [ ] Authentication requires both `USER` and `PASS`. Without successful authentication, server replies with `530 Not logged in`.
- [ ] Invalid filenames or non-existent directories trigger `550 No such file or directory`.
- [ ] Invalid command sequences trigger `503 Bad sequence of commands`.
- [ ] Extensive error checking (e.g., handling whitespace or escape characters) is not required.
