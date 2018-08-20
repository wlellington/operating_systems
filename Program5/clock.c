/* Wesley Ellington
 * Operating Systems
 * Spring 2018
 *
 * Program 5 */

/* This is a simulated implementation of the clock page replacement algorithm. It can be invoked from
 * the command line using 'clock <x>' where x is the size of the circular array. */

/* Additional support added for monitoring hit rate should statistics be desired 
 * To use invoke with 'clock <x> s' with x as size of table and s as 'stats' flag*/

// compile: gcc clock.c -o clock


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define CLOCK_SIZE 4

// Structs for storing page frame data
typedef struct record_entry{
	int frame;
	int page;
	int use;
	int modify;
} record_entry;	

// 'Clock' assembly, stores all page information in iteratable struct
typedef struct frame_record{
	int clock_size;
	int curr_ptr;
	record_entry* records;
	int num_requests;
	int num_hits;
} frame_records;

// Initialize empty record
struct frame_record* create_record(int clock_sz);

// Clean up record
void destroy_record(struct frame_record* table);

// Search for page in already filled frames
int search_record(struct frame_record* table, int page_num);

// Print records out to screen or file
void print_record(FILE* filePtr, struct frame_record* table);

// Process requests, clock algorithm main body
void process_requests(FILE* infile, FILE* outfile, struct frame_record* table);

// Find next valid location for replacement, returns -1 if no location is found
// Acts as sweeps of clock 'arm'
int scan_no_update(struct frame_record* table);
int scan_update(struct frame_record* table);

// Main function
int main(int argc, char* argv[]){
 	// Set clock size based on input or default
	int clock_sz = CLOCK_SIZE;
	
	// Change clock size
	if(argc > 1){
		int input_size = atoi(argv[1]);
		if(input_size > 0){
			clock_sz = input_size;
		}
	}

	// Enable hit rate display
	int disp_hit_rate = 0;
	if(argc == 3){
		 disp_hit_rate  = (strcmp(argv[2], "s") == 0) ? 1:0;
	}

	// Open input file
	char inFileName[ ] = "testdata.txt";
	FILE *inFilePtr = fopen(inFileName, "r");
	if(inFilePtr == NULL) {
		printf("File %s could not be opened.\n", inFileName);
		exit(1);
	}
 
	// Initialize and open output file
	char outFileName[ ] = "results.txt";
	FILE *outFilePtr = fopen(outFileName, "w");
	if(outFilePtr == NULL) {
		printf("File %s could not be opened.\n", outFileName);
		exit(1);
	}


	// Construct frame record
 	struct frame_record * frame_table = create_record(clock_sz);

	// Iterate over input requests
	process_requests(inFilePtr, outFilePtr, frame_table);	

	// Print hit rate if enabled
	if(disp_hit_rate){
		int hits = frame_table->num_hits;
		int reqs = frame_table->num_requests;
		double hit_rate = (double)hits/ (double)reqs;

		fprintf(outFilePtr, "HIT RATE: %f, [%i hit of %i requests]\n",
				hit_rate, hits, reqs);
	}


	// Cleanup
	destroy_record(frame_table);

	fclose(inFilePtr);
	fclose(outFilePtr);
	return 0;
};

struct frame_record* create_record(int clock_sz){
	struct frame_record* new_record = malloc(sizeof(struct frame_record));
	new_record->clock_size = clock_sz;
	new_record->curr_ptr = 0;
	new_record->num_hits = 0;
	new_record->num_requests = 0;
	new_record->records = malloc(sizeof(struct record_entry) * clock_sz);
	for(int i = 0; i < clock_sz; i++){
		new_record->records[i].frame = i;
		new_record->records[i].page = -1;
		new_record->records[i].use = 0;
		new_record->records[i].modify = 0;
	}

	return new_record;
}

// Clean up record
void destroy_record(struct frame_record* table){
	free(table->records);	
}

// Search records for specific page
int search_record(struct frame_record* table, int page_num){
	// Iterate through frames
	for(int i = 0; i < table->clock_size; i++){
		// if page is resident in frame, return index
		if(table->records[i].page == page_num){
			return i;
		}
	}
	// Else return null cond
	return -1;
}

void print_record(FILE* filePtr, struct frame_record* table){	
	fprintf(filePtr, "FRAME\tPAGE\tUSE\tMODIFY\n");
	for(int i = 0; i < table->clock_size; i++){
		// Basic frame information
		fprintf(filePtr, "%i\t%i\t%i\t%i",
				table->records[i].frame,
				table->records[i].page,
				table->records[i].use,
				table->records[i].modify);
		
		// Indicate location of pointer
		if(i == table->curr_ptr){
			fprintf(filePtr, " <-next frame");
		}
		
		// Line return
		fprintf(filePtr, "\n");
	}
	fprintf(filePtr, "\n");
}

// Process requests, the heart of the clock algorithm
void process_requests(FILE* infile, FILE* outfile, struct frame_record* table){
	int page = 0;
	char operation = 'r';

	int test_pointer = 0;
	fscanf(infile, "%d%c", &page, &operation);
	while(!feof(infile)) {
		fprintf(outfile, "Page referenced: %i %c\n", page, operation);
		
		// Update numeber of total requests
		table->num_requests++;

	       	// Check if page is already resident
		int search_result = search_record(table, page);
		if(search_result != -1){
			// use bit to 1 and move on
			table->records[search_result].use = 1;
			int tmp_mod = table->records[search_result].modify;
			if(operation == 'w'){
				tmp_mod = 1;
			}
			table->records[search_result].modify = tmp_mod;
			
			// Update hit counter
			table->num_hits++;
		}
		// If not found, begin scans
		else{
			int scan_done = 0;
			int repl_index = -1;
			
			// Make first pass
			repl_index = scan_no_update(table);	
			scan_done = (repl_index != -1) ? 1:0;

			// Check for m = 1, update pass
			if (!scan_done){
				repl_index = scan_update(table);
				scan_done = (repl_index != -1) ? 1:0;
			}

			// Second scan with no update
			if(!scan_done){
				repl_index = scan_no_update(table);
				scan_done = (repl_index != -1) ? 1:0;
			}

			// Final pass, shouldnt happen	
			if (!scan_done){
				repl_index = scan_update(table);
				scan_done = (repl_index != -1) ? 1:0;
			}

			// Update Records
			table->records[repl_index].page = page;
			table->records[repl_index].use = 1;
			table->records[repl_index].modify = (operation == 'w') ? 1:0;
			
			// Update next pointer
			repl_index++;
			// loop if neccessary
			if(repl_index == table->clock_size){
				repl_index = 0;
			}

			table->curr_ptr = repl_index;


 		}
 
 		// Print Record to File
         	print_record(outfile, table);

		// Get next Request	
		fscanf(infile, "%d%c", &page, &operation);
	}
}	

// Find next valid location for replacement, returns -1 if no location is found
int scan_no_update(struct frame_record* table){
	int test_index = table->curr_ptr;
	struct record_entry* base = table->records;
	for(int i = 0; i < table->clock_size; i++){
		// Check if u = 0 and m = 0
		if(base[test_index].use == 0 
				&& base[test_index].modify == 0){
			return test_index;
		}
		
		// Incrament current index
		test_index ++;
		
		// loop
		if(test_index == table->clock_size){
			test_index = 0;
		}
	}
	return -1;
}

int scan_update(struct frame_record* table){
	int test_index = table->curr_ptr;
	struct record_entry* base = table->records;
	for(int i = 0; i < table->clock_size; i++){
		// Check u = 0, m = 1
		if(base[test_index].use == 0 
				&& base[test_index].modify == 1){
			return test_index;
		}
		
		// Update use bit
		base[test_index].use = 0;

		test_index ++;
		
		// loop
		if(test_index == table->clock_size){
			test_index = 0;
		}
	}
	return -1;
}


