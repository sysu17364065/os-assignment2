#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <alloca.h>

extern void show_stack(void);  // a function to show the stack growth

int bss_var;
int data_var1 = 40; 
int data_var2 = 42;

int main(int argc, char **argv){
	
	char *stack_p, *h, *new_h;
	
	printf("Text Locations:\n");
	printf("\tAddress of main: %p\n", main);
	printf("\tAddress of show_stack: %p\n", show_stack);
	
	printf("Stack Locations:\n");
	show_stack();
	
	stack_p = (char *) alloca(48);
	
	if(stack_p != NULL){
		printf("\tStart of alloca() 'ed array: %p\n", stack_p);
		printf("\tEnd of alloca() 'ed array: %p\n", stack_p + 47);
	}
	
	printf("Data Locations:\n");
	printf("\tAddress of data_var1: %p\n", &data_var1);
	printf("\tAddress of data_var2: %p\n\n", &data_var2);
	
	printf("BSS Locations:\n");
	printf("\tAddress of bss_var: %p\n", &bss_var);
	printf("\tBUSS is above data variables.\n\n");
	
	h = sbrk((ptrdiff_t)16);
	new_h = sbrk((ptrdiff_t)0);
	
	printf("Heap Locations:\n");
	printf("\tInitial end of heap: %p\n", h);
	printf("\tHeap grow upward.\n");
	printf("\tNew end of heap: %p\n", new_h);
	
	h = sbrk((ptrdiff_t)-48);
	new_h = sbrk((ptrdiff_t)0);
	
	printf("\tAddress space shrinks.\n");
	printf("\tFinal end of heap: %p\n\n", new_h);
	
	sleep(100);
		
}

void show_stack(void){
	static int level = 0;
	auto int stack_var;
	level++;
	if(level == 4)
		return;
	
	printf("\tStack level %d: Address of stack_var: %p\n", level, &stack_var);
	
	show_stack();
}
