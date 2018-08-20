/* Wesley Ellington
 * Operating Systems
 * Program 3
 * 2 April 2018 */

// Compiled using:
// gcc -pthread shell.c -o shell

// USAGE:
// ./shell <x> <y>
// begins work with x number worker(consumers) with buffer of 
// size y
// If either value is zero or less, values default to 2 and 2 

// ** Extra Commands **
// runwork -  toggles on and off worker threads from handling tasks
// listwork - prints list of items in work buffer
// msgwork - prints thread activty when processing

/**********************************
 **** The Shell Of Disapproval ****
 ****** Thread Pool Edition *******
 **********************************/

// A simple shell written as a wrapper of system calls
// Uses a pool of worker threads, so compile using the
// -pthread compiler flag

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

// Struct for containing shell commands for workers
typedef struct command {
	int cmd_num;
	int num_args;
	int is_sys_call;
	char cmd_name[80];
	char args[3][10];
} command;

// Loop state for exit command
int loop = 1;

// prompt string
char * prompt = "(ಠ_ಠ) >>> ";
char * cmd_args[3];

// Number of Worker threads
// Not including boss (producer)
int num_workers = 2;

// Mutex for work buffer related data
pthread_mutex_t work_mutex = PTHREAD_MUTEX_INITIALIZER;

// Size of work buffer
// Number of tasks waiting at max
int work_buffer_size = 2;

// Buffer for commands
struct command * work_buffer;

// Number of commands  in buffer
int num_cmds_to_run = 0;

// Run or pause workers
int active_workers = 1;

// Flag fro work delegation message
int work_msg = 0;

//Array of thread identifiers
pthread_t * workers;

//Producer thread
pthread_t producer;

// Environment variables
extern char **environ;
char ** env;

// Known command strings
int num_cmds = 10;
char known_cmds[10][10] = {
	"\n",		//null command
	"clr",		//clears screen
	"dir",		//alias "ls -la"
	"environ",	//prints environment vars
	"quit",		//exits shell
	"frand",	//creates file with random ints
	"fsort",	//sorts file of ints
	"listwork",	//shows contents of work buffer
	"runwork",	//toggles worker threads on or off from doing tasks
	"msgwork",	//toggles on/off worker thread execution information
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
	0,
	0,
	0,
	};

// List of functions runnable by producer thread
int prod_funcs[10] = {
	4,
	7,
	8,
	9,
};

// Main parsing loop function
int input_loop();

// Prints work queue to screen
void print_work_buffer(int length);

// Producer thread entry function
void * start_producer(void * prod_context);

// Work wait loop for worker threads
void * work_wait(void* work_context);

// Checks if using shell defined function, returns 1 if so
int check_function(char* command);

//Get number of expected arguments for function
int check_num_args(int cmd_num);

// Runs shell defined function
int shell_function(command new_cmd);

// Runs system called function 
int system_function(command new_cmd);

// Creates file filled with random ints
int frand(const char* file, const char* size);

// Sorts contents of file
int fsort(const char* file);

// Main function
int main(int argc, char * argv[]){
	fprintf(stdout, "Initializing...\n");

	// Detect if thread count and buffer size were input
	if (argc == 3){
		int input_worker_num = atoi(argv[1]);
		int input_buffer_size = atoi(argv[2]);
		// Test valid inputs
		if(input_worker_num <= 0){
			fprintf(stderr,"Improper number of threads assigned!\n");
			// This will leave the default value 
		}
		else{
			// Set value to provided input
			num_workers = input_worker_num;
		}
		
		if(input_buffer_size <= 0){
			fprintf(stderr,"Improper work buffer size assigned!\n");
			// This will leave the default value 
		}
		else{
			// Set value to provided input
			work_buffer_size = input_buffer_size;
		}

	}

	// Allocate work buffer
	work_buffer = malloc(sizeof(command) * work_buffer_size);

	// Fill buffer with dummies
	for(int i =0; i < work_buffer_size; i++){
		work_buffer[i].cmd_num = 0;
		work_buffer[i].is_sys_call = 0;
		work_buffer[i].cmd_name[0] = '\0';
		work_buffer[i].num_args = 0;
		work_buffer[i].args[0][0] = '\0';
		work_buffer[i].args[1][0] = '\0';
		work_buffer[i].args[2][0] = '\0';
	}

	// Create Mutex
	

	// Spawn workers
	workers = malloc(sizeof(pthread_t) * num_workers);


	for(int i = 0; i< num_workers; i++){
		// Create thread and send to work state
		long context = i;
		if(pthread_create(&workers[i], NULL, work_wait, (void *)context )){
			fprintf(stderr, "Error creating thread %i!\n", i);
		}
	}

	fprintf(stdout, "Beginning work with %i threads and buffer size %i\n ",
			num_workers, work_buffer_size);

	// Start input producer thread, wrapper around input loop function
	long context = num_workers;
	if(pthread_create(&producer, NULL, start_producer, (void*) context)){
		fprintf(stderr, "Error creating input thread");
	}

	// Wait for termination of producer
	pthread_join(producer, NULL);

	fprintf(stdout, "\nProducer thread joined, waiting for workers...\n");

	for(int i = 0; i < num_workers; i++){
		pthread_join(workers[i], NULL);
	}

	fprintf(stdout, "All threads joined, Goodbye!\n");

	// Release work buffer
	free(work_buffer);

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
			// Skip both execution blocks if just new line char
			if(cmd_num > 0){
				int is_thread_work = 1;

				// Check if command is to be run by input thread
				for(int i = 0; i < 4; i++){
					if(cmd_num == prod_funcs[i]){
						struct command prod_cmd;
						prod_cmd.cmd_num = cmd_num;
						shell_function(prod_cmd);
						is_thread_work = 0;
					}

				}

				// check if room for new command exists
				if(work_buffer_size == num_cmds_to_run && is_thread_work == 1){	
					fprintf(stdout, "Waiting for free work buffer space...\n");

					// Wait for buffer to become available if currently full
					while(work_buffer_size == num_cmds_to_run){
						//stall
					} 
				}

				// check for available space in command buffer
				if(is_thread_work){
					// Take mutex
					pthread_mutex_lock(&work_mutex);
				
					// Set command name
					strcpy(work_buffer[num_cmds_to_run].cmd_name, command);

					// Set command number
					work_buffer[num_cmds_to_run].cmd_num = cmd_num;

					// Set handler flag
					work_buffer[num_cmds_to_run].is_sys_call = 0;

					// Parse arguments, save to cmd struct
					while(token != NULL){
						token = strtok(NULL, " \n\0");
						if(token == NULL){
							break;
						}
						// Save arguments to array for later use
						strcpy(work_buffer[num_cmds_to_run].args[num_params],
							token);
						
						cmd_args[num_params] = token;
						num_params ++;
					}
					
					// Set number of parameters in command
					work_buffer[num_cmds_to_run].num_args = num_params;
						
					// Check for valid number of input args
					if(num_params == check_num_args(cmd_num)){
						// Update buffer usage counter	
						num_cmds_to_run++;
					}
					// Else over dont update counter, this causes command to be lost
					// as it will be written over ont he next command input
					else{
						fprintf(stderr,"Invalid Arguments!\n");
					}
					
					//Release mutex
					pthread_mutex_unlock(&work_mutex);
				}





			}
			// Else pass to system handler
			if(cmd_num == -1){
				
				// check if room for new command exists
				if(work_buffer_size == num_cmds_to_run){	
					fprintf(stdout, "Waiting for free work buffer space...\n");

					// Wait for buffer to become available if currently full
					while(work_buffer_size == num_cmds_to_run){
						//stall
					} 
				}
				
				// Take mutex
				pthread_mutex_lock(&work_mutex);

				// Set handler flag
				work_buffer[num_cmds_to_run].is_sys_call = 1;
				
				// Copy command to buffer
				strcpy(work_buffer[num_cmds_to_run].cmd_name, input_buf); 
				
				// Incrament work counter
				num_cmds_to_run++;

				// Release mutex
				pthread_mutex_unlock(&work_mutex);
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
int system_function(command new_cmd){
	int status;
	fprintf(stdout, "\n");
       	status = system(new_cmd.cmd_name);
	fprintf(stdout, "\n%s", prompt);
	fflush(stdout);
	return status;
}

// Runs shell defined function, handler for built in functionality
int shell_function(command cmd_to_run){
	int status = 0;
	switch(cmd_to_run.cmd_num){
		// No input
		case 0:
			break;
		// clr
		case 1:
			fprintf(stdout, "\n");
			status = system("clear");
			fprintf(stdout, "%s", prompt);
			break;
		// dir
		case 2:
			fprintf(stdout, "\n");
			status = system("ls -la");
			fprintf(stdout, "%s", prompt);
			break;
		// environ
		case 3:
			fprintf(stdout, "\n");
			env = environ;
			while (*env) fprintf(stdout, "%s\n",*env++);
			fprintf(stdout, "%s", prompt);
			break;
		// Quit	
		case 4:
			loop = 0;
			fprintf(stdout,"\nQuitting...\n");
			return 0;
			break;
		// frand
		case 5:
			frand(cmd_to_run.args[0], cmd_to_run.args[1]);
			break;
		// fsort
		case 6:
			fsort(cmd_to_run.args[0]);
			break;
		// Print work buffer
		case 7:
			print_work_buffer(num_cmds_to_run);
			break;
		// Pause worker threads
		case 8:
			active_workers = (active_workers == 1) ? 0:1; 
			break;
		// Toggle task delegation messages
		case 9:
			work_msg = (work_msg == 1) ? 0:1;
			break;
		default:
			break;

	}
	fflush(stdout);
	return status;
}

// Creates file filled with random ints on child proc
int frand(const char* file, const char* size){	
	// Open file and ensure creation/ opened properly
	FILE *fp = fopen(file, "w+");
	if(fp == NULL){
		fprintf(stderr, "File failed to open!\n");
		return 1;
	}

	// Write size many values to file
	for(int i = 0; i < atoi(size); i++){
		fprintf(fp, "%i ", rand());	
	}
	// Close file
	fclose(fp);

	return 0;
}

// Compare function for sort
int cmpfunc (const void * a, const void * b) {
	   return ( *(int*)a - *(int*)b );
}

// Sorts contents of file
int fsort(const char* file){
	FILE *fp = fopen(file, "r+");
	if(fp == NULL){
		fprintf(stderr, "File failed to open!\n");
		return 1;
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
		return 1;
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
		return 1;
	}

	// Write values to file
	for(int i = 0; i < size; i++){
		fprintf(fp, "%i ", mem_ints[i]);
	}

	// Close file and exit
	fclose(fp);

	// Free number array memory
	free(mem_ints);

	return 0;
}


void print_work_buffer(int length){
	if(length > work_buffer_size){
		fprintf(stderr, "Cannot Read larger than buffer!\n");
		return;
	}

	fprintf(stdout, "[%i] of [%i] tasks in work buffer\n", length, work_buffer_size);

	for(int i = 0; i < length ; i++){
		fprintf(stdout, "Command: %s (%i) with %i args:\n", 
			work_buffer[i].cmd_name,	
			work_buffer[i].cmd_num,
			work_buffer[i].num_args);
		for(int j = 0; j < work_buffer[i].num_args; j++){
			fprintf(stdout, "\t %s\n", work_buffer[i].args[j]);
		}
	}
}


// Work wait loop for thread
void * work_wait(void* work_context){
	command thread_work;
	int tid;
	tid = (int)(long)work_context;

	// Start worker loops
	while(loop){
		int num_remaining = num_cmds_to_run;
		if(active_workers && (num_remaining > 0) && (0 == pthread_mutex_trylock(&work_mutex))){
			
			// If cmds to run has changed in the nano 
			// seconds since the evaluation,jump to next itr
			if(num_cmds_to_run != num_remaining){
				pthread_mutex_unlock(&work_mutex);
				continue;
			}

			if(work_msg){fprintf(stdout, "\nThread %i has mutex\n",tid);};

			//fprintf(stdout, "num: %i", num_cmds_to_run);

			// Copy command from buffer
			memcpy(&thread_work, &work_buffer[num_cmds_to_run - 1], sizeof(command));
			
			// Show thread name and work item taken if enabled
			if(work_msg){
				fprintf(stdout, "\nThread %i taking item %i (%s)\n",
						tid, num_cmds_to_run - 1, thread_work.cmd_name);
			}

			// Update num commands in buffer;
			num_cmds_to_run --;

			// Release Mutex
			pthread_mutex_unlock(&work_mutex);

			// Determine if shell funciton or system call
			if(thread_work.is_sys_call == 0){
				// Execute work
				shell_function(thread_work);
			}
			else{
				// Execute work
				system_function(thread_work);
			}
		}
	}
	return NULL;
}

// Producer thread entry function
void * start_producer(void * prod_context){
	input_loop();
	return NULL;
}
