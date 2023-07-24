//
// Created by   bar cohen on 03/06/2023.
// ID:209499284
//


//===== includes ======================================
#include <cstring>
#include <cstdio>  // For perror
#include <cstdlib> // For exit and EXIT_FAILURE
#include <iostream>
#include <fstream>
#include <vector>
#include <fcntl.h>   // For file-related constants and functions
#include <sys/stat.h>   // For file permission constants
#include <unistd.h>
#include <cmath>

#ifndef EX_5_SIM_MEM_H
#define EX_5_SIM_MEM_H

using namespace std;

//===== macros ========================================
#define MEMORY_SIZE 200
#define NUM_EXTERNAL_ENTRIES 4

//===== types =========================================
extern char main_memory[MEMORY_SIZE];

typedef struct page_descriptor {
    bool valid; // flag that say if he is on the main_memory or not , is page mapped to frame
    int frame; // The number of the block in the main_memory that there is the process
    bool dirty; // if the page is used or not
    int swap_index; // the index of the process in the swap file
} page_descriptor;

enum MemorySection { //where the page is found
    TEXT,
    DATA,
    BSS,
    HEAP_STACK
};

// Structure definition
typedef struct ram_memory_control { // to track the memory section and the number of iterations for a page in RAM.
    MemorySection section;
    int counter_iteration;
}ram_memory_control;


//===== class sim_memory =========================================

class sim_mem {
    int swapfile_fd; //swap file fd
    int program_fd; //executable file fd
    int text_size;
    int data_size;
    int bss_size;
    int heap_stack_size;
    int page_size;
    int number_of_text_page;
    int number_of_data_page;
    int number_of_bss_page;
    int number_of_heap_stack_page;
    bool* swap_free_location;
    int no_more_open_frame;
    page_descriptor **page_table; //pointer to page table
    ram_memory_control* second_chance;

public:
    sim_mem(char exe_file_name[], char swap_file_name[], int text_size, int data_size,
            int bss_size, int heap_stack_size, int page_size);//Constructor
    ~sim_mem();//Detractor
    char load(int address);
    void store(int address, char value);
    void init_page_table(page_descriptor **page_table);
    static vector<int> convertToBinary(int address);
    MemorySection getMemorySection(const vector<int>& indices);
    int binaryToDecimal(const vector<int>& address_vector, int startIndex, int endIndex);
    void print_memory();
    void print_swap();
    void print_page_table();
    void read_page_from_file(int file_descriptor, int start_index, int page_size,char* page);
    int update_Ram_Memory(MemorySection sec,int page_location);
    int findMinField(int page_size,int page_location, int index_free);
    int write_to_swap(char *ppage);
    int count_bits(int decimal_number);
};

#endif //EX_5_SIM_MEM_H
