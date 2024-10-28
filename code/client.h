#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

#define INIT_PROMPT "Hello!! Please Authenticate to run server commands\n1. type \"USER\" followed by a space and your username\n2. type \"PASS\" followed by a space and your password\n\n\"QUIT\" to close connection at any moment\nOnce Authenticated\n"
#define COMMAND_LIST "this is the list of commands :\n\"STOR\" + space + filename |to send a file to the server\n\"RETR\" + space + filename |to download a file from the server\n\"LIST\" |to list all the files under the current server directory\n\"CWD\" + space + directory |to change the current server directory\n\"PWD\" |to display the current server directory\nAdd \"!\" before the last three commands to apply them locally\n"

#define MAX_BUFFER 100

int init(); 
void input(char* prompt, char* input);
int send_port_command(char host_address[], int tcp_port_address, int control_socket);
void request_port_command(int control_socket, int control_port, int client_plusone, char operation[], char operand[]);
int send_underlying_command(int control_socket, char operation[], char operand[]);
#endif // CLIENT_H