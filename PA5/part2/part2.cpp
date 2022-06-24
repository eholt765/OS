/****************************************************************
 *
 * Author: Elijah Holt
 * Title: PA5 CSCE 451
 * Date: 4/10/2022
 * Description: Part 1 of a virtual memory manager that is assuming
 * there is infinite memory space. In otherwords there is no
 * need for page replacement in Part 1
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


 /******************************************************
  * Declarations
  ******************************************************/
  // #Define'd sizes
#define PAGE_TABLE 256
#define PAGE_SIZE 256

#define T_BUFF 16
#define ADDRESS_SIZE 12

#define FRAME_SIZE 256
#define FRAME_TABLE 256

#define MEMORY_SIZE 256

#define PAGE_BIT 8
#define OFFSET_MASK 0xFF


int fault = 0;
int hit = 0;
int updated_frame;

int address;
int address_count;
char address_line[ADDRESS_SIZE];

//Files
FILE* addressFile;
FILE* correct;
const char* backingStore;

// Make the TLB array
// Need pages associated with frames (could be 2D array, or C++ list, etc.)
int tlb[T_BUFF][2];
int tlb_index = 0;

// Make the Page Table
// Again, need pages associated with frames (could be 2D array, or C++ list, etc.)
int p_table[PAGE_TABLE][2];
int page_index = 0;

// Make the memory
// Memory array (easiest to have a 2D array of size x frame_size)
char phys_mem[FRAME_SIZE][FRAME_TABLE];

/******************************************************
 * Function Declarations
 ******************************************************/

 /***********************************************************
  * Function: get_page - get the page from the logical address
  * Parameters: logical_address
  * Return Value: page_num
  ***********************************************************/
int get_page(int logical_address) {

	int page_num = (logical_address >> PAGE_BIT) & 0xFFFF;
	return page_num;

}

/***********************************************************
 * Function: get_offset - get the offset from the logical address
 * Parameters: logical_address
 * Return Value: offset
 ***********************************************************/
int get_offset(int logical_address) {

	int offset = logical_address & OFFSET_MASK;
	return offset;
}

/***********************************************************
 * Function: get_frame_TLB - tries to find the frame number in the TLB
 * Parameters: page_num
 * Return Value: the frame number, else NOT_FOUND if not found
 ***********************************************************/
int get_frame_TLB(int page_num) {

	for (int i = 0; i < T_BUFF; i++) {

		if (tlb[i][0] == page_num) {

			hit = hit + 1;
			return tlb[i][1];
		}

	}
	return -1;
}

/***********************************************************
 * Function: get_available_frame - get a valid frame
 * Parameters: none
 * Return Value: frame number
 ***********************************************************/
int get_available_frame() {

	return updated_frame;
}

/***********************************************************
 * Function: get_frame_pagetable - tries to find the frame in the page table
 * Parameters: page_num
 * Return Value: page number, else NOT_FOUND if not found (page fault)
 ***********************************************************/
int get_frame_pagetable(int page_num) {

	for (int i = 0; i < PAGE_TABLE; i++) {

		if (page_num == p_table[i][0]) {

			return p_table[i][1];

		}
	}
	return -1;
}

/***********************************************************
 * Function: backing_store_to_memory - finds the page in the backing store and
 *   puts it in memory
 * Parameters: page_num - the page number (used to find the page)
 *   frame_num - the frame number for storing in physical memory
 * Return Value: none
 ***********************************************************/
void backing_store_to_memory(int page_num, int frame_num, const char* fname) {

	char buff[PAGE_SIZE];
	FILE* backingStore = fopen(fname, "rb");

	fseek(backingStore, page_num * PAGE_SIZE, SEEK_SET);

	fread(buff, sizeof(char), PAGE_SIZE, backingStore);

	for (int i = 0; i < PAGE_SIZE; i++) {

		phys_mem[frame_num][i] = buff[i];

	}

	fclose(backingStore);
}

/***********************************************************
 * Function: update_page_table - update the page table with frame info
 * Parameters: page_num, frame_num
 * Return Value: none
 ***********************************************************/
void update_page_table(int page_num, int frame_num) {
	p_table[page_index][1] = frame_num;
	p_table[page_index][0] = page_num;
	page_index = (page_index + 1) % PAGE_TABLE;
}

/***********************************************************
 * Function: update_TLB - update TLB (FIFO)
 * Parameters: page_num, frame_num
 * Return Value: none
 ***********************************************************/
void update_TLB(int page_num, int frame_num) {
	tlb[tlb_index][0] = page_num;
	tlb[tlb_index][1] = frame_num;
	tlb_index = (tlb_index + 1) % T_BUFF;
}

/***********************************************************
 * Function: print_info - print program information to file
 * Parameters: *correct
 * Return Value: non
 ****************************************************/
void print_info(FILE* correct) {

	double fault_rate = fault / (double)address_count;
	double hit_rate = hit / (double)address_count;

	fprintf(correct, "Number of Translated Addresses = %d\n", address_count);
	fprintf(correct, "Page Faults = %d\n", fault);
	fprintf(correct, "Page Fault Rate = %.3f\n", fault_rate);
	fprintf(correct, "TLB Hits = %d\n", hit);
	fprintf(correct, "TLB Hit Rate = %.3f\n", hit_rate);

	printf("Number of Translated Addresses = %d\n", address_count);
	printf("Page Faults = %d\n", fault);
	printf("Page Fault Rate = %.3f\n", fault_rate);
	printf("TLB Hits = %d\n", hit);
	printf("TLB Hit Rate = %.3f\n", hit_rate);
}



/******************************************************
 * Assumptions:
 *   If you want your solution to match follow these assumptions
 *   1. In Part 1 it is assumed memory is large enough to accommodate
 *      all frames -> no need for frame replacement
 *   2. Part 1 solution uses FIFO for TLB updates
 *   3. In the solution binaries it is assumed a starting point at frame 0,
 *      subsequently, assign frames sequentially
 *   4. In Part 2 you should use 128 frames in physical memory
 ******************************************************/

int main(int argc, char* argv[]) {
	// argument processing

	backingStore = argv[1];
	addressFile = fopen(argv[2], "r");
	correct = fopen("correct.txt", "w");

	// For Part2: read in whether this is FIFO or LRU strategy

	// initialization
	address_count = 0;
	updated_frame = 0;
	int physical_address;
	int	value;


	// Set initial talbe values
	memset(p_table, -1, sizeof(int) * PAGE_TABLE * 2);
	memset(phys_mem, -1, sizeof(MEMORY_SIZE));
	memset(tlb, -1, sizeof(int) * T_BUFF * 2);



	while (fgets(address_line, ADDRESS_SIZE, addressFile) != NULL) {

		// pull addresses out of the file
		address_count = address_count + 1;
		address = atoi(address_line);

		// Step 0: get page number and offset bit twiddling need to get the physical address (frame + offset):
		int page_num = get_page(address);
		int offset = get_offset(address);


		// Step 1: check in TLB for frame
		int frame;
		frame = get_frame_TLB(page_num);
		if (frame == -1) {

			// Step 2: not in TLB, look in page table
			frame = get_frame_pagetable(page_num);
			if (frame == -1) {

				// PAGE_FAULT!
				fault = fault + 1;

				// Step 3:dig up frame in BACKING_STORE.bin (backing_store_to_memory()) bring in frame page# x 256 store in physical memory
				frame = get_available_frame();
				updated_frame = updated_frame + 1;
				backing_store_to_memory(page_num, frame, backingStore);

				// Step 4: update page table with corresponding frame from storing into physical memory
				update_page_table(page_num, frame);
			}

			//   Step 5: (always) update TLB when we find the frame update TLB (updateTLB())
			update_TLB(page_num, frame);
		}

		// Step 6: read val from physical memory
		physical_address = (frame << PAGE_BIT) | offset;
		value = phys_mem[frame][offset];


		fprintf(correct, "Virtual address: %d Physical address: %d Value: %d\n", address, physical_address, value);
		printf("Virtual address: %d Physical address: %d Value: %d\n", address, physical_address, value);
	}

	fclose(addressFile);

	// output useful information for grading purposes
	print_info(correct);

	fclose(correct);
	return 0;
}