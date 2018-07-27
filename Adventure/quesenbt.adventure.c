/*
 * Author: Tanner Quesenberry
 * Date: 7/24/17
 * References:
 *    Class notes/lectures
 *    Piazza posts
 *    en.cppreference.com/w/c/chrono/strftime
 *    https://linux.die.net/man/3/localtime
 *    http://man7.org/linux/man-pages/man/man3/getline.3.html
 *    https://stackoverflow.com/questions/12252103/how-to-read-a-line-from-stdin-blocking-until-the-newline-is-found
 *    https://stackoverflow.com/questions/9628637/how-can-i-get-rid-of-n-from-string-in-c
 *    https://www.tutorialspoint.com/c_standard_library/c_function_strcmp.html
 *    http://pubs.opengroup.org/onlinepubs/009695399/functions/fscanf.html
 *    https://www.tutorialspoint.com/c_standard_library/c_function_fscanf.html
 *    https://stackoverflow.com/questions/1088622/how-do-i-create-an-array-of-strings-in-c/
 *    https:://stackoverflow.com/questions/15161774/how-to-create-an-array-of-strings-in-c
 *    man7.org/linux/man-pages/man3/pthread_join.3.html
 *    https://www.tutorialspoint.com/c_standard_library/time_h.htm
 */

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#define TR 7

struct Room {
    char name[6];
    char type[15];
    int totalConnections;
    char outbound[6][6];  // Array of strings
};

// Global array for rooms read from files
struct Room rooms[TR];

// Hardcoded room names
char* roomNames[] = {"Rome", "Kiev", "Lima", "Oslo", "Nice", "Giza", "Lyon", "Bern", "Gaza", "Xian"};

// Thread function 
void* displayTime(void* argument){
    // Have thread lock mutex
    pthread_mutex_lock((pthread_mutex_t *)argument);

    // Open file to write time to
    FILE *timeFile = fopen("currentTime.txt", "w");

    // Get time since Epoch
    time_t timer;
    char timeString[40];
    // Stores time information
    struct tm* time_info;

    // Get time
    time(&timer);
    // Get time info
    time_info = localtime(&timer);
    // Convert time info to string
    strftime(timeString, 40, "%l:%M%P, %A, %B %e, %Y", time_info);

    // Write time to file
    fprintf(timeFile, "%s", timeString);

    // close file
    fclose(timeFile);
    return NULL;
}

int main(){
    // Creat a mutex and lock it 
    pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&myMutex);

    // Creat second thread for time
    int resultInt;
    pthread_t myThreadID;
    resultInt = pthread_create(&myThreadID, NULL, displayTime, &myMutex);

    // Initialize each Room in rooms
    int i;
    for(i = 0; i < TR; i++){
        memset(rooms[i].name, '\0', sizeof(rooms[i].name));
        memset(rooms[i].type, '\0', sizeof(rooms[i].type));
        rooms[i].totalConnections = 0;
        memset(rooms[i].outbound[i], '\0', 6);
    }


    // Check for most recent directory
    int newDirTime = -1;
    char targetDirPrefix[32] = "quesenbt.rooms.";
    char newestDirName[256];
    memset(newestDirName, '\0', sizeof(newestDirName));

    DIR* dirToCheck;
    struct dirent *fileInDir;
    struct stat dirAttributes;

    dirToCheck = opendir(".");

    // If directory can be opened
    if(dirToCheck > 0){
        // for each file
        while((fileInDir = readdir(dirToCheck)) != NULL){
            // If target prefix found
            if(strstr(fileInDir->d_name, targetDirPrefix) != NULL){
                // Check timestamp
                stat(fileInDir->d_name, &dirAttributes);

                // Update if timestamp newer
                if((int)dirAttributes.st_mtime > newDirTime){
                    newDirTime = (int)dirAttributes.st_mtime;
                    memset(newestDirName, '\0', sizeof(newestDirName));
                    strcpy(newestDirName, fileInDir->d_name);
                }
            }
        }
    }

    closedir(dirToCheck);


    // Read files into structs
    FILE* dataFile;
    char fileName[200];
    char roomType[20];
    char roomName[6];
    int j = 0;
    int connectionCount;

    // Open directory
    DIR* fileDir = opendir(newestDirName);
    // if directory can be opened
    if(fileDir > 0){
        // for each file
        while((fileInDir = readdir(fileDir)) != NULL){
            // Don't process . or ..
            if(!strcmp(fileInDir->d_name, "."))
                continue;
            if(!strcmp(fileInDir->d_name, ".."))
                continue;

            // Reset file name holder
            memset(fileName, '\0', sizeof(fileName));
            // Get path to current file with directory name and file name
            sprintf(fileName, "%s/%s", newestDirName, fileInDir->d_name);
            // Open the room file
            dataFile = fopen(fileName, "r");
           
            
            // Get room name in line 1
            memset(roomName, '\0', sizeof(roomName));
            fscanf(dataFile, "ROOM NAME: %s\n", roomName);
            strcpy(rooms[j].name, roomName);
 
            // Get connections for current room file, also increment total connections
            connectionCount = 0;
            memset(roomName, '\0', sizeof(roomName));

            // Read in each connection to current room and assign it in room's struct
            while(fscanf(dataFile, "CONNECTION %d: %s\n", &connectionCount, roomName)){
                strcpy(rooms[j].outbound[connectionCount - 1], roomName);
            }
            // Assign total connections, this depends on file format
            // Adjust accordingly if numbering system in files changes
            rooms[j].totalConnections = connectionCount;

            // Get type of current room
            memset(roomType, '\0', sizeof(roomType));
            fscanf(dataFile, "ROOM TYPE: %s", roomType);
            strcpy(rooms[j].type, roomType);

            // Increment counter to upload room data to next struct in array
            j++;

            // Close current file
            fclose(dataFile);
        }

    }
    // Close room directory
    closedir(fileDir);


    // Find start room
    int location;
    for(i = 0; i < TR; i++){
        if(strcmp("START_ROOM", rooms[i].type) == 0){
            location = i;
        }
    }

    // To track player path
    char path[50][6];
    int steps = 0;
    char* userInput = NULL;
    bool pathExists;

    // Begin game loop
    while(true){
        // Display current location
        printf("CURRENT LOCATION: %s\n", rooms[location].name);
        // Display possible connections
        printf("POSSIBLE CONNECTIONS: ");
        for(i = 0; i < rooms[location].totalConnections; i++){
            // If last option print with . and newline.
            // Else print with , and space
            if(i == rooms[location].totalConnections - 1){
                printf("%s.\n", rooms[location].outbound[i]);
            }else{
                printf("%s, ", rooms[location].outbound[i]);
            }
        }
        //Prompt user
        printf("WHERE TO? >");
        // Get input
        size_t size;
        if(getline(&userInput, &size, stdin) != -1){
            // Strip off the newline character
            userInput[strlen(userInput) - 1] = '\0';
        }

        // If time entered
        while(strcmp(userInput, "time") == 0){
            // Unlock thread B
            pthread_mutex_unlock(&myMutex);
            int join;
            // Halt thread A until thread B completes
            join = pthread_join(myThreadID, NULL);
           
            // lock mutex again?
            //pthread_mutex_lock(&myMutex);

            // create on thread b again
            resultInt = pthread_create(&myThreadID, NULL, displayTime, &myMutex);
        
            // Read data from file/print
            dataFile = fopen("currentTime.txt", "r");
            char fileTime[45];
            memset(fileTime, '\0', sizeof(fileTime));
            // Read time from file
            fgets(fileTime, sizeof(fileTime), dataFile);
            // Print time
            printf("\n%s\n\n", fileTime);
            // Close file
            fclose(dataFile);

            // Prompt again
            printf("WHERE TO? >");
            if(getline(&userInput, &size, stdin) != -1){
                // Strip off the newline character
                userInput[strlen(userInput) - 1] = '\0';
            }
            
        }

        // Loop through available connections to check that input is valic choice
        pathExists = false;
        for(i = 0; i < rooms[location].totalConnections; i++){
            if(strcmp(userInput, rooms[location].outbound[i]) == 0){
                pathExists = true;
            }
        }
    
        // Print error or change rooms
        if(pathExists == false){
            printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
        }else{
            // CHange rooms here, loop through rooms array to find
            // room name index matching user input room
            for(i = 0; i < TR; i++){
                if(strcmp(userInput, rooms[i].name) == 0){
                location = i;
                }
            }
            // Place new room in path
            strcpy(path[steps], userInput);
            // Increment step counter
            steps++;
            // If new room is end, break from loop
            if(strcmp("END_ROOM", rooms[location].type) == 0){
            break;
            }
            printf("\n");
        }
    }


    // Display total steps and path taken
    printf("\nYOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);
    for(i = 0; i < steps; i++){
        printf("%s\n", path[i]);
    }

    // Destroy the mutex
    pthread_mutex_destroy(&myMutex);
    return 0;
}
