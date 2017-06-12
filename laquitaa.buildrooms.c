#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define numRooms 7 //the number of rooms we are dealing with.

//Struct that holds all important room data
struct Room {
	int roomStatus;		//if its a start middle or end 1 = start , 0 = end, 3 = middle
	int connIdx[6];	// up to 6 possible connecting rooms.
	int connections; //Numer of connections
	int name;	//randomly selected name from roomNames
};

//Function takes a struct room as an arguement and assigns it a non repeating integer value x.  0 < x < numRooms
//in order for the names to be randomized.
void randomName (struct Room* rooms) {
  int i, j, r, found, arr[numRooms], count1=0, count2=0;
      
	for (i=0; i< numRooms; i++){  //loop through the rooms
		 if (count1 == 0) {//first element in room gets assigned random value
		arr[count1++] = rand() % 10;
		rooms[count2++].name = rand() % 10; 
	  }

	  do { 
		r = rand() % 10;
		found = 1;
	
		for(j = 0 ; j < numRooms; j++) { //loop to see if we have a match!
		  if (arr[j] == r) {
			found = 0;
		  }
		}

	  } while (found == 0); //run r as a random number until there isn't a match
	  arr[count1++] = r;
	  rooms[count2++].name = r; //if it isn't a duplicate, it will get stored in room
	}
}

//Function takes a room struct as an argument and assigns a the room status for every room.  1 = start, 0 = end, 3 = middle
void assignStatus(struct Room* rooms){
	srand(time(NULL));
	int start = rand() % numRooms;  //starting room random index
	int end = rand() % numRooms;	//ending room random index
	int i;
	while (end == start) { //you can't start where you end silly
		end = rand() % numRooms; 
	}
	//assign start and ending rooms
	rooms[start].roomStatus = 1;
	rooms[end].roomStatus = 0;
	//If you're not first or last youre.... IN THE MIDDLE!
	for(i=0; i<numRooms; i++){
		if (i != start && i !=end){  
			rooms[i].roomStatus = 3; 
		}
	}
	return;
}

/*Function returns a name at postion pos.  I'm using this like a hash map without the hashing function
the int value in the rooms struct will correspond to the string at pos in the roomNames array.*/
char* getName(char* rooms[], int pos)
{
	return rooms[pos];
}

//Function takes an array of rooms and assigns (hallways) connections between the rooms
void makeConnections(struct Room* rooms)
{
	srand(time(NULL));
	int i = 0, j = 0, c=0;
	
	//zero out all connections.  The compiler was yelling at me for not doing this before.  Prevents seg faults.
	for (i=0; i <numRooms; i++)
	{
		rooms[i].connections = 0;
	}

	//Loop through all the rooms
	for(i = 0; i < numRooms; i++ )
	{
		 while (rooms[i].connections < 3)
		{
			int unique = 0;			//flag to determine if connection is unique
            int r;
            while (unique == 0) {  
                r = rand() % 7;     // pick a random room out of the 7.
                if (r == i) {       // a room cannot be connected to itself
                    continue;
                }
				 unique = 1;         
                int k;
                for (k = 0; k < 6; k++) {  // check to ensure connection hasn't already been made
                    if (rooms[i].connIdx[k] == r) {
                        unique = 0;
                        break;
					}
				}
			}
			//connect the dots... errr rooms.
			rooms[i].connIdx[rooms[i].connections] = r;
            rooms[i].connections++;
            rooms[r].connIdx[rooms[r].connections] = i;
            rooms[r].connections++;
		}
	}
	//start the process over again to see if a room has more than 3 but less than 7 connections
	for (i = 0; i < numRooms; i++) {
        int n = rand() % 3;    //50% odds of creating a new connection
        while (n == 0 && rooms[i].connections < 6) {  
            //the rest follow the same pattern as above
            int unique = 0;
            int r;
            while (unique == 0) { 
                r = rand() % 7;    
                if (r == i) {
                    continue;  
                }
                unique = 1;
                int k;
                for (k = 0; k < 6; k++) {  
                    if (rooms[i].connIdx[k] == r) {
                        unique = 0;
                        break;
                    }
                }
            } 
            rooms[i].connIdx[rooms[i].connections] = r;
            rooms[i].connections++;
            rooms[r].connIdx[rooms[r].connections] = i;
            rooms[r].connections++;
            n = rand() % 3 + i;   // decrease the odds of making connection next iteration.
        }
    }
	return;
}

//Main: go ahead and do your thing.
int main()
{
	srand(time(NULL));
	struct Room rooms[numRooms];
	char* roomNames[] = { "Bedroom", "livingRoom", "Kitchen", "Bathroom", "DiningRoom", "Basement", "Attic", "Den", "garage", "backYard" };
	int nameArr[numRooms];
	int i = 0, j = 0, pos = 2;
	
	randomName(rooms);		//Assign random name value to rooms
	assignStatus(rooms);	//determine if room is start end or middle
	makeConnections(rooms); //Assign room connections
	char fileBuffer[1024];
	
	//FILE *outFile;
	char* name, *name2;
	//create directory for room files and write out contents.
	for (i = 0; i < numRooms; i++) {
		name = getName(roomNames, i);
		sprintf(fileBuffer, "laquitaa.rooms.%d/", getpid());
        char filename[512];
        sprintf(filename, "%s%s", fileBuffer, name);
        mkdir(fileBuffer, S_IRWXU | S_IRWXG | S_IRWXO);

        // create file names for each room name
        FILE *file;
        file = fopen(filename, "w");

        // Write name to file
        fprintf(file, "ROOM NAME: %s\n", name);

        // Write connections to file
        for (j = 0; j < rooms[i].connections; j++) { 
			name2 = getName(roomNames, rooms[i].connIdx[j]);
			fprintf(file, "CONNECTION %i: %s\n", j+1, name2);
        }
         int b = rooms[i].roomStatus; 
		//write roomStatus to file
        if (b == 1) {
            fprintf(file, "ROOM TYPE: START_ROOM\n");
        } 
        else if (b == 0) {
            fprintf(file, "ROOM TYPE: END_ROOM\n");
        } 
        else {
            fprintf(file, "ROOM TYPE: MID_ROOM\n");
        } 
		//close the file and be done with it.
        fclose(file);
    }
	return 0;
  }