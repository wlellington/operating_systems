// Wesley Ellington
// Operating Systems
// Feb 16, 2018
// Program 2
//
// Sleeps a variable number of processes for the time provided by a CLI
// argument. See defines on line 20 for functionality switching.

// Compiled on genuse36
// gcc twoSleepy.c -o twoSleepy

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/wait.h>


// Change one of the two following values to 1 to enable
// alternate functionality (Parts 2 and 3 of assignment)
#define PART_2 0
#define PART_3 0

// Declarations
unsigned int sleep_n(unsigned int seconds);
unsigned int spawn_children();

// Main loop
int main(int argc, char *argv[ ]) {
      	int count;
	// Check for input
      	if (argc == 2){
		count = atoi(argv[1]);
	}
	// Default to five if no input
	else{
		count = 5;
	}
	
	// Check valid number has been supplied
	if(count >= 0){
		spawn_children(count);
	}
	else{
		printf("INVALID TIME PROVIDED");
	}

	return 0;
}

unsigned int spawn_children(unsigned int seconds){
	// get original pid
	int pid = (int) getpid();
	int ppid = (int) getppid();
	
	// Spawn child process
	int fork_result = fork();

	// Detect error
	if(fork_result < 0){
		printf("No Child Spawned, Aborting!");
		return 1;
	}

	// Parent Code
	if (fork_result != 0){
		// Declare child status
		int status = 0;

		// Begin wait loop
		for(int i = 0; i < seconds; i++){
			//Print tick and ID
			printf("Original Process with PID: %i and PPID: %i; tick %i\n", pid, ppid, i + 1);
			
			// Sleep for one second
			if (!PART_2){
				sleep(1);
			}
		}
		//Disable for part 2
		if(!PART_2){
			// Wait for child process to exit
			while( wait(&status) > 0){
				//printf("Exit status was: %i\n", status);
			}
		}
		printf("Parent process terminating\n");
	}

	// Child Code
	else{
		// Execute for parts 1 and 2
		if(!PART_3){
			// Set child pid
			pid = (int) getpid();
			ppid = (int) getppid();
			// Begin wait loop
			for(int i = 0; i < seconds; i++){
				//Print tick and ID
				printf("Child Process with PID: %i and PPID: %i; tick %i\n", pid, ppid, i + 1);
				
				// Sleep for one second
				sleep(1);
			}
			printf("Child process terminating\n");
			
			// Exit child
			exit(1);
		}
		//Execute for part 3
		else{
			// Set child pid
			pid = (int) getpid();
			ppid = (int) getppid();

			// Spawn child process
			int fork_result = fork();

			// Detect error
			if(fork_result < 0){
				printf("No Grandchild Spawned, Aborting!");
				return 1;
			}
			
			//Child code
			if(fork_result != 0){
				// Declare grandchild status
				int status = 0;

				// Begin wait loop
				for(int i = 0; i < seconds; i++){
					//Print tick and ID
					printf("Child Process with PID: %i and PPID: %i; tick %i\n", pid, ppid, i + 1);
					
					// Sleep for one second
					sleep(1);
				}
				// Wait for child process to exit
				while( wait(&status) > 0){
					//printf("Exit status was: %i\n", status);
				}
				printf("Child process terminating\n");
				exit(1);
			}
			//Grandchild code
			else{
				// Set grandchild pid
				pid = (int) getpid();
				ppid = (int) getppid();
				// Begin wait loop
				for(int i = 0; i < seconds; i++){
					//Print tick and ID
					printf("Grandchild Process with PID: %i and PPID: %i; tick %i\n", pid, ppid, i + 1);
					
					// Sleep for one second
					sleep(1);
				}
				printf("Grandchild process terminating\n");
				
				// Exit Grandchild
				exit(1);
			}	
		}
	}
	return 0;	
}


// Part I
// Starting out, each process will start in the new state, created by a fork
// or a call from the CLI. After executing a bit, it may move to ready,
// depending on system load. In the loop, every time the sleep function is 
// called, assuming a hardware timer is being used, the processes goes into
// the blocked state, waiting for the timer to return. On completion, the
// process is moved back into ready, then to running again. This repeats
// until the children call exit(), or when the final original thread returns
// in the main().
//
// The interleaving here makes a decent amount of sense given the above. The
// processes are spawned in order, with a small amount of time between, giving
// one a head start on its sleep timer. Small exeptions are seen occasionally,
// probably due to context switches or other things being handled by the OS. 
// The termiation messages must always be in order, as the child process
// must finish before the parent, giving it a stack-like behavior when 
// shutting down.
//
//
//## SEE DEFINES AT TOP TO ENABLE PART II AND PART III CODE ###
//
// Part II
// At execution, the parent processes ticks all the way to n in a fraction of
// a second. Then, the child begins to run normally, but with a ppid of 1.
// At the end of its run, the child hangs indefiniately, until killed by the 
// user. My guess is that the child process has become a zombie, that is,
// it no longer has a parent process and no oversight from above. This is
// very undesireable, as now all sorts of issues with resource allocation,
// as well as termination problems can occur.
//
// Part III
// Runs as expected. Some minor out of sequence prints may appear, but it
// was not more so than the results of part one. None of the outputs within
// the loop are synchronized, so this is to be exepected.
