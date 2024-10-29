// PROJECT 1: FILE TRANSPORT PROTOCOL (CLIENT)
// Author: Adam Sharif
// Date: 11-10-2024

#include "client.h"

int main(int argc, char *argv[]) {

    bool is_test_mode = false;
    char* test_commands[MAX_TEST_LINES] = {0};

    // Check if any arguments were provided
    if (argc > 1 && strcmp(argv[1], "-t") == 0) {
        is_test_mode = true;
        if (argc > 2) {
            handle_test_mode(test_commands, argv[2]);
        } else {
            printf("Error: No file path provided for test mode\n");
            return -1;
        }
    }

    int server_control_socket = 0;                     // control socket on the client side manages control channel 
    int client_data_socket = 0;                        // data socket on the client side manages data channel
    int server_control_port = 0;                       // control port on the client side maanges control port
    int client_data_port_count = 1;                    // counter variable to keep track of the number of data connections made by the client, use to uniquely identify the next port  
    struct sockaddr_in client_control_address;
    struct sockaddr_in client_data_address;
    struct sockaddr_in server_control_address;
    struct sockaddr_in server_data_address; 
    char buffer[MAX_BUFFER] = {0};              // buffer for storing/parsing/sending messages
    char command[MAX_BUFFER];                   // buffer for storing/parsing/sending commands
    char working_dir[MAX_BUFFER];               // buffer for storing current working directory 
    char* operation;                            // STOR, RETR, USER, PWD
    char* operand;                              // cat.png, dog.jpg, bob, donuts
    socklen_t addr_len = sizeof(client_control_address);

    // Create socket
    if ((server_control_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error: Failed to create socket\n");
        return -1;
    }

    // Set up client address
    client_control_address.sin_family = AF_INET;
    client_control_address.sin_addr.s_addr = INADDR_ANY; // Bind to any available interface
    client_control_address.sin_port = htons(0); // Bind to any available port

    // Bind the socket to the client address
    if (bind(server_control_socket, (struct sockaddr *)&client_control_address, sizeof(client_control_address)) < 0) {
        printf("Error: Failed to bind socket\n");
        return -1;
    }

    // Retrieve the assigned port
    if (getsockname(server_control_socket, (struct sockaddr *)&client_control_address, &addr_len) < 0) {
        printf("Error: Failed to get socket name\n");
        return -1;
    }

    // setup and configure control port
    server_control_port = ntohs(client_control_address.sin_port);
    printf("Client bound to port %d\n", server_control_port);
    server_control_address.sin_family = AF_INET;
    server_control_address.sin_port = htons(CONTROL_PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, LOCALHOST, &server_control_address.sin_addr) <= 0) {
        printf("Error: Invalid IP address \n");
        return -1;
    }

    // Connect to server
    if (connect(server_control_socket, (struct sockaddr *)&server_control_address, sizeof(server_control_address)) < 0) {
        printf("Error: Failed to connect to server\n");
        return -1;
    }

    // CONNECTED
    printf("%s%s\n%s\n", INIT_PROMPT, COMMAND_LIST, SERVICE_READY);
    printf("%s\n", SERVICE_READY);

    // loop indefinitely, break statement => IF statement checking for SERVICE_QUIT message
    int test_command_index = 0;
    while(true){

        // if test mode is enabled, iterate through test_commands array, instead of taking user input
        if (is_test_mode == true){
            if (test_commands[test_command_index] == NULL){
                printf("End of test script\n");
                break;
            }
            printf("ftp-test> %s\n", test_commands[test_command_index]);
            strncpy(command, test_commands[test_command_index], MAX_BUFFER);
            test_command_index++;
        }
        else{
            printf("ftp>");
            input("", command);
        }

        if (strlen(command) == 0){continue;}

        strncpy(buffer, command, MAX_BUFFER);
        operation = strtok(buffer, " ");
        operand = strtok(NULL, " ");

        if (operation[0] == '!'){                       // LOCAL COMMANDS
            if (strcmp(operation, "!PWD") == 0){        // PRINT WORKING DIRECTORY
                getcwd(working_dir, MAX_BUFFER);
                printf("%s\n", working_dir);            
            }
            else if (strcmp(operation, "!CWD") == 0){   // CHANGE WORKING DIRECTORY
                if (sys_cwd(operand) == 0){
                    getcwd(working_dir, MAX_BUFFER);
                    printf("Directory changed to %s\n", working_dir);
                }
            }
            else if (strcmp(operation, "!LIST") == 0){  // LIST WORKING DIRECTORY CONTENTS
                system("ls");   fflush(stdout);
            }
            else{
                printf("%s\n", INVALID_COMMAND);
            }
        }
        else if ((strcmp(operation, "USER") == 0) ||    // CONTROL COMMANDS (DONT REQUIRE DATA SOCKET)
                 (strcmp(operation, "PASS") == 0) || 
                 (strcmp(operation, "PWD") == 0) || 
                 (strcmp(operation, "CWD") == 0) || 
                 (strcmp(operation, "QUIT") == 0)){
            
            send(server_control_socket, command, strlen(command), 0);  // send `command` of size `strlen(command)` to `control_socket` 
            read(server_control_socket, buffer, MAX_BUFFER);           // read into `buffer` size `MAX_BUFFER` characters from `control_socket`
            printf("%s\n", buffer);                             // print response
            
            // Terminate after server replies with SERVICE_QUIT message
            if (strcmp(buffer, SERVICE_QUIT) == 0){             // ONLY WAY TO EXIT LOOP
                // close sockets
                close(server_control_socket);
                close(client_data_socket);
                exit(0);
                break;
            }
            memset(buffer, 0, MAX_BUFFER);                      // clear buffer
        }
        else if ((strcmp(operation, "LIST") == 0) ||  // DATA COMMANDS (REQUIRE DATA SOCKET)
                 (strcmp(operation, "RETR") == 0) ||
                 (strcmp(operation, "STOR") == 0)){  
            
            int client_nplusone = server_control_port + client_data_port_count;    // get port number of new 
            client_data_port_count++;
            handle_data_command(server_control_socket, client_nplusone, operation, operand);
        }
        else {
            // PROBABLY AN INVALID COMMAND, send through control channel
            send(server_control_socket, command, strlen(command), 0);  // send `command` of size `strlen(command)` to `control_socket` 
            read(server_control_socket, buffer, MAX_BUFFER);           // read into `buffer` size `MAX_BUFFER` characters from `control_socket`
            printf("%s\n", buffer);    
            memset(buffer, 0, MAX_BUFFER);                      // clear buffer
        }
    }

    return 0;
} 

void handle_data_command(int server_control_socket, int client_nplusone, char operation[], char operand[]){
    // check if port is available
    while(is_port_available(client_nplusone) == false){
        client_nplusone++;
    }

    char buffer[MAX_BUFFER];
    int client_data_socket;
    struct sockaddr_in client_data_address;
    struct sockaddr_in server_data_address;

    // Create client data socket
    if ((client_data_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error: Failed to create socket\n");
        exit(1);
    }
    
    // Set up client address
    client_data_address.sin_family = AF_INET;
    client_data_address.sin_addr.s_addr = INADDR_ANY; // Bind to any available interface
    client_data_address.sin_port = htons(client_nplusone); // Bind to client_nplusone

    // Bind the socket to the client address
    if (bind(client_data_socket, (struct sockaddr *)&client_data_address, sizeof(client_data_address)) < 0) {
        printf("Error: Failed to bind socket, collision already using %d\n", client_nplusone);
        exit(1);
    }

    // Send PORT command to server
    int port_command_result = send_port_command(LOCALHOST, client_nplusone, server_control_socket);
    if (port_command_result != 0){
        printf("Error: Failed to send PORT command\n");
        return;
    }
    // At this point the Server replies with: "200 PORT command successful."
    // Send underlying command to server either of LIST, RETR, STOR 
    int underlying_command_result = send_underlying_command(server_control_socket, operation, operand);
    if (underlying_command_result != 0){
        if (underlying_command_result == 1){  // if invalid resource message is receievd its already been printed, so dont print another error but dont proceed further either
            close(client_data_socket);
            return;
        }
        printf("Error: Failed to send underlying command\n");
        close(client_data_socket);
        return;
    }

    // At this point the Server replies with "150 File status okay; about to open data connection."
    // Create server data address
    server_data_address.sin_family = AF_INET;
    server_data_address.sin_port = htons(DATA_PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, LOCALHOST, &server_data_address.sin_addr) <= 0) {
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

    if (strcmp(operation, "RETR") == 0){    // retrieve a file from the server, create a file and write data to it from the server
        // Ensure operand is not NULL
        if (operand == NULL) {
            printf("Error: No filename specified for RETR command\n");
            close(client_data_socket);
            exit(1);
        }

        srand(time(0));   
        int random_number = rand();
        char temp_filename[MAX_BUFFER];
        sprintf(temp_filename, "%d.temp", random_number);

        // Open the file for writing
        FILE *file = fopen(temp_filename, "wb");  // write in binary mode
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

        memset(buffer, 0, MAX_BUFFER);      // clear buffer
        int bytes_received;
        while ((bytes_received = recv(incoming_socket, buffer, MAX_BUFFER, 0)) > 0) {   // keep receiving data from server. until bytes_received is 0
            fwrite(buffer, 1, bytes_received, file); // Write the received data to the file
        }

        if (bytes_received < 0) {
            printf("Error: Failed to receive data from server\n");
        } else {
            printf("\nFile transfer completed. Connection closed by server.\n");
        }

        fclose(file);

        // Rename the temporary file to the correct filename
        if (rename(temp_filename, operand) != 0) {
            printf("Error: Failed to rename file\n");
        }

        close(incoming_socket);
        close(client_data_socket);
    }
    else if (strcmp(operation, "LIST") == 0){       // list files in the server working directory
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
        memset(buffer, 0, MAX_BUFFER);              // clear buffer
        int bytes_received;
        while ((bytes_received = recv(incoming_socket, buffer, MAX_BUFFER, 0)) > 0) {
            printf("%s", buffer); // Print the received data
            memset(buffer, 0, MAX_BUFFER);          // clear buffer
        }
        fflush(stdout);     // force flush to stdout, to avoid inconsistency in printing order when there are tests where fast iteration thorugh commands may cause problems
        if (bytes_received < 0) {
            printf("Error: Failed to receive data from server\n");
        }

        close(incoming_socket);
        close(client_data_socket);
    }
    else if (strcmp(operation, "STOR") == 0){           // store a file on the server, read a file and send data to the server until EOF
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
        FILE *file = fopen(operand, "rb");  // read in binary mode, `r` mode may cause data loss
        if (file == NULL) {
            printf("Error: Failed to open file %s for reading\n", operand);
            close(client_data_socket);
            exit(1);
        }
        char buffer[MAX_BUFFER];
        int bytes_sent;
        while ((bytes_sent = fread(buffer, 1, MAX_BUFFER, file)) > 0) {     // keep reading data from file until bytes_sent is 0
            send(incoming_socket, buffer, bytes_sent, 0);
        }
        fclose(file);
        close(incoming_socket);
        close(client_data_socket);
    }
} 

// Send PORT command to server, 
int send_port_command(char host_address[], int tcp_port_address, int server_control_socket){

    int p1 = tcp_port_address / 256;        // calculate p1 and p2 from tcp_port_address
    int p2 = tcp_port_address % 256;
    int h1, h2, h3, h4;     
    char buffer[MAX_BUFFER];

    sscanf(host_address, "%d.%d.%d.%d", &h1, &h2, &h3, &h4);        // parse host_address into h1, h2, h3, h4
    sprintf(buffer, "PORT %d,%d,%d,%d,%d,%d", h1, h2, h3, h4, p1, p2);

    send(server_control_socket, buffer, strlen(buffer), 0);
    memset(buffer, 0, MAX_BUFFER);              // clear buffer for reading
    read(server_control_socket, buffer, MAX_BUFFER);
    printf("%s\n", buffer);                     // print response
    if (strcmp(buffer, PORT_SUCCESS) == 0) {    // if server replies with "200 PORT command successful."
        return 0;
    }
    return -1;
}
// Send underlying command to server, either of LIST, RETR, STOR
int send_underlying_command(int server_control_socket, char operation[], char operand[]){
    char buffer[MAX_BUFFER];
    sprintf(buffer, "%s %s\0", operation, operand);
    send(server_control_socket, buffer, strlen(buffer), 0);

    memset(buffer, 0, MAX_BUFFER);      // clear buffer for reading
    read(server_control_socket, buffer, MAX_BUFFER);
    printf("%s\n", buffer);                     // print response
    if (strcmp(buffer, TRANSFER_READY) == 0) {  // if server replies with "150 File status okay; about to open data connection.", 
        return 0;
    }
    else if (strcmp(buffer, INVALID_RESOURCE) == 0){    // if invalid resource message is receievd its already been printed, so dont print another error but dont proceed further either
        return 1;
    }
    return -1;
}
void input(char* prompt, char* input){  
    printf("%s", prompt);    // print prompt
    fflush(stdout);                         // flush stdout immediately after printing prompt
    fgets(input, 1000, stdin);              // get input
    input[strlen(input)-1] = '\0';          // remove \n from input
}
bool is_port_available(int port_number){
    int client_data_socket;
    struct sockaddr_in client_data_address;

    // Set up client address
    client_data_address.sin_family = AF_INET;
    client_data_address.sin_addr.s_addr = INADDR_ANY; // Bind to any available interface
    client_data_address.sin_port = htons(port_number); // Bind to client_nplusone

    // Create client data socket
    if ((client_data_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error: Failed to create socket\n");
        return false;
    }
    
    // Set up client address
    client_data_address.sin_family = AF_INET;
    client_data_address.sin_addr.s_addr = INADDR_ANY; // Bind to any available interface
    client_data_address.sin_port = htons(port_number); // Bind to client_nplusone

    // Bind the socket to the client address
    if (bind(client_data_socket, (struct sockaddr *)&client_data_address, sizeof(client_data_address)) < 0) {
        // printf("Error: Failed to bind socket, collision already using %d\n", port_number);
        return false;
    }
    else{
        close(client_data_socket);
        return true;
    }
}
void handle_test_mode(char *test_commands[], char *file_path) {
    printf("RUNNING IN TEST MODE\n");
    FILE *file;
    char line[MAX_BUFFER];
    int line_count = 0;

    // Open the file for reading
    file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(-1);
    }

    // Read each line and store it in the test_commands array
    while (fgets(line, MAX_BUFFER, file) != NULL && line_count < MAX_TEST_LINES) {
        // Remove newline character if present
        line[strcspn(line, "\n")] = '\0';
        test_commands[line_count] = strdup(line);
        if (test_commands[line_count] == NULL) {
            perror("Error allocating memory");
            fclose(file);
            exit(-1);
        }
        line_count++;
    }

    fclose(file);
}