#ifndef SERVER_H
#define SERVER_H

#include "common.h"

#define MAX_BUFFER 1024
#define MAX_CLIENTS 10

struct connection {
    int socket_descriptor;
    int port;
    int child_pid;
    int user_number;
    char username[100];
    bool is_logged_in;
    bool is_transfering;
    char current_directory[MAX_BUFFER]; //Added changes:  New field for each client's working directory
} typedef connection;

struct user {
    char username[100];
    char password[100];
} typedef user;


void handle_client(int client_socket, struct sockaddr_in client_addr, connection* conn, char command[]);
int load_users(char filename[], user users[]);
bool user_exists(char username[], user users[], int num_users);
bool authenticate_user(char username[], char password[], user users[], int num_users);
void capture_ls_output(char *buffer, size_t buffer_size);
void init_connections(connection connections[], int num_connections);
void print_connections(connection connections[], int num_connections);
int unsplit_port(char operand[]);
int serve_port_command(int client_socket, struct sockaddr_in client_addr, connection* conn, char* operand);

#endif // SERVER_H