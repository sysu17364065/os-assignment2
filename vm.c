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
#define MEMORY_SIZE 256*256
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
char memory[MEMORY_SIZE];
int M_index = 0; // the frame number in memory
int Value; // the final output is stored in memory 
int page_fault_counter = 0;
int Physical_Address;



/* Defining function */
// the function to initialize the page table
void init_page_table(void); 

// the function to intialize the TLB 
void init_TLB(void);

// the function to check the TLB
int check_TLB(int page);

// the function to check the page table
int check_page_table(int page);

// the function to update the TLB
void update_TLB(int page, int frame);


int main(int argc, char *argv[])
{
	char *V_Address_Dir; // addresses.txt
	char *STORE_Dir; // BACKING_STORE.bin
	int n = 0;	
	
	if(argc != 3){
		printf("Enter input, store file names!");
        exit(EXIT_FAILURE);
	}
	// receive parameters from main 
	STORE_Dir = argv[1];
	V_Address_Dir = argv[2];

	// define a file pointer
	FILE *file_ptr = NULL;
	
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
			
				//指向物理内存下一帧 
				M_index++;		
			}
			else{
				// No Frame Fault 
				// figure out the physical address
				Physical_Address = Frame_Num * FRAME_SIZE + Page_Offset;
				
				// get the final output from the memory
				Value = memory[Physical_Address];	
			}// memory
			
			//update the TLB with Page_Num, Frame_Num
			update_TLB(Page_Num, Frame_Num);
			
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
		printf("Page faults = %d\n", page_fault_counter);
		printf("TLB hits = %d\n",tlb_hit);
		 
	}
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
 
 //initialize the TLB with -1
 void init_TLB(void){
 	for(int i = 0; i <16; i++){
 		TLB[i][0] = -1;
 		TLB[i][1] = -1;
	 }
 }
 
 //check TLB[16]
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
 
 // update the TLB
 void update_TLB(int page, int frame){
 	//update the TLB 
 	TLB[tlb_count][0] = page;
 	TLB[tlb_count][1] = frame;
 	
 	//tlb_count point to the next available position
 	tlb_count++;
 	//FIFO:first in first out
 	if(tlb_count % 16 == 0)
 		tlb_count = 0;
 }


