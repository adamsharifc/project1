// PROJECT 1: FILE TRANSPORT PROTOCOL (SERVER)
// Author: Adam Sharif
// Date: 11-10-2024
#include "server.h"

user users[MAX_CLIENTS];
connection connections[MAX_CLIENTS];
int user_count;
int connection_count;
char BASE_DIRECTORY[MAX_BUFFER];

int main() {
    
    int server_socket;
    int socket_descriptor_count;
    int new_socket;
    int socket_descriptor;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
    char buffer[MAX_BUFFER];
    fd_set readfds;

    // set base directory for server
    getcwd(BASE_DIRECTORY, sizeof(BASE_DIRECTORY));

    // Load users from CSV file
    user_count = load_users("../users.csv", users);

    connection connections[MAX_CLIENTS];
    init_connections(connections, MAX_CLIENTS);

    int connection_instances_count = 0;

    // init server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // Set SO_REUSEADDR option
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(CONTROL_PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error: Cannot bind to socket");
        exit(1);
    }

    // Listen for connections
    if (listen(server_socket, 10) < 0) {
        perror("Error: Cannot listen on socket");
        exit(1);
    }

    printf("Server listening on CONTROL_PORT %d\n", CONTROL_PORT);

    while (true) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to the readfds set
        FD_SET(server_socket, &readfds);
        socket_descriptor_count = server_socket;

        // Add all active client sockets to readfds set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            socket_descriptor = connections[i].socket_descriptor;
            if (socket_descriptor > 0) {
                FD_SET(socket_descriptor, &readfds);
            }
            // set socket_descriptor_count to higest file descriptor number
            if (socket_descriptor > socket_descriptor_count) {
                socket_descriptor_count = socket_descriptor;
            }
        }

        // Wait for activity on one of the sockets
        select(socket_descriptor_count + 1, &readfds, NULL, NULL, NULL);

        // Check if a request was made to the server socket (new connection)
        if (FD_ISSET(server_socket, &readfds)) {
            new_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
            if (new_socket < 0) {
                perror("Error: Failed to accept client connection");
                continue;
            }

            printf("Connection established with user %d\nTheir port: %d\n\n", new_socket, ntohs(client_addr.sin_port));

            // Add new socket to array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++) {                    
                if (connections[i].socket_descriptor == 0){
                    connections[i].socket_descriptor = new_socket;
                    connections[i].port = ntohs(client_addr.sin_port);
                    connections[i].user_number = connection_instances_count + 1;
                    strcpy(connections[i].working_directory, BASE_DIRECTORY);   // set working directory to base directory
                    connection_instances_count++;
                    break;
                }
            }
        }

        // Check all client sockets for incoming data
        for (int i = 0; i < MAX_CLIENTS; i++) {
            socket_descriptor = connections[i].socket_descriptor;

            if (FD_ISSET(socket_descriptor, &readfds)) {
                int valread;
                if ((valread = read(socket_descriptor, buffer, MAX_BUFFER)) == 0) {
                    // Client disconnected
                    getpeername(socket_descriptor, (struct sockaddr*)&client_addr, &addr_size);
                    printf("Host disconnected, ip %s, port %d\n",
                           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    close(socket_descriptor);
                    connections[i].socket_descriptor = 0;   // mark as free slot
                } else {
                    // Echo back the message
                    buffer[valread] = '\0';
                    // printf("%d: %s\n", connections[i].port, buffer);
                    handle_client(socket_descriptor, client_addr, &connections[i], buffer);
                    // send(socket_descriptor, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    return 0;
}

void handle_client(int client_socket, struct sockaddr_in client_addr, connection* conn, char command[]) {
    char buffer[MAX_BUFFER] = {0};  // store messages to be sent
    char* operation;                // the operation to be performed e.g. PWD || CWD || LIST ...
    char* operand;                  // the operand for the operation e.g users || gryffindor.jpg
    int read_size;                  // the size of the buffer
    
    // printf("%d: %s\n", conn->port, command);
    fflush(stdout); // force print to stdout

    // split command into operation and operand
    operation = strtok(command, " ");
    operand = strtok(NULL, " ");

    // BAD SEQUENCES
    // If user has not entered a username, then all commands EXCEPT USER are invalid
    if (conn->username[0] == '\0' && strcmp(operation, "USER") != 0){
        send(client_socket, INVALID_SEQUENCE, strlen(INVALID_SEQUENCE), 0); return;
    }
    // If user has already entered a username, they cannot send USER again
    if (conn->username[0] != '\0' && strcmp(operation, "USER") == 0){
        send(client_socket, INVALID_SEQUENCE, strlen(INVALID_SEQUENCE), 0); return;
    }
    // If user has already logged in, they cannot send PASS again
    if (conn->is_logged_in && strcmp(operation, "PASS") == 0){
        send(client_socket, INVALID_SEQUENCE, strlen(INVALID_SEQUENCE), 0); return;
    }
    // For all operations, other than USER and PASS, user must be logged in, otherwise, send "530 Not logged in."
    if (!conn->is_logged_in && strcmp(operation, "USER") != 0 && strcmp(operation, "PASS") != 0){
        send(client_socket, USER_FAIL, strlen(USER_FAIL), 0); return;
    }

    // BEYOND HERE, IF THE COMMAND ISNT USER OR PASS, THE USER SHOULD BE ABLE TO ACCESS AND WORK WITH THE CORRECT WORKING DIRECTORY
    if (conn->is_logged_in && strcmp(operation, "USER") != 0 && strcmp(operation, "PASS") != 0){
        // check if the server is in the correct working directory to serve the client
        if (is_correct_working_directory(conn->working_directory) == false){
            printf("%s should be in %s\n", conn->username, conn->working_directory); fflush(stdout);
            // change to correct working directory
            if (sys_cwd(conn->working_directory) == 0){     // if working directory is changed successfully, see implementation in common.c
                getcwd(buffer, MAX_BUFFER);     // copy current working directory to buffer
                strcpy(conn->working_directory, buffer);    // copy buffer to connection working directory
            }
            else {
                send(client_socket, INVALID_RESOURCE, strlen(INVALID_RESOURCE), 0);
            }
        }
    }

    // AUTHENTICATED COMMANDS
    if (strcmp(operation, "USER") == 0){            
        if (user_exists(operand, users, user_count)){
            strncpy(conn->username, operand, sizeof(conn->username) - 1);  
            conn->username[sizeof(conn->username) - 1] = '\0'; // Ensure null-termination
            send(client_socket, USER_SUCCESS, strlen(USER_SUCCESS), 0);
            printf("Successful username verification\n"); fflush(stdout);
        } else {
            send(client_socket, USER_FAIL, strlen(USER_FAIL), 0);
        }   
    }   
    else if (strcmp(operation, "PASS") == 0){   
        if (authenticate_user(conn->username, operand, users, user_count)){
            conn->is_logged_in = true;
            send(client_socket, PASS_SUCCESS, strlen(PASS_SUCCESS), 0);
            printf("Successful login\n");   fflush(stdout);
        } else {
            conn->is_logged_in = false;
            send(client_socket, PASS_FAIL, strlen(PASS_FAIL), 0);
        }
    }
    else if (strcmp(operation, "PWD") == 0){
        getcwd(buffer, MAX_BUFFER);     // get current working directory not to be confused with CWD (Change Working Directory)
        send(client_socket, buffer, strlen(buffer), 0);
    }
    else if (strcmp(operation, "CWD") == 0){
        printf("Changing directory to: %s\n", operand);
        if (sys_cwd(operand) == 0){     // if working directory is changed successfully, see implementation in common.c
            getcwd(buffer, MAX_BUFFER);     // copy current working directory to buffer
            strcpy(conn->working_directory, buffer);    // copy buffer to connection working directory
            send(client_socket, buffer, strlen(buffer), 0);
        }
        else {
            send(client_socket, INVALID_RESOURCE, strlen(INVALID_RESOURCE), 0);
        }
    }
    else if (strcmp(operation, "QUIT") == 0){
        send(client_socket, SERVICE_QUIT, strlen(SERVICE_QUIT), 0);
        conn->socket_descriptor = 0;    // mark as free slot
        conn->port = 0;
        conn->child_pid = 0;
        conn->username[0] = '\0';
        conn->user_number = 0;
        conn->is_logged_in = false;
        conn->is_transfering = false;
        strcpy(conn->working_directory, BASE_DIRECTORY);
        close(client_socket);
        printf("Closed!\n");
    }
    else if (strcmp(operation, "PORT") == 0){
        // handle_port_command(client_socket, client_addr, operand);
        serve_port_command(client_socket, client_addr, operand);
    }
    else {
        send(client_socket, INVALID_COMMAND, strlen(INVALID_COMMAND), 0);
    }     
}

int serve_port_command(int client_socket, struct sockaddr_in client_addr, char* operand){
    
    printf("PORT received: %s\n", operand); fflush(stdout);
    // server reads the port requst and extracts the client_plusone port for data transfer
    int client_plusone = unsplit_port(operand);

    // server replies with "200 PORT command succesful"
    send(client_socket, PORT_SUCCESS, strlen(PORT_SUCCESS), 0);

    // server waits for the underlying data transfer command
    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);
    read(client_socket, buffer, MAX_BUFFER);
    // server extracts the underlying operation and operand
    char* underlying_operation = strtok(buffer, " ");
    char* underlying_operand = strtok(NULL, " ");
    

    if (strcmp(underlying_operation, "RETR") == 0 || strcmp(underlying_operation, "STOR") == 0){
        if (!file_exists(underlying_operand) && !directory_exists(underlying_operand)){
            send(client_socket, INVALID_RESOURCE, strlen(INVALID_RESOURCE), 0);
            return -1;
        }
    }

    // server replies with file status okay "150 File status okay; about to open data connection."
    printf("File okay, beginning data connections\n"); fflush(stdout);
    send(client_socket, TRANSFER_READY, strlen(TRANSFER_READY), 0);

    // server forks a child process to handle the data transfer
    pid_t pid = fork();
    if (pid < 0){
        perror("Error: Fork failed");
        return -1;
    }
    else if (pid == 0){     // child process

        // configure client_data_addr
        struct sockaddr_in client_data_addr;
        client_data_addr.sin_family = AF_INET;
        client_data_addr.sin_addr.s_addr = inet_addr(inet_ntoa(client_addr.sin_addr));
        client_data_addr.sin_port = htons(client_plusone);

        // create data socket
        int data_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (data_socket < 0) {
            perror("Error: Failed to create data socket");
            exit(1);
        }

        // Set SO_REUSEADDR option, allows the server to reuse the same port number 
        int opt = 1;
        setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // configure data server address
        struct sockaddr_in server_data_addr;
        server_data_addr.sin_family = AF_INET;
        server_data_addr.sin_addr.s_addr = INADDR_ANY;
        server_data_addr.sin_port = htons(DATA_PORT);

        // bind data socket to port
        if (bind(data_socket, (struct sockaddr*)&server_data_addr, sizeof(server_data_addr)) < 0) {
            perror("Error: Failed to bind data socket");
            exit(1);
        }

        printf("Connecting to Client Transfer Socket...\n");
        // Connect to client N + 1
        int attempt_count = 0;
        while(connect(data_socket, (struct sockaddr*)&client_data_addr, sizeof(client_data_addr)) != 0){
            attempt_count++;
        }
        printf("Connection Successful\n");
        // CONNECTED
        // ABOVE CODE IS SAME FOR LIST RETR AND STOR
        // BELOW CODE IS DIFFERENT FOR LIST RETR AND STOR


        // send list of files
        char buffer[MAX_BUFFER];
        memset(buffer, 0, MAX_BUFFER);
        if (strcmp(underlying_operation, "LIST") == 0){
            printf("Listing directory\n");
            FILE *fp;
            char line[MAX_BUFFER];
            buffer[0] = '\0';

            // Open the command for reading
            fp = popen("ls", "r");
            if (fp == NULL) {
                perror("Error: Failed to open command");
                exit(1);
            }

            // Read the output a line at a time and send it to the client
            while (fgets(line, sizeof(line), fp) != NULL) {
                send(data_socket, line, strlen(line), 0);
            }          

            // Close the file pointer
            pclose(fp);
            printf("%s\n", TRANSFER_COMPLETE);
            exit(0);
        }
        else if (strcmp(underlying_operation, "RETR") == 0){
            // open file as specified in operand and send to client

            FILE *file = fopen(underlying_operand, "rb");   // read in binary mode
            if (file == NULL){
                perror("Error: Cannot open file");
                exit(1);
            }
            
            char buffer[MAX_BUFFER];
            memset(buffer, 0, MAX_BUFFER);
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, MAX_BUFFER, file)) > 0) {
                send(data_socket, buffer, bytes_read, 0);
            }

            fclose(file);
            printf("%s\n", TRANSFER_COMPLETE);
            exit(0);

        }
        else if (strcmp(underlying_operation, "STOR") == 0){
            // receive file from client and save it to server

            srand(time(0));
            int random_number = rand();
            char temp_filename[MAX_BUFFER];
            sprintf(temp_filename, "%d.temp", random_number);
            
            FILE *file = fopen(temp_filename, "wb");   // write in binary mode
            if (file == NULL){
                perror("Error: Cannot open file");
                exit(1);
            }

            ssize_t bytes_received;
            while ((bytes_received = recv(data_socket, buffer, MAX_BUFFER, 0)) > 0) {
                fwrite(buffer, 1, bytes_received, file);
            }

            fclose(file);
            if (rename(temp_filename, underlying_operand) != 0){
                printf("Error: Cannot rename file\n");
                exit(1);
            }
            printf("%s\n", TRANSFER_COMPLETE);
            exit(0);
        }

        send(client_socket, TRANSFER_COMPLETE, strlen(TRANSFER_COMPLETE), 0);
        close(data_socket);
        exit(0);
    }
    else{   // parent process
        // does NOT WAIT for the child process to finish
        // as this will PREVENT the server from accepting new connections
    }

    return 0;
}
int load_users(char filename[], user users[]){
    FILE* file = fopen(filename, "r");
    if (file == NULL){
        perror("Error: Cannot open file");
        return -1;
    }

    char line[MAX_BUFFER];
    int user_count = 0;
    while (fgets(line, MAX_BUFFER, file) != NULL) {
        char* token = strtok(line, ",");
        strcpy(users[user_count].username, token);
        token = strtok(NULL, "\n");
        strcpy(users[user_count].password, token);
        user_count++;
    }

    fclose(file);
    return user_count;
}
bool user_exists(char username[], user users[], int num_users){
    for (int i = 0; i < num_users; i++) {
        if (strcmp(username, users[i].username) == 0) {     // username exists
            return true;
        }
    }
    return false;
}
bool authenticate_user(char username[], char password[], user users[], int num_users){
    for (int i = 0; i < num_users; i++) {
        if (strcmp(username, users[i].username) == 0 && strcmp(password, users[i].password) == 0) {     // username exists and password matches
            return true;
        }
    }
    return false;
}
void capture_ls_output(char *buffer, size_t buffer_size) {
    FILE *fp;
    char line[MAX_BUFFER];

    buffer[0] = '\0';

    // Open the command for reading
    fp = popen("ls", "r");
    if (fp == NULL) {
        snprintf(buffer, buffer_size, "Failed to run command\n");
        return;
    }

    // Read the output a line at a time
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Check if there's enough space in the buffer
        if (strlen(buffer) + strlen(line) < buffer_size - 1) {
            strcat(buffer, line);
        } else {
            // If buffer is full, truncate and indicate it
            strncat(buffer, line, buffer_size - strlen(buffer) - 4);
            strcat(buffer, "...");
            break;
        }
    }

    // Close the file pointer
    pclose(fp);
}
void init_connections(connection connections[], int num_connections){
    for (int i = 0; i < num_connections; i++){
        connections[i].socket_descriptor = 0;
        connections[i].port = 0;
        connections[i].child_pid = 0;
        connections[i].user_number = 0;
        connections[i].is_logged_in = false;
        connections[i].is_transfering = false;
        connections[i].username[0] = '\0';
        strcpy(connections[i].working_directory, BASE_DIRECTORY);
    }
}
int unsplit_port(char operand[]){
    int h1, h2, h3, h4, p1, p2;
    sscanf(operand, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
    return p1 * 256 + p2;
}
bool is_correct_working_directory(char should_be_directory[]){
    char current_directory[MAX_BUFFER];
    getcwd(current_directory, sizeof(current_directory));
    if (strcmp(current_directory, should_be_directory) == 0){
        return true;
    }
    return false;
}  