#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>


#define PAGE_SIZE 256
#define PAGE_NUM 256
#define PAGE_TABLE_SIZE 256
#define FRAME_SIZE 256
#define FRAME_NUM 256
#define TLB_SIZE 16


/* Virtual Addresses */

char buffer[8]; // read 8 characters once and put them in buffer[8] 
int Page_Num; // Virtual Memory Page Number
int Page_Offset; // Virtual Memory Page Offset
int Virtual_Address; // Virtual Address
int mask = 255; // 0000 0000 0000 0000 0000 0000 1111 1111
int V_Address_Count = 0; // counting the numer of Virtual Addresses
	
/* page_table[256] */
int page_table[PAGE_TABLE_SIZE];
int Frame_Num; // the frame number stored in page table

/* TLB[16] */
int TLB[TLB_SIZE][2];
int tlb_hit = 0; // record the number of "tlb hit" 
int tlb_count = 0; // record the number of the next valiable tlb 
	
/* memory[256*256] */
char memory[256*256];
int M_index = 0; // the frame number in memory
int Value; // the final output is stored in memory 
int page_fault_counter = 0;
int Physical_Address;

/* LRU frame time counter*/
int time_counter[256];


/* Defining function */
// the function to initialize the page table
void init_page_table(void); 

// the function to intialize the TLB 
void init_TLB(void);

// the function to initialize the time_counter[256] 
void init_time_counter(void);

// the function to check the TLB
int check_TLB(int page);

// the function to check the page table
int check_page_table(int page);

// free page_table
void free_page_table(int frame);

// free TLB
void free_TLB(int frame);

// the function to update the TLB
void fifo_update_TLB(int page, int frame);

// the function to update the TLB
void lru_update_TLB(int page, int frame);

// the function to record the used time of pages
void  update_time_counter(int page);

// the function to get nex available page to replace 
int lru_replace_page(void);


int main(int argc, char *argv[])
{
	char *V_Address_Dir;// = "addresses.txt";
	char *STORE_Dir;// = "BACKING_STORE.bin"; 
	char *Memory_Size;// = "128"; 
	char *Replace_Algorithm;// = "LRU"; 

	int MEMORY_SIZE; 
	int mem_empty = 1;
	
	if(argc != 5 & argc!=2){
		printf("Enter BACKING_STORE.bin, addresses.txt, Memory Size, FIFO or LRU!");
        exit(EXIT_FAILURE);
	}
	
	if(argc == 5){
		STORE_Dir = argv[1];
		V_Address_Dir = argv[2];
		Memory_Size = argv[3];
		Replace_Algorithm = argv[4];
	}
	
	if(argc == 2){
		STORE_Dir = argv[1];
		V_Address_Dir = argv[2];
		Memory_Size = "256";
		Replace_Algorithm = "FIFO";
	}
	
	// get the real memory size
	MEMORY_SIZE = atoi(Memory_Size);
	printf("MEMORY_SIZE = %d\n", MEMORY_SIZE);
	printf("%s\n",Replace_Algorithm);

	// define a file pointer
	FILE *file_ptr = NULL;
	int n;
 	//open the  BACKING_STORE.bin
 	file_ptr = fopen(STORE_Dir, "rb");
	

	// Virtual Address file pointer
	FILE *filp = NULL; 
	
	// open the addresses.txt
	filp = fopen(V_Address_Dir,"r");
   
	// initializing the page table with -1;
	init_page_table();
	
	// initializing the TLB with -1
	init_TLB();
	
	//initializing the time_counter with -1
	init_time_counter();
	
	// read a Virtual address once and store it in buffer[8]
	while(fgets(buffer,sizeof(buffer),filp)){
		/* get virtual address*/
		// char buffer[8] -> int Virtual
		Virtual_Address = atoi(buffer);
		
		// Calculate the Virtual Page Offset and Virtual Page Number
		Page_Offset = Virtual_Address&mask;
		Page_Num = (Virtual_Address>>8)&mask;
		
		// Use V_Address_Count to count the number of Virtual_Address Addresses
		V_Address_Count++;
		
		/* get physical address*/
		//check TLB
		Frame_Num = check_TLB(Page_Num);
		
		// record the least used page
		update_time_counter(Page_Num);
		
		//if TLB fail
		if (Frame_Num == -1){
	
			//check page table
			Frame_Num = check_page_table(Page_Num);
		
			if (Frame_Num == -1){ 
				// Page Fault
				page_fault_counter++;
				
				// read 256 bytes(a page) from store file to memory
				fseek(file_ptr, Page_Num*256, SEEK_SET);
 				n = fread(memory+M_index*256, 1, 256, file_ptr);
 				
 				// read fail
 				if(n == 0){
 					printf("BACKING_STORE.bin could not be read");
 					exit(EXIT_FAILURE);
				}
				
				// update the page table 
				Frame_Num = M_index; 
				page_table[Page_Num] = Frame_Num;
				
				//get final output from memory 
				Physical_Address = Frame_Num * FRAME_SIZE + Page_Offset;
				Value = memory[Physical_Address];
			
				// if memory[i] is empty
				if(mem_empty){
					
					// M_index point to the next available memory 
					M_index++; 
					
					// if memory is full
					if(M_index == (MEMORY_SIZE - 1)){
						// set men_empty = 0
						mem_empty = 0;
					}
				}
				else{
					// if memory is full
					// replace page 
					if(Replace_Algorithm == "LRU"){ 
						// LRU: find the next availabe frame
						// memory is full, replace page
						M_index = lru_replace_page();
						
						// if replace page, free page_table[i] and time_counter[i]
						free_page_table(M_index);
						
						// if replace page, free TLB[i]		
						free_TLB(M_index);	
					}
					else{
						// FIFO: find the next availabe frame
						// if memory is full, replace page
						M_index = (M_index + 1) % MEMORY_SIZE;
						
						// if replace page, free page_table[i] and time_counter[i]
						free_page_table(M_index);
					
						// if replace page, free TLB[i]		
						free_TLB(M_index);		
					}
						
				}
									
			}
			else{
				// No Frame Fault 
				// figure out the physical address
				Physical_Address = Frame_Num * FRAME_SIZE + Page_Offset;
				
				// get the final output from the memory
				Value = memory[Physical_Address];
				
			}// memory
			
			//update the TLB with Page_Num, Frame_Num
			if(Replace_Algorithm == "LRU"){
			
				//if Replace Algorithm is LRU
				lru_update_TLB(Page_Num, Frame_Num);
			}
			else{
				
				//if Replace Algorithm is FIFO
				fifo_update_TLB(Page_Num, Frame_Num);
			}
			
		}
		else{
			// TlB hit
			// figure out the physical address
			Physical_Address = Frame_Num * FRAME_SIZE + Page_Offset;
			
			// get the final output from the memory 
			Value = memory[Physical_Address];
		}
			
		
		// print Virtual Address//
		printf("%d\n", Value);
		
	}
	
	printf("Page faults = %d\n", page_fault_counter);
		printf("TLB hits = %d\n",tlb_hit);
	fclose(file_ptr);
	fclose(filp);
	exit(0);
 } 
 
 
 
 // initialize the page_table[256] with -1
 void init_page_table(void){
 	for(int i = 0; i < PAGE_TABLE_SIZE; i++){
 		page_table[i] = -1;
	 }
 }
 
 
 // initialize the TLB with -1
 void init_TLB(void){
 	for(int i = 0; i <16; i++){
 		TLB[i][0] = -1;
 		TLB[i][1] = -1;
	 }
 }
 
 
 // initialize lru time counter[256] with -1
 void init_time_counter(void){
 	for(int i = 0; i < 256; i++){
 		time_counter[i] = -1;
	 }
 }
 
 
 // check TLB[16]
 int check_TLB(int page){
 	for(int i = 0; i < 16; i++){
 		if(TLB[i][0] == page){
 			//TLB hit
 			tlb_hit++;
 			return TLB[i][1];
		 }
	 }
	 //if TLB doesn't hit
	 return -1;
 }
 
 
 // check page_table[256]
 int check_page_table(int page){
 	return page_table[page];
 }
 
 
 // free page table[i] and time_counter[i]
void free_page_table(int frame){
	
	// if replace page in memory
	// free page_table[i]
	for(int i = 0; i < PAGE_TABLE_SIZE; i++){
		
		// find out the replaced frame and free it from page_table[256]
		if(page_table[i] == frame){
			page_table[i] = -1;
			time_counter[i] = -1;
		}
	}
}


// free TLB
void free_TLB(int frame){
	// if frame is always in TLB[i], then free TLB[i]
 	// set page=-1 and frame = -1
	for(int i = 0; i < 16; i++){
		if(TLB[i][1] == frame){
			TLB[i][0] = -1; // get page = -1
			TLB[i][1] = -1; // set frame = -1
		}
	}
	 	
}


// update time counter
void  update_time_counter(int page){
	
	// time_counter of the recently used page is 0
	// time_counter of the least recently used page is the maximum.  
	for(int i =0; i < 256; i++){
		if(i == page){
			time_counter[i] = 0;
		}
		else{
			if(time_counter[i] != -1){
				time_counter[i]++;
			}
		}
	}
} 


//FIFO: update TLB
 void fifo_update_TLB(int page, int frame){	 
		//update the TLB 
 		TLB[tlb_count][0] = page;
 		TLB[tlb_count][1] = frame;
 	
 		//tlb_count point to the next available position
 		tlb_count++;
 	
 		//FIFO:first in first out
 		if(tlb_count % 16 == 0)
 			tlb_count = 0;
}
	
	
//LRU: update TLB	
void lru_update_TLB(int page, int frame){
	int tlb_p;
	int max_p; // the max time page number
	int max_time = 0;
	int lru_n; // the relative TLB number
	int empty = 0; // TLB is full
	
	for(int i = 0; i < TLB_SIZE; i++){
		
		// if TLB[i] is empty
		if (TLB[i][0] == -1){
			
			// get the empty TLB number
			empty = 1;
			
			// get the relative TLB number
			lru_n = i;
		}
		
		//if TLB[i] is full
		if (!empty){
			// get the pages number stored in TLB
			tlb_p = TLB[i][0]; 
		
			// find out the max time of those pages
			// the max-time-page is the least used page
			if(time_counter[tlb_p] > max_time){
				max_time = time_counter[tlb_p];
				
				// get the max time page number
				max_p = tlb_p;
				
				// get the relative TLB number
				lru_n = i;
			}
		}
	}
	
	// update TLB in LRU
	TLB[lru_n][0] = page;
	TLB[lru_n][1] = frame; 
}


// LRU: replace page
int lru_replace_page(void){
	int max_p;
	int max_time = 0;
	int page_table_p;
	
	// go through the page table and time counter to find the max used page 
	for(int i = 0; i < 256; i++){
		page_table_p = i;
		if(time_counter[page_table_p] > max_time){
			max_time = time_counter[page_table_p];
			max_p = page_table_p;
		}
	}
	
	// return frame number which store the max used page 
	return page_table[max_p];
}


