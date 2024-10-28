#ifndef SERVER_H
#define SERVER_H

#include "common.h"

#define MAX_CLIENTS 10

struct connection {
    int socket_descriptor;
    int port;
    int child_pid;
    int user_number;
    char username[100];
    char working_directory[MAX_BUFFER];
    int connection_instance_count;
    bool is_logged_in;
    bool is_transfering;
} typedef connection;

struct user {
    char username[100];
    char password[100];
} typedef user;


void handle_client(int client_socket, struct sockaddr_in client_addr, connection* conn, char command[]);
int serve_port_command(int client_socket, struct sockaddr_in client_addr, char* operand);
int load_users(char filename[], user users[]);
bool user_exists(char username[], user users[], int num_users);
bool authenticate_user(char username[], char password[], user users[], int num_users);
void capture_ls_output(char *buffer, size_t buffer_size);
void init_connections(connection connections[], int num_connections);
int unsplit_port(char operand[]);
bool is_correct_working_directory(char should_be_directory[]);

#endif // SERVER_H