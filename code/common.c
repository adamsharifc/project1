#include "common.h"

//     - CWD foldername
//         - This command is used to change the current server directory to `foldername`. The server must reply with `200 direcotry changed to pathname/foldername`
//     - !CWD foldername (executed locally at the client and should execute the corresponding shell commands through the system() function)
//         - This command is used to change the current client directory.
int sys_cwd(char dir[]){
    if (dir == NULL){
        printf("Error: dir is null\n");
        return -1;
    }
    if (!directory_exists(dir)){    // directory does not exist
        printf("Error: invalid directory\n");
        return -2;
    }
    int chdir_return_code = chdir(dir);

    if (chdir_return_code != 0){   // Command did not execute successfully
        printf("Error: invalid directory\n");
        return -2;
    }
    return chdir_return_code;
}

bool directory_exists(const char path[]) {
    return access(path, F_OK) != -1;
}
bool file_exists(const char path[]) {
    return access(path, F_OK) != -1;
}
int get_unix_time(){
    time_t current_time;
    time(&current_time);
    return current_time;
}