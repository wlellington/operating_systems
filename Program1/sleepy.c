// Wesley Ellington
// Operating Systems
// Feb 5, 2018
// Program 1
//
// Sleeps process for time supplied as CLI argument

// Compiled on genuse36
// gcc sleepy.c -o sleepy

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>


// Declarations
unsigned int sleep_n(unsigned int seconds);

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
		sleep_n(count);
	}
	else{
		printf("INVALID TIME PROVIDED");
	}

	return 0;
}

// Sleeps n seconds
unsigned int sleep_n(unsigned int seconds){
	int pid = 0;
	
	// Get process ID from OS
	pid = (int) getpid();
	
	// Print ID
	printf("%i-START\n", pid);
	
	// Begin wait loop
	for(int i = 0; i < seconds; i++){
		// Get process ID to see if it changes
		pid = getpid();
		
		//Print tick and ID
		printf("%i-TICK %i\n", pid, i + 1);
		
		// Sleep for one second
		sleep(1);
	}
	return 0;
}

// 1) Yes, the process number is always the same within each run. However, 
// between runs it often changes. This is because the process
// number does not change for any reason untill the process is complete, but
// once finished, can take on a new value depending on what the OS assigns it.
//
// 2) In the simplest case, this would cause the process to move from running
// to ready, as the processor could be doing valuable work during the time
// waiting for the timer. I am not sure exactly how sleep() itself is 
// implemented, but I know that sometimes hardware timers are used for wait 
// functions, so it is possible that it is also waiting for that timer. But
// I cannot be sure without knowing more about sleep()'s actual function.
// When running on a UNIX machine, this would probably imply moving into
// the asleep in memory state. Following our more general version, the 
// process is most likely moving into the blocked state, as it is waiting
// on the interrupt from the hardware timere to begin execution again.
