#include "client.h"

int main() {

    int control_socket = 0;
    int data_socket = 0;
    int control_port = 0;
    int data_port_count = 1;
    struct sockaddr_in control_client_address;
    struct sockaddr_in data_client_address;
    struct sockaddr_in control_serv_address;
    struct sockaddr_in data_serv_address; 
    char buffer[MAX_BUFFER] = {0};
    char command[MAX_STRING];
    char working_dir[MAX_STRING];
    char* operation;
    char* operand;
    socklen_t addr_len = sizeof(control_client_address);

    // Create socket
    if ((control_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error: Failed to create socket\n");
        return -1;
    }

    // Set up client address
    control_client_address.sin_family = AF_INET;
    control_client_address.sin_addr.s_addr = INADDR_ANY; // Bind to any available interface
    control_client_address.sin_port = htons(0); // Bind to any available port

    // Bind the socket to the client address
    if (bind(control_socket, (struct sockaddr *)&control_client_address, sizeof(control_client_address)) < 0) {
        printf("Error: Failed to bind socket\n");
        return -1;
    }

    // Retrieve the assigned port
    if (getsockname(control_socket, (struct sockaddr *)&control_client_address, &addr_len) < 0) {
        printf("Error: Failed to get socket name\n");
        return -1;
    }
    control_port = ntohs(control_client_address.sin_port);
    printf("Client bound to port %d\n", control_port);

    control_serv_address.sin_family = AF_INET;
    control_serv_address.sin_port = htons(CONTROL_PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, LOCALHOST, &control_serv_address.sin_addr) <= 0) {
        printf("Error: Invalid IP address \n");
        return -1;
    }

    // Connect to server
    if (connect(control_socket, (struct sockaddr *)&control_serv_address, sizeof(control_serv_address)) < 0) {
        printf("Error: Failed to connect to server\n");
        return -1;
    }

    // CONNECTED
    // printf("%s%s\n%s\n", INIT_PROMPT, COMMAND_LIST, SERVICE_READY);
    printf("%s\n", SERVICE_READY);

    while(true){
        
        input("", command);
        if (strlen(command) == 0){continue;}

        strncpy(buffer, command, MAX_BUFFER);
        operation = strtok(buffer, " ");
        operand = strtok(NULL, " ");

        if (operation[0] == '!'){  // LOCAL COMMAND
            if (strcmp(operation, "!PWD") == 0){      // PRINT WORKING DIRECTORY
                getcwd(working_dir, MAX_STRING);
                printf("%s\n", working_dir);            
            }
            else if (strcmp(operation, "!CWD") == 0){ // CHANGE WORKING DIRECTORY
                if (sys_cwd(operand) == 0){
                    getcwd(working_dir, MAX_STRING);
                    printf("Directory changed to %s\n", working_dir);
                }
            }
            else if (strcmp(operation, "!LIST") == 0){    // LIST WORKING DIRECTORY CONTENTS
                system("ls");   
            }
            else{
                printf("%s\n", INVALID_COMMAND);
            }
        }
        else if ((strcmp(operation, "USER") == 0) ||  // CONTROL COMMANDS (DONT REQUIRE DATA SOCKET)
                 (strcmp(operation, "PASS") == 0) || 
                 (strcmp(operation, "PWD") == 0) || 
                 (strcmp(operation, "CWD") == 0) || 
                 (strcmp(operation, "QUIT") == 0)){
            
            send(control_socket, command, strlen(command), 0);
            read(control_socket, buffer, MAX_BUFFER);
            printf("%s\n", buffer);
            
            // Terminate after server replies with SERVICE_QUIT message
            if (strcmp(buffer, SERVICE_QUIT) == 0){
                break;
            }
            memset(buffer, 0, MAX_BUFFER);
        }
        else if ((strcmp(operation, "LIST") == 0) ||  // DATA COMMANDS (REQUIRE DATA SOCKET)
                 (strcmp(operation, "RETR") == 0) ||
                 (strcmp(operation, "STOR") == 0)){  
            
            int client_plusone = control_port + data_port_count;
            data_port_count++;
            request_port_command(control_socket, control_port, client_plusone, operation, operand);
        }
    }

    return 0;
} 

void request_port_command(int control_socket, int control_port, int client_plusone, char operation[], char operand[]){
    // Send PORT command to server
    int port_command_result = send_port_command(LOCALHOST, client_plusone, control_socket);
    if (port_command_result != 0){
        printf("Error: Failed to send PORT command\n");
        return;
    }
    // At this point the Server replies with: "200 PORT command successful."
    // Send underlying command to server
    int underlying_command_result = send_underlying_command(control_socket, operation, operand);
    if (underlying_command_result != 0){
        printf("Error: Failed to send underlying command\n");
        return;
    }
    // At this point the Server replies with "150 File status okay; about to open data connection."

    char buffer[MAX_BUFFER];
    int client_data_socket;
    struct sockaddr_in data_client_address;
    struct sockaddr_in data_serv_address;

    pid_t pid = fork();
    if (pid < 0) {
        perror("Error: Failed to fork");
        exit(1);
    } else if (pid == 0) { // Child process
        
        // Create client data socket
        if ((client_data_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("Error: Failed to create socket\n");
            exit(1);
        }
        
        // Set up client address
        data_client_address.sin_family = AF_INET;
        data_client_address.sin_addr.s_addr = INADDR_ANY; // Bind to any available interface
        data_client_address.sin_port = htons(client_plusone); // Bind to client_nplusone

        // Bind the socket to the client address
        if (bind(client_data_socket, (struct sockaddr *)&data_client_address, sizeof(data_client_address)) < 0) {
            printf("Error: Failed to bind socket\n");
            exit(1);
        }

        // Create server data address
        data_serv_address.sin_family = AF_INET;
        data_serv_address.sin_port = htons(DATA_PORT);

        // Convert IPv4 and IPv6 addresses from text to binary form
        if (inet_pton(AF_INET, LOCALHOST, &data_serv_address.sin_addr) <= 0) {
            printf("Error: Invalid IP address \n");
            exit(1);
        }

        // Listen for incoming connections
        if (listen(client_data_socket, 1) < 0) {
            printf("Error: Failed to listen on data socket\n");
            exit(1);
        }

        // ABOVE THIS EVERYTHING IS THE SAME FOR ANY PORT COMMAND
        // BELOW THIS EVERYTHING IS DIFFERENT FOR EACH PORT COMMAND

        if (strcmp(operation, "RETR") == 0){
            // Ensure operand is not NULL
            if (operand == NULL) {
                printf("Error: No filename specified for RETR command\n");
                close(client_data_socket);
                exit(1);
            }

            // Open the file for writing
            FILE *file = fopen(operand, "wb");  // write in binary mode
            if (file == NULL) {
                printf("Error: Failed to open file %s for writing\n", operand);
                close(client_data_socket);
                exit(1);
            }

            // Accept the incoming connection from the server
            int incoming_socket;
            struct sockaddr_in incoming_addr;
            socklen_t incoming_addr_len = sizeof(incoming_addr);
            if ((incoming_socket = accept(client_data_socket, (struct sockaddr *)&incoming_addr, &incoming_addr_len)) < 0) {
                printf("Error: Failed to accept incoming connection\n");
                fclose(file);
                close(client_data_socket);
                exit(1);
            }

            memset(buffer, 0, MAX_BUFFER);
            int bytes_received;
            while ((bytes_received = recv(incoming_socket, buffer, MAX_BUFFER, 0)) > 0) {
                fwrite(buffer, 1, bytes_received, file); // Write the received data to the file
            }

            if (bytes_received < 0) {
                printf("Error: Failed to receive data from server\n");
            } else {
                printf("\nFile transfer completed. Connection closed by server.\n");
            }

            fclose(file);
            close(incoming_socket);
            close(client_data_socket);
            exit(0); // Exit the child process   
        }
        else if (strcmp(operation, "LIST") == 0){
            // Accept the incoming connection from the server
            int incoming_socket;
            struct sockaddr_in incoming_addr;
            socklen_t incoming_addr_len = sizeof(incoming_addr);
            if ((incoming_socket = accept(client_data_socket, (struct sockaddr *)&incoming_addr, &incoming_addr_len)) < 0) {
                printf("Error: Failed to accept incoming connection\n");
                close(client_data_socket);
                exit(1);
            }
            // Receive the file listing from the server
            memset(buffer, 0, MAX_BUFFER);
            int bytes_received;
            while ((bytes_received = recv(incoming_socket, buffer, MAX_BUFFER, 0)) > 0) {
                printf("%s", buffer); // Print the received data
                memset(buffer, 0, MAX_BUFFER);
            }
            if (bytes_received < 0) {
                printf("Error: Failed to receive data from server\n");
            }

            close(incoming_socket);
            close(client_data_socket);
            exit(0); // Exit the child process
        }
        else if (strcmp(operation, "STOR") == 0){
            // Accept the incoming connection from the server
            int incoming_socket;
            struct sockaddr_in incoming_addr;
            socklen_t incoming_addr_len = sizeof(incoming_addr);
            if ((incoming_socket = accept(client_data_socket, (struct sockaddr *)&incoming_addr, &incoming_addr_len)) < 0) {
                printf("Error: Failed to accept incoming connection\n");
                close(client_data_socket);
                exit(1);
            }
            // Send the file to the server
            FILE *file = fopen(operand, "rb");  // read in binary mode
            if (file == NULL) {
                printf("Error: Failed to open file %s for reading\n", operand);
                close(client_data_socket);
                exit(1);
            }
            char buffer[MAX_BUFFER];
            int bytes_sent;
            while ((bytes_sent = fread(buffer, 1, MAX_BUFFER, file)) > 0) {
                send(incoming_socket, buffer, bytes_sent, 0);
            }
            fclose(file);
            close(incoming_socket);
            close(client_data_socket);
            exit(0); // Exit the child process
        }
    } 

    else {    // parent process
        // does NOT WAIT for the child process to finish
        // as this will PREVENT the server from accepting new connections
    }


}

// Send PORT command to server, 
int send_port_command(char host_address[], int tcp_port_address, int control_socket){

    int p1 = tcp_port_address / 256;
    int p2 = tcp_port_address % 256;

    int h1, h2, h3, h4;
    sscanf(host_address, "%d.%d.%d.%d", &h1, &h2, &h3, &h4);
    char buffer[MAX_BUFFER];
    sprintf(buffer, "PORT %d,%d,%d,%d,%d,%d", h1, h2, h3, h4, p1, p2);

    send(control_socket, buffer, strlen(buffer), 0);
    memset(buffer, 0, MAX_BUFFER);      // clear buffer for reading
    read(control_socket, buffer, MAX_BUFFER);
    printf("%s\n", buffer); 
    if (strcmp(buffer, PORT_SUCCESS) == 0) {
        return 0;
    }
    return -1;
}
int send_underlying_command(int control_socket, char operation[], char operand[]){
    
    char buffer[MAX_BUFFER];
    sprintf(buffer, "%s %s\0", operation, operand);
    send(control_socket, buffer, strlen(buffer), 0);

    memset(buffer, 0, MAX_BUFFER);      // clear buffer for reading
    read(control_socket, buffer, MAX_BUFFER);
    printf("%s\n", buffer);
    if (strcmp(buffer, TRANSFER_READY) == 0) {
        return 0;
    }
    return -1;
}
void input(char* prompt, char* input){  
    printf("%s", prompt);    // print prompt
    fflush(stdout);                         // flush stdout immediately after printing prompt
    fgets(input, 1000, stdin);              // get input
    input[strlen(input)-1] = '\0';          // remove \n from input
}
void handle_port_command(int control_socket, int control_port, int data_port_count, char* operation, char* operand) {
    
    // declare variables
    char buffer[MAX_BUFFER];
    int client_data_socket;
    struct sockaddr_in data_client_address;
    struct sockaddr_in data_serv_address;

    int client_nplusone = control_port + data_port_count;
    // send port_command
    int port_command_status = send_port_command(LOCALHOST, client_nplusone, control_socket);
    if (port_command_status == -1) {
        printf("Error: PORT command unsuccessful\n");
        return;
    }else{
        printf("%s\n", PORT_SUCCESS);
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("Error: Failed to fork");
        exit(1);
    } else if (pid == 0) { // Child process
        
        // at this PORT_SUCCESS has been received

        // Create client data socket
        if ((client_data_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("Error: Failed to create socket\n");
            exit(1);
        }
        
        // Set up client address
        data_client_address.sin_family = AF_INET;
        data_client_address.sin_addr.s_addr = INADDR_ANY; // Bind to any available interface
        data_client_address.sin_port = htons(client_nplusone); // Bind to client_nplusone

        // Bind the socket to the client address
        if (bind(client_data_socket, (struct sockaddr *)&data_client_address, sizeof(data_client_address)) < 0) {
            printf("Error: Failed to bind socket\n");
            exit(1);
        }

        // Create server data address
        data_serv_address.sin_family = AF_INET;
        data_serv_address.sin_port = htons(DATA_PORT);

        // Convert IPv4 and IPv6 addresses from text to binary form
        if (inet_pton(AF_INET, LOCALHOST, &data_serv_address.sin_addr) <= 0) {
            printf("Error: Invalid IP address \n");
            exit(1);
        }

        // Listen for incoming connections
        if (listen(client_data_socket, 1) < 0) {
            printf("Error: Failed to listen on data socket\n");
            exit(1);
        }

        printf("%d, Client listening on DATA port %d\n", get_unix_time(), ntohs(data_client_address.sin_port));
        

        // ABOVE THIS EVERYTHING IS THE SAME FOR ANY PORT COMMAND
        // BELOW THIS EVERYTHING IS DIFFERENT FOR EACH PORT COMMAND

        printf("||%s %s||\n", operation, operand);


        // Accept the incoming connection from the server
        int incoming_socket;
        struct sockaddr_in incoming_addr;
        socklen_t incoming_addr_len = sizeof(incoming_addr);
        if ((incoming_socket = accept(client_data_socket, (struct sockaddr *)&incoming_addr, &incoming_addr_len)) < 0) {
            printf("Error: Failed to accept incoming connection\n");
            exit(1);
        }

        memset(buffer, 0, MAX_BUFFER);
        recv(incoming_socket, buffer, MAX_BUFFER, 0);

        printf("Server response received: \n%s", buffer);

        close(incoming_socket);
        close(client_data_socket);
        exit(0); // Exit the child process
    } else {
        // Parent process continues to handle control connection
        // No changes needed here for the control connection
    }
}