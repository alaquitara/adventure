#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>

#define numRooms 7 //number of rooms in game

// Declare Mutex and thread handles, referenced: http://www.thegeekstuff.com/2012/05/c-mutex-examples/?refcom for usage
pthread_mutex_t locks[2];
pthread_t threads[2];

// struct used to store room information
struct room {
	char og[6][15];	//out-going connections
	int connections; //number of connections
	char name[13];  //room's name
	int roomStatus; //if its a start, middle, or end room
};

 /*function roomIdx takes a struct of rooms, a value, and number of rooms
 compare value to name of rooms and if there is a match return that index*/
int roomIdx(const struct room *rooms, const char *value, int roomNum) {
	int i = 0;
	while(i < roomNum){
		if (strcmp(rooms[i].name, value) == 0){ //http://www.cplusplus.com/reference/cstring/strcmp/
			return i;//we have  a winner! (a match).
		}
		i++;
	}
  return -1; //no match
}

/* function compValues takes a 2d array of connections, the value to compare -v and number of connections - b
 It compares values to name and retuns an index of the match*/
int compValues(char names[6][15], const char *v, int b) {
	int i = 0;
	while(i<b){
		if (strcmp(names[i], v) == 0){ //http://www.cplusplus.com/reference/cstring/strcmp/
			return i;//we have  a winner! (a match).
		}
		i++;
	}
return -1;//no match
}

 /*function writeTime uses a value -e as an ending flag
 time is locked until main opens it up via mutex and write time in currentTime.txt */
void* writeTime(void *e) {
	int *close = (int*)e; 
	FILE *file;             
	char michaelBuffer[50];      //Everyone's faroite boxing announcer... kidding its a file buffer
	time_t raw;             // http://en.cppreference.com/w/c/chrono/time_t
	struct tm *t;        		// http://www.cplusplus.com/reference/ctime/tm/
 
	while (!*close) { // until close is triggered
		pthread_mutex_lock(&locks[0]); //mutex locked
		// check flag
		if (*close){ //check flag
			break;
		}
		file = fopen("./currentTime.txt", "w");
		if (file == NULL) {
			perror("File Error\n");
		}
		// get the time and write it to currentTime.txt
		else {
			time ( &raw );
			// use raw time to fill struct tm
			t = localtime (&raw);
			// fill buffer with formatted current time
			strftime(michaelBuffer, 50, "%I:%M%p, %A, %B %d, %Y", t); //http://www.cplusplus.com/reference/ctime/strftime/
			michaelBuffer[5] = tolower(michaelBuffer[5]);
			michaelBuffer[6] = tolower(michaelBuffer[6]);
			// remove leading 0 from single digit times
				if (t->tm_hour < 10 || (t->tm_hour > 12 && t->tm_hour < 22)){
					memmove(michaelBuffer, michaelBuffer+1, strlen(michaelBuffer)); //http://www.cplusplus.com/reference/cstring/memmove/
				}
		fputs(michaelBuffer, file);
		fclose(file);
		}
	pthread_mutex_unlock(&locks[0]);  //http://www.ibm.com/support/knowledgecenter/ssw_ibm_i_73/apis/users_65.htm unlock it to prevent dedlock connection
    usleep(100); //https://linux.die.net/man/3/usleep
  }
  return NULL;
}

/*function readTime a value -e as an ending flag
 It reads the time and prints it when called on*/
void* readTime(void *e){
	int *close = (int*) e;  
	FILE *file; 	
	char michaelBuffer[50];
  
	while (!*close) {// until close is triggered
		pthread_mutex_lock(&locks[1]); //locked until main wants it
		if (*close){
			break; //if lock and flag is set
		}
		file = fopen("./currentTime.txt", "r"); //open and read currentTime.txt, let us know if we get an error
		if (file == NULL) {
			perror("Cannot open file\n");
		}
		//read time, print, and close
		else {
			fgets(michaelBuffer, 50, file);
			printf("\n%s\n", michaelBuffer);
			fclose(file);
		}
		// same as above functionk
		pthread_mutex_unlock(&locks[1]);
		usleep(50);
	}
  return NULL;
}

/* function getDirectory takes a directory  - dir and a buffer to write information -buf
using stats it finds the most recent buffer in the directory */
int getDirectory(const char *dir, char *buf) {
	int rec = -1, cur = 0;  // rec = newest directory, cur = current room time.
	struct stat statsI;  	// http://codewiki.wikidot.com/c:struct-stat
	DIR *direct;             // https://www.gnu.org/software/libc/manual/html_node/Opening-a-Directory.html
	struct dirent *dirEnt;	// stores directory entry
	// open directory and check for error
	direct = opendir(dir); 
	if (direct== NULL) {
		perror("Error: CANNOT OPEN DIRECTORY\n");
		return -1;
	}
	// read each directory entry
    while ((dirEnt = readdir(direct))) {
		if (strncmp(dirEnt->d_name, "laquitaa.room", 12) == 0) {
			stat(dirEnt->d_name, &statsI);
			cur = (int)statsI.st_mtime;          //gets most recent time
			//if the current is the most current then copy it to the buffer
			if (rec < cur) {                       		 
				strcpy(buf, dirEnt->d_name);   	
				rec = cur;                         // cur is maximum
			}
		}
	}
	closedir(direct);                         
	return rec;                              
}

 /*fuction gameData takes an array of rooms, a directory pointer -game, and an int pointer for the starting pointer
 This function reads laquitara.rooms.pid and fills in all pertinent information for the rooms struct.
 Main then uses this information to run the game */
int  gameData(struct room *rooms, const char *game, int *start) {
	int i = 0, count = 0;
	struct dirent *dirEnt;       //http://pubs.opengroup.org/onlinepubs/009695399/basedefs/dirent.h.html
	DIR *dir;                   // https://www.gnu.org/software/libc/manual/html_node/Opening-a-Directory.html
	FILE *file;                          
	char *noNew;
	char buffer[100];                                                 
	                   
	// zero out the out count for each room
	for (count = 0; count < numRooms; count++){
		rooms[count].connections  = 0;
	}
	count = 0;  //reset count because we just used it
	dir = opendir(game);               
	if (dir == NULL) {
		perror("ERROR: Failed to open directory\n");
		return -1;					//open failed return an error
	} 
 
	while ((dirEnt = readdir(dir))) { //read each file
		if (strlen(dirEnt->d_name) > 2){
			sprintf(buffer, "./%s/%s", game, dirEnt->d_name); //add the path to the buffer
			file = fopen(buffer, "r");
			if (file == NULL ) { 
				perror("Error: FILE DIDN'T OPEN\n"); return -1;
			}
			while(fgets(buffer, 100, file)!=NULL) { //read until the end of file and look for colons, the next value after the colon tells us what kind of data we are accessing
				noNew = strchr(buffer, '\n');  //no newlines in this buffer! http://www.cplusplus.com/reference/cstring/strchr/
				*noNew = '\0';
				noNew = strchr(buffer, ':');   //colon marks new important data from the rooms file
				i = noNew - buffer;
				if (strncmp(buffer, "ROOM NAME", i) == 0) {
					strcpy(rooms[count].name, noNew+2); //copy the name from count index
				}
				else if (strncmp(buffer, "CONNECTION", i-2) == 0) {
					strcpy(rooms[count].og[rooms[count].connections++], noNew+2); //copy the number of connections from count index
				} 
				else if (strncmp(buffer, "ROOM TYPE", i) == 0) { //copy the room type at count index
				strcpy(buffer, noNew+2);
					if (strcmp(buffer, "END_ROOM")==0){
						rooms[count].roomStatus = 0; //ending room
					}
					else if (strcmp(buffer, "START_ROOM") == 0) {
						rooms[count].roomStatus = 1;
						*start = count; //this is the starting room
					}
					else
						rooms[count].roomStatus = 3; //middle room
				}
			}
		fclose(file);
		count++;
    }
  }
  // close directory
  closedir(dir);
  return 0;
}

// This is where the magic happens
int main() {
	struct room *rooms = malloc(sizeof(struct room)*numRooms);  //cs261 taught us this oh so well
	int i = 0, count= 0, errCode = 0, cur, close = 0;
	int visit[150];  //keep track of the rooms we've visited, I hope it doesn't take 150 visits to finish.   
	char buffer[50]; 
	char *mikeBuffer = NULL, *c;                
	size_t size = 0;  
	//lock the mutex in main
	for (i = 0; i < 2; i++) {
		if(pthread_mutex_init(&locks[i], NULL) != 0){  
			printf("Error: Mutex failed %d\n", i+1);
			free(rooms);  //we failed miserably so we have to free up the heap
			return 1;	 //return error
		}
		pthread_mutex_lock(&locks[i]);
	}
	//thread is created to track time, follows the same logic as above
	errCode= pthread_create(&threads[0], NULL, &readTime, &close);
	if (errCode != 0) { //if pthread gets mad at us
		printf("Thread creation failed: %s\n", strerror(errCode));
		free(rooms);
		return 1;
	}
	//thread to read time and print it with printf, follows same logic as above
	errCode = pthread_create(&threads[1], NULL, &writeTime, &close);
	if (errCode != 0) {
		printf("Thread creation failed: %s\n", strerror(errCode));
		free(rooms);
		return 1;
	}
	//find the most recent directory of our room data 
	errCode = getDirectory("./", buffer);
	if (errCode == -1) {
		printf("Failed to locate room directory!\n");
		return 1;
	}
	
	//call gemeData to get all the information we need to fill in our rooms struct in order to play the game
	errCode = gameData(rooms, buffer, &cur);
	if (errCode == -1) {
		printf("Failed to read room file\n");
		return 1;
	}
	count++;
	visit[count] = cur; //the room we are currently in has been visited!

	do { 
		if (errCode != INT_MAX) {  //https://www.tutorialspoint.com/c_standard_library/limits_h.htm
			printf("CURRENT LOCATION: %s\n", rooms[cur].name);
			printf("POSSIBLE CONNECTIONS:");
			// print room connections
			for (i = 0; i < rooms[cur].connections; i++) {
				if (i+1 != rooms[cur].connections ){
				printf(" %s,", rooms[cur].og[i]); //a comma is inserted in-between room names
				}
				else
				printf(" %s.", rooms[cur].og[i]); //the last room name is special.  It gets a period.
			}
		}
    printf("\nWHERE TO? >");
    errCode= getline(&mikeBuffer, &size, stdin); //have the user tell us if they want to move rooms or get the time.
    c = strchr(mikeBuffer, '\n'); 
    *c = '\0';                    
    //if user asks for the time
    if (strcmp(mikeBuffer, "time") == 0) {
      pthread_mutex_unlock(&locks[0]);
      usleep(100); 
      pthread_mutex_lock(&locks[0]); //prevent write thread from looping
      pthread_mutex_unlock(&locks[1]);
      usleep(100); // pause in order to lock
      pthread_mutex_lock(&locks[1]); // lock it up again
      errCode = INT_MAX;  
    }
    //if user asks to move rooms but they don't enter a valid connection
    else if(compValues(rooms[cur].og, mikeBuffer, rooms[cur].connections)	< 0){
      printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
	}
    else { //They entered a valid room
      errCode = roomIdx(rooms, mikeBuffer, numRooms);
      if (errCode < 0) continue;
      cur = errCode; //current room is now the room they asked to enter
	  count++;
      visit[count] = errCode; // new room is added to list of visited rooms, just like above
      printf("\n"); 
    }
    // clear out everything so we can execute the loop again.
    free(mikeBuffer);
    mikeBuffer = NULL;
    size = 0;

	} while (rooms[cur].roomStatus != 0); //congratulations, the user found the end room. 0 being the ending

	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
	printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", count-1);

	for (i = 1; i < count; i++){
		printf("%s\n", rooms[visit[i]].name);  // Print every room visited e.g the path to victory
	}
	close = 1;
	pthread_mutex_unlock(&locks[0]);
	pthread_mutex_unlock(&locks[1]);

	// suspend execution of the calling thread until the target thread terminates http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_join.html
	for (i = 0; i < 2; i++){
		if (pthread_join(threads[i], NULL) != 0){
			perror("Failed Thread Join\n");
		}
	}
  
	// since they are joined we can now destroy them
	pthread_mutex_destroy(&locks[0]);
	pthread_mutex_destroy(&locks[1]);

	free(rooms);
	return 0;
}
