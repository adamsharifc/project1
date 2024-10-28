#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

#define INIT_PROMPT "Hello!! Please Authenticate to run server commands\n1. type \"USER\" followed by a space and your username\n2. type \"PASS\" followed by a space and your password\n\n\"QUIT\" to close connection at any moment\nOnce Authenticated\n"
#define COMMAND_LIST "this is the list of commands :\n\"STOR\" + space + filename |to send a file to the server\n\"RETR\" + space + filename |to download a file from the server\n\"LIST\" |to list all the files under the current server directory\n\"CWD\" + space + directory |to change the current server directory\n\"PWD\" |to display the current server directory\nAdd \"!\" before the last three commands to apply them locally\n"
#define MAX_TEST_LINES 100


void handle_data_command(int server_control_socket, int client_nplusone, char operation[], char operand[]);
int send_port_command(char host_address[], int tcp_port_address, int server_control_socket);
int send_underlying_command(int server_control_socket, char operation[], char operand[]);
void input(char* prompt, char* input);
bool is_port_available(int port_number);
void handle_test_mode(char *test_commands[], char *file_path);
#endif // CLIENT_H