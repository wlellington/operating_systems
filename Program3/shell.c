/* Wesley Ellington
 * Operating Systems
 * Program 3
 * 9 March 2018 */

// Compiled using:
// gcc -pthread shell.c -o shell


/**********************************
 **** The Shell Of Disapproval ****
 **********************************/

// A simple shell written as a wrapper of system calls
// Uses a few threaded functions, so compile using the
// -pthread compiler flag

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Loop state for exit command
int loop = 1;

// prompt string
char * prompt = "(ಠ_ಠ) >>> ";
char * cmd_args[3];


// Environment variables
extern char **environ;
char ** env;

// Known command strings
int num_cmds = 7;
char known_cmds[10][10] = {
	"",
	"clr",
	"dir",
	"environ",
	"quit",
	"frand",
	"fsort",
	};

// Number of expected arguments
int known_cmd_args[10] = {
	0,
	0,
	0,
	0,
	0,
	2,
	1,
	};

// Main parsing loop function
int input_loop();

// Checks if using shell defined function, returns 1 if so
int check_function(char* command);

//Get number of expected arguments for function
int check_num_args(int cmd_num);

// Runs shell defined function
int shell_function(int cmd_num, char* args);

// Runs system called function 
int system_function();

// Creates file filled with random ints
int frand(char* file, char* size);

// Sorts contents of file
int fsort(char* file);

// Main function
int main(int argc, char * argv[]){
	fprintf(stdout, "Initializing...\n");
	
	// Set up environment variables
	
	// Begin loop
	input_loop();

	return 0;
}

// Main parsing loop function
int input_loop(){
	char* input_buf = NULL;
	size_t len;
	int read;
	char split_str[450];
	char* token = NULL;
	char command[100];
	int num_params = 0;

	// Run until quit keyword
	while(loop){
		// Display prompt
		fprintf(stdout, "%s" ,prompt);
		
		// Get line of input on enter press
		read = getline(&input_buf, &len, stdin);
		fflush(stdout);

		// If line is grabbed, begin parse
		if(-1 != read){
			// Reset param count
			num_params = 0;
			
			// Copy original string to tmp
			strcpy(split_str, input_buf);
			
			// Get token for first word
			token = strtok(split_str, " \n\0");
			
			// Copy command word to buffer
			strcpy(command, split_str);
			
			int cmd_num = 0;
			
			// Check if known command
			cmd_num = check_function(command);
			
			// If known, parse for arguments and pass to handler
			if(cmd_num != -1){
				// Parse arguments
				while(token != NULL){
					token = strtok(NULL, " \n\0");
					if(token == NULL){
						break;
					}
					// Save arguments to array for later use
					cmd_args[num_params] = token; 
					num_params ++;
				}
				// Check for valid number of input args
				if(num_params == check_num_args(cmd_num)){
					shell_function(cmd_num, token);
				}
				else{
					fprintf(stderr,"Invalid Arguments!\n");
				}
			}
			// Else pass to system handler
			else{
				system_function(input_buf);
			}
		}
	}

	return 0;
}

//Check if command is in list of shell commands, return index in table
int check_function(char* command){
	for(int i = 0; i < num_cmds; i++){
		if(0 == strcmp(command, known_cmds[i])){
			return i;
		}
	}
	
	return -1;
}

//Get number of expected arguments for function
int check_num_args(int cmd_num){
	return known_cmd_args[cmd_num];
}

// Runs system called function 
int system_function(char* command){
	return system(command);
}

// Runs shell defined function, handler for built in functionality
int shell_function(int cmd_num, char* args){
	int status = 0;
	switch(cmd_num){
		// No input
		case 0:
			break;
		// clr
		case 1:
			status = system("clear");
			break;
		// dir
		case 2:
			status = system("ls -la");
			break;
		// environ
		case 3:
			env = environ;
			while (*env) fprintf(stdout, "%s\n",*env++);
			break;
		// Quit	
		case 4:
			loop = 0;
			exit(0);
			break;
		// frand
		case 5:
			frand(cmd_args[0], cmd_args[1]);
			break;
		// fsort
		case 6:
			fsort(cmd_args[0]);
		default:
			break;

	}
	return status;
}

// Creates file filled with random ints on child proc
int frand(char* file, char* size){	
	// Fork to create child process
	int fork_result = fork();
	
	// Ensure child was spawned
	if(fork_result < 0){
		fprintf(stderr, "No child spawned!\n");
		exit(1);
	}

	//Parent
	if(fork_result != 0){
		// Return to prompt
		return 0;
	}
	// Child
	else{
		// Open file and ensure creation/ opened properly
		FILE *fp = fopen(file, "w+");
		if(fp == NULL){
			fprintf(stderr, "File failed to open!\n");
			exit(1);
		}

		// Write size many values to file
		for(int i = 0; i < atoi(size); i++){
			fprintf(fp, "%i ", rand());	
		}
		// Close file
		fclose(fp);

		// Exit child
		exit(1);
	}
	return 0;
}

// Compare function for sort
int cmpfunc (const void * a, const void * b) {
	   return ( *(int*)a - *(int*)b );
}

// Sorts contents of file
int fsort(char* file){
	int pid = (int) getpid();
	int ppid = (int) getppid();

	int fork_result = fork();

	if(fork_result < 0){
		fprintf(stderr, "No child spawned!\n");
		exit(1);
	}

	//Parent
	if(fork_result != 0){
		// Exit to prompt
		return 0;
	}
	// Child
	else{
		FILE *fp = fopen(file, "r+");
		if(fp == NULL){
			fprintf(stderr, "File failed to open!\n");
			exit(1);
		}

		// Go to end of file, get size of file and reset pointer
		int end = 1;
		int size = 0;
		char buf[20];
		while(end){
			fscanf(fp, "%s", buf);
			size++;
			if(feof(fp)){
				end = 0;
			}
		}

		// Reset file pointer
		fseek(fp, 0, SEEK_SET);

		// Allocate array of ints
		unsigned int* mem_ints = malloc(sizeof(int) * size);
		
		// Check if memory properly allocated
		if(mem_ints == NULL){
			fprintf(stderr,"File not loaded to memory!\n");
			exit(1);
		}

		// Load file values to memory
		end = 1;
		for(int i = 0; i < size; i++){
			fscanf(fp, "%s", buf);
			mem_ints[i] = atoi(buf);
		}

		// Close file 
		fclose(fp);

		// Sort values 
		qsort(mem_ints, size, sizeof(int), cmpfunc);

		// Re open file in write mode
		fp = fopen(file, "w+");
		if(fp == NULL){
			fprintf(stderr, "File failed to open!\n");
			exit(1);
		}

		// Write values to file
		for(int i = 0; i < size; i++){
			fprintf(fp, "%i ", mem_ints[i]);
		}

		// Close file and exit
		fclose(fp);

		// Free number array memory
		free(mem_ints);
		exit(1);
	}
	return 0;
}

