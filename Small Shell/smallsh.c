/* 
 * Author: Tanner Quesenberry
 * Date: 8/7/17
 * References:
 *	Class Notes
 *      pubs.opengroup.org/onlinepubs/007908799/xsh/strncpy.html
 *      man7.org/linux/man-pages/man3/getline.3.html
 *      fresh2fresh.com/c-programming/c-string/c-strstr-function/
 *      https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
 *      https://stackoverflow.com/questions/18838933/why-do-i-first-have-to-strcpy-before-strcat
 *      https://www.tutorialspoint.com/c_standard_libary/c_function_get_env.htm
 *      https://stackoverflow.com/questions/9493234/chdir-to-home-directory
 *      https://tutorialspoint.com/c_standard_library/c_function_strncmp.htm
 *      https://www.tutorialspoint.com/c_standard_library/c_function.strcat.htm
 *
 */

// Include header files
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

// Macro
#define MAX_COMMAND 2048
#define MAX_ARGS 512

// Function prototypes
void prompt();
void getInput();
void checkBackground();

// Global variables
bool background_process_flag = false;
bool redirect_input_flag = false;
bool redirect_output_flag = false;
bool foreground_only = false;

char* input = NULL;
char* args[MAX_ARGS];
char command[MAX_COMMAND];
char toToken[MAX_COMMAND];
int totalArgs;
int loop = 1;
int background_process[200];
int total_background = 0;
int stat = 0;
int childExitMethod = 0;
int current_foreground_process;

// Function used with sigaction struct to catch SIGINT
void catchSIGINT(int signo){
    // Kill the current foreground process
    kill(current_foreground_process, SIGKILL);
    // Notify user
    char* message = "terminated by signal 2\n";
    write(STDOUT_FILENO, message, 23);
    // Update the most recent exit status
    waitpid(current_foreground_process, &childExitMethod, 0);
}

// Function used with struct sigaction to catch SIGTSTP
void catchSIGTSTP(int signo){
    char* message1 = "\nEntering foreground-only mode (& is now ignored)\n";
    char* message2 = "\nExiting foreground-only mode\n";

    // Change command state
    if(foreground_only == false){
        foreground_only = true;
        write(STDOUT_FILENO, message1, 50);
    }else{
        foreground_only = false;
        write(STDOUT_FILENO, message2, 30);
    }
    
}

// MAIN
int main(int argc, char* argv[]){
    int reset;
    //int childExitMethod = 0;

    // CTRL-C sigaction struct
    struct sigaction SIGINT_action = {0};
    SIGINT_action.sa_handler = catchSIGINT;    
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;

    // CTRL-Z sigaction struct
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;

    while(loop == 1){
        // Set sigactions each loop
        sigaction(SIGINT, &SIGINT_action, NULL);
        sigaction(SIGTSTP, &SIGTSTP_action, NULL);

        // Reset command arguments to NULL for each command
        for(reset = 0; reset < MAX_ARGS; reset++){
            args[reset] = NULL;
        }

        // Display finished background processes, then gather user input
        checkBackground();
        prompt();
        getInput();

        // Blank line, skip
        if(strlen(command) == 0){
            continue;
        // Re prompt if starts with #, this is a comment
        }else if(strncmp(command, "#", 1) == 0){
            continue;
        // Exit program
        }else if(strcmp(command, "exit") == 0){
            loop = 0;
            // Kill processes
            int i;
            for(i = 0; i < total_background; i++){
                kill(background_process[i], SIGKILL);
            }
        }else if(strcmp(command, "status") == 0){
            // Status stuff

            // Ended by signal?
            if(WIFSIGNALED(childExitMethod)){
                // Get specific signal number and display
                stat = WTERMSIG(childExitMethod);
                printf("terminated by signal %d\n", stat);
                fflush(stdout);  ////////////////
            }
            // Ended by finishing?
            if(WIFEXITED(childExitMethod)){
                // Get exit code and display
                stat = WEXITSTATUS(childExitMethod);
                printf("exit value %d\n", stat);
                fflush(stdout);    //////////
            }

            //stat = WEXITSTATUS(childExitMethod);
            //printf("exit value %d\n", stat);
        }else if(strncmp("cd", command, 2) == 0){
            // Change directory here
            // If just cd, go to HOME directory
            if(strlen(command) == 2){
                char* home = getenv("HOME");
                chdir(home);
            }else{
                // Get the current directory
                char current_Dir[1200];
                getcwd(current_Dir, sizeof(current_Dir));
                
                // Add / to path if not present
                if(strncmp(command, "cd /", 4) != 0){
                    strcat(current_Dir, "/");
                }

                // Input directory starts at index 3 (cd directory)
                strcat(current_Dir, &command[3]);

                // Change to input directory
                chdir(current_Dir);
                //printf("%s", current_Dir);
            }
        }else{
            // Fork off child process based on background or foreground
            if(background_process_flag && foreground_only == false){
                // Background process
                
                pid_t spawnpid = -5;
                spawnpid = fork();
                // Make copy of command to tokenize through
                strcpy(toToken, command);
                char* token;
                char* newInput;
                char* newOutput;
                
                // Get initial token
                token = strtok(toToken, " ");
                totalArgs = 0;
                // Add tokens to args array to be used in executing
                while( token != NULL){
                   // If redirect input
                   if(strcmp(token, "<") == 0){
                       // Get next token
                       token = strtok(NULL, " ");
                       // Assign input location
                       newInput = token;
                       // Get next token to analyze
                       token = strtok(NULL, " ");
                       
                   // If redirect output
                   }else if(strcmp(token, ">") == 0){
                       // Get next token
                       token = strtok(NULL, " ");
                       // Assign output location
                       newOutput = token;
                       // Get next token to analyze
                       token = strtok(NULL, " ");

                   }else{
                        // Put arguments into args array for execvp
                        args[totalArgs] = token;
                        totalArgs += 1;
                        token = strtok(NULL, " ");
                    }
                }
                
                // Spawn child process
    //            spawnpid = fork();
                int result;
                // File descriptors
                int sourceFD;
                int targetFD;
                switch(spawnpid){
                    // Error
                    case -1:
                       // perror("Error forking child process.");
                        exit(1);
                        break;
                    // Child process
                    case 0:
                        // If < redirect
                        if(redirect_input_flag == true){
                            // Source input
                            sourceFD = open(newInput, O_RDONLY, 0);
                            // Error check
                            if(sourceFD == -1){
                                printf("Cannot open %s to receive input\n", newInput);
                                exit(1);
                            }else{
                                // Redirect
                                result = dup2(sourceFD, 0);
                                close(sourceFD);
                            }
                        }else{
                            // Redirect to /dev/null if no input location given
                            sourceFD = open("/dev/null", O_RDONLY, 0);
                            result = dup2(sourceFD, 0);
                            close(sourceFD);
                        }

                        // If > redirect
                        if(redirect_output_flag == true){
                            // Destination output
                            targetFD = open(newOutput, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                            // Error check
                            if(targetFD == -1){
                                printf("Cannot open %s for output", newOutput);
                                exit(1);
                            }else{
                                // Redirect
                                result = dup2(targetFD, 1);
                                close(targetFD);
                            }
                        }else{
                            // Redirect to /dev/null if no output location given
                            targetFD = open("/dev/null", O_WRONLY);
                            result = dup2(targetFD, 1);
                            close(targetFD);
                        }
                        
                        // Execute command
                        execvp(args[0], args);
                        // If command not recognized, print message
                        // This line is not executed if command is valid
                        printf("No file or directory named: %s\n",args[0]);
                        exit(1);
                    // Parent process
                    default:
                        // Put process id in background array
                        background_process[total_background] = spawnpid;
                        total_background += 1;
                        // Print background id to user
                        printf("background pid is %d\n", spawnpid);
                        fflush(stdout);   /////////
                        continue;
                }

            }else{
                // Foreground process
                pid_t spawnpid = -5;
                // Make copy of command to tokenize through
                strcpy(toToken, command);
                char* token;
                char* newInput;
                char* newOutput;

                // Get initial token
                token = strtok(toToken, " ");
                totalArgs = 0;
                // Add tokens to args array to be used in executing
                while( token != NULL){
                   // If redirect input
                   if(strcmp(token, "<") == 0){
                       // Get next token
                       token = strtok(NULL, " ");
                       // Assign input location
                       newInput = token;
                       // Get next token to analyze
                       token = strtok(NULL, " ");
                       
                   // If redirect output
                   }else if(strcmp(token, ">") == 0){
                       // Get next token
                       token = strtok(NULL, " ");
                       // Assign output location
                       newOutput = token;
                       // Get next token to analyze
                       token = strtok(NULL, " ");

                   }else{
                        // Put arguments into args array for execvp
                        args[totalArgs] = token;
                        totalArgs += 1;
                        token = strtok(NULL, " ");
                    }
                }
                // Make sure last argument is null
                args[totalArgs] = NULL;

                // Spawn child process
                spawnpid = fork();
                // Set current process id, used in catchSIGINT
                current_foreground_process = spawnpid;
                int result;
                // File descriptors
                int sourceFD;
                int targetFD;
                switch(spawnpid){
                    // Error
                    case -1:
                    //    perror("Error forking child process.");
                        exit(1);
                        break;
                    // Child process
                    case 0:
                        // If < redirect
                        if(redirect_input_flag == true){
                            // Source input
                            sourceFD = open(newInput, O_RDONLY, 0);
                            // Error check
                            if(sourceFD == -1){
                                printf("Cannot open %s to receive input\n", newInput);
                                exit(1);
                            }else{
                                // Redirect
                                result = dup2(sourceFD, 0);
                                close(sourceFD);
                            }
                        }
                        // If > redirect
                        if(redirect_output_flag == true){
                            // Destination output
                            targetFD = open(newOutput, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                            // Error check
                            if(targetFD == -1){
                                printf("Cannot open %s for output", newOutput);
                                exit(1);
                            }else{
                                // Redirect
                                result = dup2(targetFD, 1);
                                close(targetFD);
                            }
                        }
                        
                        // Execute command
                        execvp(args[0], args);
                        // If command not recognized, print message
                        // This line is not executed if command is valid
                        printf("No file or directory named: %s\n",args[0]);
                        fflush(stdout);  ////////////////
                        exit(1);
                    // Parent process
                    default:
                        // Wait for child to finish and put exit value in childExitMethod
                        waitpid(spawnpid, &childExitMethod, 0);
                }

            }

        }

    }


    free(input);
    return 0;
}





// Get user input function, set flags
void getInput(){

    // Rest flag variables to 0
    background_process_flag = false;
    redirect_input_flag = false;
    redirect_output_flag = false;
 
    // Receive input
    size_t size;
    int charsEntered = -5;
    charsEntered = getline(&input, &size, stdin);
    if(charsEntered == -1){
        clearerr(stdin);
    }

    // Strip off newline
    input[strlen(input) - 1] = '\0';

    // Replace $$ with process id where necessary
    int i;
    // Get process id
    int process = getpid();
    // Temp string
    char temp[10];
    memset(temp, '\0', 10);
    // Turn int into string
    sprintf(temp, "%d", process);
    memset(command, '\0', MAX_COMMAND);
    // Copy over input replacing $$'s
    for(i = 0; i < strlen(input); i++){
        // If next char is not $, catenate
        if(strncmp(&input[i], "$", 1) != 0){
            strncat(command, &input[i], 1);
        // If $ found, check is another $ follows, replace with temp
        }else if(strncmp(&input[i + 1], "$", 1) == 0){
            strcat(command, temp);
            i = i + 1;
        // Else it is a single $, do not replace
        }else{
            strncat(command, &input[i], 1);
        }
    }

    // Copy max command of 2048 chars into command from user input
    //strncpy(command, input, sizeof(command));


    // Search input for flag symbols, set as appropriate
    char* ret = strstr(command, " < ");

    if(ret){
        redirect_input_flag = true;
    }

    ret = strstr(command, " > ");
    if(ret){
        redirect_output_flag = true;
    }

    // & is at end of input, no error checking required per assignment
   // ret = strstr(command, " &");
    if(strcmp("&", &command[strlen(command) - 1]) == 0){
        background_process_flag = true;
        // Remove & and space from end of string if present as it is irrelevent once flag is set
        command[strlen(command) - 1] = '\0';
        command[strlen(command) - 1] = '\0';
    }

    // Testing line
//    printf("%s", command);
    return;
}

// Print the command prompt to user and flush output stream
void prompt(){
    // Extra flushing
    fflush(stdout);
    // Command prompt for user
    printf(": ");
    fflush(stdout);
//    char* colon = ": ";
//    write(STDOUT_FILENO, colon, 2);
    return;
}

// Print messages for finished background processes
void checkBackground(){
    int i;
    int current;
    // For each background process
    for(i = 0; i < total_background; i++){
        // Check if finished
        if(waitpid(background_process[i], &current, WNOHANG) > 0){
            // Ended by signal?
            if(WIFSIGNALED(current)){
                // Get specific signal number and display
                int termSignal = WTERMSIG(current);
                printf("background pid %d is done: terminated by signal %d\n", background_process[i], termSignal);
                fflush(stdout);
            }
            // Ended by finishing?
            if(WIFEXITED(current)){
                // Get exit code and display
                int exited = WEXITSTATUS(current);
                printf("background pid %d is done: exit value %d\n", background_process[i], exited);
                fflush(stdout);
            }
        }
    }

    return;
}
