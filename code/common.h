#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>

#define MAX_STRING 1024

// Messages
#define PORT_SUCCESS "200 PORT command successful."
#define USER_SUCCESS "331 Username OK, need password."
#define USER_FAIL "530 Not logged in."
#define PASS_SUCCESS "230 User logged in, proceed."
#define PASS_FAIL "530 Not logged in."
#define PWD_SUCCESS "257"
#define SERVICE_READY "220 Service ready for new user."
#define SERVICE_QUIT "221 Service closing control connection."
#define TRANSFER_READY "150 File status okay; about to open data connection."
#define TRANSFER_COMPLETE "226 Transfer completed."
#define INVALID_COMMAND "202 Command not implemented."
#define INVALID_RESOURCE "550 No such file or directory."
#define INVALID_SEQUENCE "503 Bad sequence of commands."

// CHANGE PORT NUMBER TO 21 AND 20
#define CONTROL_PORT 2021
#define DATA_PORT 2020

#define LOCALHOST "127.0.0.1"

char* sys_list(char path[]);
int sys_cwd(char path[]);

bool directory_exists(const char path[]);
bool file_exists(const char path[]);
int get_unix_time();

#endif // COMMON_H