/***********************************************************************
 * Author: Patrick Kilgore
 * Date: 17 Oct 2017
 * Description: Generates a series of files describing various
 *  connected rooms to be used by a text-based adventure game.
 *********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define NUM_ROOMS 7

typedef enum {false, true} bool;
typedef enum {START_ROOM, MID_ROOM, END_ROOM} RoomType;

struct room
{
    int id;
    char name[10];
    int numOutboundConnections;
    struct room* outboundConnections[6];
    RoomType rType;
};
typedef struct room Room;

// Function declarations
bool IsGraphFull(Room roomGraph[], int numRooms);
void AddRandomConnection(Room roomList[], int numRooms);
Room* GetRandomRoom(Room roomList[], int numRooms);
bool CanAddConnectionFrom(Room* x);
bool ConnectionAlreadyExists(Room* x, Room* y);
void ConnectRoom(Room* x, Room* y);
bool IsSameRoom(Room* x, Room* y);
void RoomNameListShuffle(char list[][10], size_t n);
void WriteRoomFiles(Room roomList[], int listSize);
char* RoomTypeString(RoomType x);

// Program main entry point
int main()
{
    // set random seed using system clock
    srand((unsigned) time(0));

    // 10 room names needed, longest name is 9 characters, add 1 for \0
    char roomNames[10][10];
    strcpy(roomNames[0], "Altuve");
    strcpy(roomNames[1], "Beltran");
    strcpy(roomNames[2], "Bregman");
    strcpy(roomNames[3], "Correa");
    strcpy(roomNames[4], "Gattis");
    strcpy(roomNames[5], "Gonzalez");
    strcpy(roomNames[6], "Gurriel");
    strcpy(roomNames[7], "Keuchel");
    strcpy(roomNames[8], "Springer");
    strcpy(roomNames[9], "Verlander");

    // Shuffle the list to get a random set for building rooms
    RoomNameListShuffle(roomNames, 10);

    // Build array of 7 rooms using first 7 entries in room name list
    Room roomsList[NUM_ROOMS];
    int i;
    for (i = 0; i < NUM_ROOMS; i++)
    {
        roomsList[i].id = i;                        // set id
        strcpy(roomsList[i].name, roomNames[i]);    // set name
        roomsList[i].numOutboundConnections = 0;    // initialize connections cound

        // set room 0 to start, room 6 to end, all other mid
        if (i == 0)
            roomsList[i].rType = START_ROOM;
        else if (i == NUM_ROOMS - 1)
            roomsList[i].rType = END_ROOM;
        else
            roomsList[i].rType = MID_ROOM;
    }

    // Generate room connections, 3 min, 6 max
    while (IsGraphFull(roomsList, NUM_ROOMS) == false)
    {
        AddRandomConnection(roomsList, NUM_ROOMS);
    }

    // Build room description files in separate directory
    // First create the directory for the room files
    char roomDirName[32];
    sprintf(roomDirName, "kilgorep.rooms.%ld", (long)getpid());
    int mkdirSuccess = mkdir(roomDirName, 0755);
    if (mkdirSuccess != 0)  // mkdir failed!!
    {
        printf("Failed to create directory for room files.\n");
        return 0;
    }

    // Navigate to the room files directory
    chdir(roomDirName);

    // Start generating the room files in the directory
    WriteRoomFiles(roomsList, NUM_ROOMS);

    // Change back to executable directory
    chdir("..");

    return 0;
}

// Check each room to see if they all have at least 3 outbound connections
bool IsGraphFull(Room roomGraph[], int numRooms)
{
    bool isFull = true;
    int i;

    // Loop through rooms, checking for rooms that have fewer than 3 connections
    // If one is found, return false
    for (i = 0; i < numRooms; i++)
    {
        if (roomGraph[i].numOutboundConnections < 3)
        {
            isFull = false;
            return isFull;
        }
    }

    // All rooms have at least 3 connections, so return true
    return isFull;
}

// Attempt to connect two random rooms
void AddRandomConnection(Room roomList[], int numRooms)
{
    Room* A;
    Room* B;

    while (true)
    {
        // Choose random room & check if it has less than 6 connections
        A = GetRandomRoom(roomList, numRooms);
        if (CanAddConnectionFrom(A) == true)
        {
            break;
        }
    }

    do
    {
        // Choose random room that:
        //      has less than 6 connections
        //      is not room A
        //      is not already connected to A
        B = GetRandomRoom(roomList, numRooms);
    } while (CanAddConnectionFrom(B) == false || IsSameRoom(A, B) == true ||
            ConnectionAlreadyExists(A, B) == true );

    // Make both room connections
    ConnectRoom(A, B);
    ConnectRoom(B, A);
}

// Choose a random room from the room list
Room* GetRandomRoom(Room roomList[], int numRooms)
{
    // Generate random room index in range 0 - 6
    int randIndex = rand() % (numRooms);

    // Return the address of that room's struct
    return &(roomList[randIndex]);
}

// Check to make sure the room hasn't maxed its outbound connections
bool CanAddConnectionFrom(Room* x)
{
    // Does Room x have fewer than 6 connections?
    if (x->numOutboundConnections < NUM_ROOMS - 1)
        return true;
    else
        return false;
}

// Check through a room's connections list for another room
bool ConnectionAlreadyExists(Room* x, Room* y)
{
    bool connectionExists = false;

    int i;

    // If the room has connections
    if (x->numOutboundConnections > 0)
    {
        // Loop through x's connections
        for (i = 0; i <= x->numOutboundConnections; i++)
        {
            // Does a particular connection point to y?
            if (x->outboundConnections[i] == y)
            {
                connectionExists = true;
                break;
            }
        }
    }

    return connectionExists;
}

// Add a connection to Room y in Room x's connection list
void ConnectRoom(Room* x, Room* y)
{
    // current value of numOutboundConnections is the index where the new link will be added
    x->outboundConnections[x->numOutboundConnections] = y;

    // increment connection cound
    ++(x->numOutboundConnections);
}

// Check if two Room pointers are pointing to the same room
bool IsSameRoom(Room* x, Room* y)
{
    if (x == y)
        return true;
    else
        return false;
}

/* Shuffles the list of room names into a random ordering
   Source: https://stackoverflow.com/questions/6127503/shuffle-array-in-c */
void RoomNameListShuffle(char list[][10], size_t n)
{
    if (n > 1)
    {
        size_t i;

        // Loop forward through elements of list array
        for (i = 0; i < n-1; i++)
        {
            // Choose a random element after the ith element
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            char tmp[10];
            // Swap elements i and j
            strcpy(tmp, list[j]);
            strcpy(list[j], list[i]);
            strcpy(list[i], tmp);
        }
    }
}

// Write all the room descriptions to individual files
void WriteRoomFiles(Room roomList[], int listSize)
{
    FILE* fp;
    const char * const fileNames[] = {"room0", "room1", "room2", "room3",
        "room4", "room5", "room6"};
    int i;
    int j;

    // Loop through elements of roomList
    for (i = 0; i < listSize; i++)
    {
        // Open ith output file
        fp = fopen(fileNames[i], "w");
        // Write room name to file
        fprintf(fp, "ROOM NAME: %s\n", roomList[i].name);

        // Loop through writing connections
        for (j = 0; j < roomList[i].numOutboundConnections; j++)
        {
            fprintf(fp, "CONNECTION %d: %s\n", j + 1, roomList[i].outboundConnections[j]->name);
        }

        // Write room type to output file
        fprintf(fp, "ROOM TYPE: %s\n", RoomTypeString(roomList[i].rType));

        // Close ith output file
        fclose(fp);
    }
}

// Converts a RoomType value to its string equivalent
char* RoomTypeString(RoomType x)
{
    if (x == START_ROOM)
        return "START_ROOM";
    else if (x == MID_ROOM)
        return "MID_ROOM";
    else if (x == END_ROOM)
        return "END_ROOM";
    else
        return "";
}
