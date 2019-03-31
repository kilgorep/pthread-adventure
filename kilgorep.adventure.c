/***********************************************************************
 * Author: Patrick Kilgore
 * Date: 23 October 2017
 * Description: A simple text-based dungeon crawler using prebuilt
 *              dungeon rooms. Also demonstrates concurrency with a
 *              time keeping function.
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define NUM_ROOMS 7

typedef enum {false, true} bool;
typedef enum {START_ROOM, MID_ROOM, END_ROOM} RoomType;

struct room
{
    int id;
    char name[20];
    int numDoors;
    struct room* doors[6];
    RoomType rt;
};
typedef struct room Room;

// Function Declarations
void BuildDungeon(Room dungeon[]);
void GetRoomsDirectoryName(char dirName[]);
void ParseRoomFile(FILE* fp, Room* roomStruct);
RoomType GetRoomTypeFromString(char rts[]);
void ParseRoomConnections(FILE* fp, Room dungeon[], int roomIndex);
Room* GetRoomPtrFromName(char rName[], Room dungeon[]);
void PlayGame(Room dungeon[]);
void ShowUserPrompt(Room dungeon[], Room* location);
bool GetUserResponse(char uEntry[], Room dungeon[], Room* location);
void* WriteTimeFile();

// Declare mutex
pthread_mutex_t squirrel;

// Program main entry point
int main()
{
    // Declare array of rooms to store dungeon layout
    Room dungeon[NUM_ROOMS];

    // Build dungeon
    BuildDungeon(dungeon);

    // Begin game loop
    PlayGame(dungeon);

    return 0;
}

// Populates the dungeon room array with data from the newest rooms
// files directory
void BuildDungeon(Room dungeon[])
{
    // find the newest directory of room files
    char roomsDir[256];
    memset(roomsDir, '\0', sizeof(roomsDir));
    GetRoomsDirectoryName(roomsDir);

    // change working directory to selected subdirectory
    chdir(roomsDir);

    // read files into dungeon array elements
    int i;
    FILE* fp;
    char roomFile[6];
    for (i = 0; i < NUM_ROOMS; i++)
    {
        dungeon[i].id = i;
        dungeon[i].numDoors = 0;

        // filenames are room0, room1, etc.
        sprintf(roomFile, "room%d", i);
        fp = fopen(roomFile, "r");
        ParseRoomFile(fp, &(dungeon[i]));
        fclose(fp);
    }
    // Connect rooms
    for (i = 0; i < NUM_ROOMS; i++)
    {
        sprintf(roomFile, "room%d", i);
        fp = fopen(roomFile, "r");
        ParseRoomConnections(fp, dungeon, i);
    }

    // return to executable directory
    chdir("..");
}

// Store the name of the newest rooms directory in the passed string var
// Code based on 2.4 Manipulating Directories reading
void GetRoomsDirectoryName(char dirName[])
{
    int newestDirTime = -1;     // time stamp for newest directory
    char targetDirPrefix[32] = "kilgorep.rooms";
    char newestDirName[256];    // holds name of newest directory
    memset(newestDirName, '\0', sizeof(newestDirName));

    DIR* dirToCheck;            // holds starting directory
    struct dirent *fileInDir;   // holds currecnt subdir
    struct stat dirAttributes;  // holds info about subdir

    dirToCheck = opendir(".");  // open executable directory

    if (dirToCheck > 0)         // make sure it can be opened
    {
        // check each subdirectory in executable directory
        while ((fileInDir = readdir(dirToCheck)) != NULL)
        {
            // search for prefix in directory name
            if (strstr(fileInDir->d_name, targetDirPrefix) != NULL)
            {
                // get stats on current subdirectory
                stat(fileInDir->d_name, &dirAttributes);
                // subdirectory has newest mod date?
                if (dirAttributes.st_mtime > newestDirTime)
                {
                    // update newest mod time
                    newestDirTime = (int)dirAttributes.st_mtime;
                    // store subdirectory name
                    memset(newestDirName, '\0', sizeof(newestDirName));
                    strcpy(newestDirName, fileInDir->d_name);
                }
            }
        }
    }

    // close the directory
    closedir(dirToCheck);
    // copy last modified subdirectory name to function argument string
    strcpy(dirName, newestDirName);
}

// Reads a room definition file and assigns name and room type to struct
// Note: connections will be made in another function
void ParseRoomFile(FILE* fp, Room* roomStruct)
{
    char curLine[80];
    char roomName[20];
    char roomType[20];
    RoomType rType;

    // Clear string contents
    memset(curLine, '\0', sizeof(curLine));
    memset(roomName, '\0', sizeof(curLine));
    memset(roomType, '\0', sizeof(curLine));

    // Read the room name
    fgets(curLine, sizeof(curLine), fp);
    sscanf(curLine, "ROOM NAME: %s\n", roomName);

    // Skip through connections
    while (fgets(curLine, sizeof(curLine), fp))
    {
        if (strstr(curLine, "ROOM TYPE") != NULL)
            break;
    }
    // Read the room type
    sscanf(curLine, "ROOM TYPE: %s\n", roomType);
    rType = GetRoomTypeFromString(roomType);

    // Assign values to struct
    strcpy(roomStruct->name, roomName);
    roomStruct->rt = rType;
}

// Converts a room type string to the matching enum value
RoomType GetRoomTypeFromString(char rts[])
{
    if (strcmp(rts, "START_ROOM") == 0)
        return START_ROOM;
    else if (strcmp(rts, "MID_ROOM") == 0)
        return MID_ROOM;
    else
        return END_ROOM;
}

// Reads through room files to build connections / doors to other rooms
void ParseRoomConnections(FILE* fp, Room dungeon[], int roomIndex)
{
    int conIndex = 0;
    char curLine[80];
    char doorRoom[80];
    Room* door;
    memset(curLine, '\0', sizeof(curLine));
    memset(doorRoom, '\0', sizeof(doorRoom));

    // Skip name line
    fgets(curLine, sizeof(curLine), fp);

    // Loop through connections
    // Does two extra loops for room type line and EOF
    while (fgets(curLine, sizeof(curLine), fp))
    {
        if (strstr(curLine, "CONNECTION") != NULL)
        {
            // Line describes a room connection, so store the connected room name
            sscanf(curLine, "CONNECTION %*d: %s\n", doorRoom);
            // Get a pointer to that room
            door = GetRoomPtrFromName(doorRoom, dungeon);
            // Build the connection and increment counters
            dungeon[roomIndex].doors[conIndex] = door;
            conIndex++;
            (dungeon[roomIndex].numDoors)++;
        }
    }
}

// Returns a pointer to the dungeon room named rName
Room* GetRoomPtrFromName(char rName[], Room dungeon[])
{
    int i;
    for (i = 0; i < NUM_ROOMS; i++)
    {
        // Looping through dungeon room structs, is the room name
        // the same as the one we're looking for?
        if (strcmp(rName, dungeon[i].name) == 0)
        {
            // Return a pointer to the matched room
            return &(dungeon[i]);
        }
    }

    // rName not found, so return a null pointer
    return NULL;
}

// Main loop for execution of the dungeon game
void PlayGame(Room dungeon[])
{
    Room* location;
    Room* destination = NULL;
    bool entrySuccess;
    FILE* dPath;            // stores names of rooms entered
    int moves = 0;          // stores number of steps taken
    pthread_t timeThread;   // thread for the timekeeping function
    int threadResult;       // stores result of pthread_create
    FILE* timeData;         // time data file created by timekeeping thread
    char timeString[150];   // current time data string
    char response[200];     // user's action entry string

    // Place player in start room, which is always dungeon[0]
    location = &(dungeon[0]);

    // Open file for storing path taken
    dPath = fopen("path.txt", "w");

    // Lock mutex and kick off the time keeping thread
    pthread_mutex_init(&squirrel, NULL);
    pthread_mutex_lock(&squirrel);
    threadResult = pthread_create(&timeThread, NULL, WriteTimeFile, NULL);

    while (location->rt != END_ROOM)
    {
        // Display game prompt
        ShowUserPrompt(dungeon, location);

        // Get response & validate
        memset(response, '\0', 200);
        entrySuccess = GetUserResponse(response, dungeon, location);

        // Update location and move count
        if (entrySuccess)
        {
            if (strcmp(response, "time") != 0)
            {
                location = GetRoomPtrFromName(response, dungeon);
                fprintf(dPath, "%s\n", response);      // store name in path file
                moves++;
            }
            // Time was requested
            else
            {
                // Clear data from previous timeString string
                memset(timeString, '\0', 150);
                
                // Release the squirrel to let the time thread proceed
                pthread_mutex_unlock(&squirrel);

                // Wait a bit and try mutex lock, block thread until unlocked
                pthread_mutex_lock(&squirrel);

                // Other thread has run and unlocked mutex, so time file is now present
                // Open it for reading
                timeData = fopen("currentTime.txt", "r");

                // Store the file data in the string timeString
                fgets(timeString, 150, timeData);
                fclose(timeData);

                // Write the time data to the screen
                printf("\n%s", timeString);
            }
        }
    }

    fclose(dPath);

    // End of dungeon found, so write victory messages
    printf("\nYOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", moves);

    // Show the path taken
    // First reopen the path file for reading
    dPath = fopen("path.txt", "r");
    char pRoom[20];
    // Now loop through the file lines
    while (fgets(pRoom, 20, dPath))
    {
        printf("%s", pRoom);
    }
    // Close and delete path file
    fclose(dPath);
    remove("path.txt");
}

// Display a prompt to the user to select a room to travel to
void ShowUserPrompt(Room dungeon[], Room* location)
{
    // Print line telling current location
    char locName[20];
    strcpy(locName, location->name);
    printf("\nCURRENT LOCATION: %s\n", locName);

    int i;

    // Build connections string
    char doorString[200];
    strcpy(doorString, "POSSIBLE CONNECTIONS: ");
    for (i = 0; i < location->numDoors; i++)
    {
        // Add each door name to the string
        strcat(doorString, location->doors[i]->name);

        // Add appropriate punctuation
        if (i == location->numDoors - 1)
            strcat(doorString, ".\n");
        else
            strcat(doorString,", ");
    }

    // Print connections
    printf("%s", doorString);
    printf("WHERE TO? >");
}

// Gets response from stdin and validates result
bool GetUserResponse(char uEntry[], Room dungeon[], Room* location)
{
    int i;

    // Get user input

        fgets(uEntry, 200, stdin);
        // Strip trailing \n
        // Taken from https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
        uEntry[strcspn(uEntry, "\n")] = 0;

        // Check if the name is a valid room
        if (GetRoomPtrFromName(uEntry, dungeon) != NULL)
        {
            // Check if entered name is connected to location
            for (i = 0; i < location->numDoors; i++)
            {
                if (strcmp(uEntry, location->doors[i]->name) == 0)
                {
                    return true;
                }
            }
        }

        // Check if user requested the time
        if (strcmp(uEntry, "time") == 0)
        {
            return true;
        }

        // Otherwise print error message
        printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
        return false;
}

// Write the current date and time to a text file
void* WriteTimeFile()
{
    FILE* tf;               // pointer to currentTime.txt
    time_t now;             // current system time
    struct tm* strNow;      // time.h struct for formatting system time
    char curTime[150];      // string buffer to hold time info

    // Start infinite time keeping loop
    while(true)
    {
        // Try to grab the squirrel
        pthread_mutex_lock(&squirrel);
        
        // Start work on building the time file
        tf = fopen("currentTime.txt", "w");

        // Get the current time
        now = time(NULL);

        // Convert to usable time units in local timezone
        strNow = localtime(&now);

        // Get formatted time string
        memset(curTime, '\0', 150);
        strftime(curTime, 150, "%I:%M %p, %A, %B %d, %Y", strNow);

        // Write string to file and close file
        fprintf(tf, "%s\n", curTime);
        fclose(tf);

        // All done, so release the lock, er, I mean squirrel
        pthread_mutex_unlock(&squirrel);
    }
}
