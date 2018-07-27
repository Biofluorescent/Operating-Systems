/* 
 * Author: Tanner Quesenberry
 * Date: 7/24/17
 * References:
 * 	Class notes
 * 	Piazza Posts
 *      https://stackoverflow.com/questions/822323/how-to-generate-a-random-number-in-c  
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>

// Total number of rooms to make
#define TR 7

// struct Room declaration
struct Room {
    int id;
    char* name;
    int totalConnections;
    struct Room* outboundRoom[6];
};

// Global array of room structs
struct Room rooms[TR];

// Hardcoded room names
char* roomNames[] = {"Rome", "Kiev", "Lima", "Oslo", "Nice", "Giza", "Lyon", "Bern", "Gaza", "Xian"};

// Array to hold random chosen room indices
int randomRooms[7];

// Function prototypes
bool IsGraphFull();
void AddRandomConnection();
struct Room* GetRandomRoom();
bool CanAddConnectionFrom(struct Room*);
void ConnectRoom(struct Room*, struct Room*);
bool IsSameRoom(struct Room*, struct Room*);
void AssignNames();
void Randomize();
bool AlreadyConnected(struct Room*, struct Room*);
void writeFiles(char*);


int main(){
    // Seed random generator
    srand(time(NULL));

    // Get current process ID
    int processID = getpid();
    
    // Determine length of future directory name
    // 16 is the prefix quesenbt.rooms. + 1 for a null terminator
    int dirLength = 16 + sizeof(processID);

    // Assign char array for directory name
    char* dirName = malloc(dirLength * sizeof(char));
    memset(dirName, '\0', dirLength);

    // Combine process id with prefix and assign to dirName
    int ret = snprintf(dirName, dirLength, "quesenbt.rooms.%d", processID);

    // Create a new directory
    ret = mkdir(dirName, 0755);

    // Randomly choose Room names
    Randomize();
    // Initialize rooms and assign names
    AssignNames();

    // Create all connections in the graph
    while(IsGraphFull() == false){
        AddRandomConnection();
    }

    // Write room data to files
    writeFiles(dirName);

    //Free struct names that were dynamically allocated in AssignNames
    int i;
    for(i = 0; i < TR; i++){
        free(rooms[i].name);
    }

    // Free malloc string
    free(dirName);      
    return 0;
}





// Returns true if all rooms have 3+ outboud connections, false otherwise
bool IsGraphFull(){
    bool full = true;
    int i;
    for(i = 0; i < TR; i++){
        if(rooms[i].totalConnections < 3){
            full = false;
        }
    }

    return full;
}

// Adds a random, valid connection from a Room to another Room
void AddRandomConnection(){
    struct Room* A;
    struct Room* B;

    while(true){
        A = GetRandomRoom();

        if(CanAddConnectionFrom(A) == true){
            break;
        }
    }

    do {
        B = GetRandomRoom();
    }while(CanAddConnectionFrom(B) == false || IsSameRoom(A, B) == true || AlreadyConnected(A, B) == true);
 
    ConnectRoom(A, B);
    ConnectRoom(B, A);

    return;
}

// Returns a random Room, does NOT validate if connection can be added
struct Room* GetRandomRoom(){
    struct Room* A;
    int r = rand() % 7;
    A = &rooms[r];
    return A;
}

// Returns true if a connection can be added from Room x, false otherwise
bool CanAddConnectionFrom(struct Room* x){
    bool can;
    if(x->totalConnections < 6){
        can = true;
    }else{
        can = false;
    }
    return can;
}

// Connects Rooms x and y together, does not check if this connection is valid
void ConnectRoom(struct Room* x, struct Room* y){
    x->outboundRoom[x->totalConnections] = y;
    x->totalConnections += 1;
    return;
}

// Returns true if Rooms x and y are the same Room, false otherwise
bool IsSameRoom(struct Room* x, struct Room* y){
    bool same;
    if(x->id == y->id){
        same = true;
    }else{
        same = false;
    }
    return same;
}

// Checks if the 2 rooms are already linked together
bool AlreadyConnected(struct Room* x, struct Room* y){
    bool linked = false;
    int i;
    // Loop through current connections
    for(i = 0; i < x->totalConnections; i++){
        // Change to true if room x already has connection to room y
        if(x->outboundRoom[i]->id == y->id){
            linked = true;
        }
    }

    return linked;
}


// Assign room names, no duplicate names, run after Randomize()
void AssignNames(){
    int i;
    // For each room assign id, set intial connections,  allocate space and assign name
    for(i = 0; i < TR; i++){
        rooms[i].id = i;
        rooms[i].totalConnections = 0;
        rooms[i].name = calloc(5, sizeof(char));
//        memset(rooms[i].name, '\0', 5);
        strcpy(rooms[i].name, roomNames[randomRooms[i]]);
    }
    return;
}

// Get 7 unique indices for naming, stored in global array randomRooms
void Randomize(){
    int i;
    
    // Initialize indices to 0
    for(i = 0; i < TR; i++){
        randomRooms[i] = 0;
    }

    int r;
    int j;
    bool used;

    // For each room index
    for(i = 0; i < TR; i++){

        do{
            // Get a random index from 0 to 9
            r = rand() % 10;
            used = false;

            // Check if that index has already been used
            for(j = 0; j < TR; j++){
                if(randomRooms[j] == r){
                    used = true;
                }
            }
        }while(used == true); // Repeat until unused index chosen

        // Assign the random index to current position in array
        randomRooms[i] = r;
    }

    return;
}


// Print room data to individual files
void writeFiles(char * dirName){
    
    // Hold name for files to write to
    char file[50];
    int i;
    int j;
    int k;

    // For each room
    for(i = 0; i < TR; i++){
        // Clear file path
        memset(file, '\0', 50);
        // Get new file path with room name as file name
        snprintf(file, 50, "%s/%s", dirName, rooms[i].name); 
        // Open that new file
        FILE* input = fopen(file, "w");
        // Print room name to its file
        fprintf(input, "ROOM NAME: %s\n", rooms[i].name);

        // Print connections for the current room to its file        
        k = 1;
        for(j = 0; j < rooms[i].totalConnections; j++){
            fprintf(input, "CONNECTION %d: %s\n", k, rooms[i].outboundRoom[j]->name);
            k++;
        }

        // Print room type to file depending on position in array
        // First index will be start room but name and connections will
        // change each run so it is essentially randomized. Last index
        // is end room and and others are mid rooms.
        if(i == 0){
            fprintf(input, "ROOM TYPE: START_ROOM");
        }else if(i == TR - 1){
            fprintf(input, "ROOM TYPE: END_ROOM");
        }else{
            fprintf(input, "ROOM TYPE: MID_ROOM");
        }

        // Close current file
        fclose(input);
    }
}
