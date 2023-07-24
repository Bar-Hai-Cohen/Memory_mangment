//
// Created by   bar cohen on 03/06/2023.
// ID:209499284
//
#include "sim_mem.h"

char main_memory[MEMORY_SIZE];

/******************************/
/*
 * constructor implementation for a class named sim_mem.
 * It initializes various member variables and performs file operations to open and create files.
 * There is also an allocation for the page table using dynamic memory.
 */
sim_mem::sim_mem(char exe_file_name[], char swap_file_name[], int text_size, int data_size, int bss_size,
                 int heap_stack_size, int page_size) {

    //Declare properties
    this->text_size = text_size;
    this->data_size = data_size;
    this->bss_size = bss_size;
    this->heap_stack_size = heap_stack_size;
    this->page_size = page_size;

    number_of_text_page = text_size / page_size;//the number of pages in the text section
    if (text_size % page_size != 0) {
        number_of_text_page++;
    }
    number_of_data_page = data_size / page_size;//the number of pages in the data section
    if (data_size % page_size != 0) {
        number_of_data_page++;
    }
    number_of_bss_page = bss_size / page_size;//the number of pages in the bss section
    if (bss_size % page_size != 0) {
        number_of_bss_page++;
    }
    number_of_heap_stack_page = heap_stack_size / page_size;//the number of pages in the heap_stack section
    if (heap_stack_size % page_size != 0) {
        number_of_heap_stack_page++;
    }

    int num_elements = MEMORY_SIZE / page_size; // The number of frame

    // Allocate the array of structures of the manage array
    second_chance = new ram_memory_control[num_elements];

    if (second_chance == nullptr) { // allocation failed
        printf("ERR\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_elements; ++i) { //Set default value in the struct in the array
        second_chance[i].counter_iteration = 0;
        second_chance[i].section = TEXT;
    }

    // Use memset to set all elements of main_memory to zero
    memset(main_memory, '0', MEMORY_SIZE);

    //if the name of file is NULL -- > error and exit
    if (swap_file_name == nullptr || exe_file_name == nullptr) {
        printf("ERR\n");
        exit(EXIT_FAILURE);
    }
    this->program_fd = open(exe_file_name, O_RDONLY); // open the exec file to read only
    if (program_fd == -1) { // if failed error and exit
        printf("ERR\n");
        exit(EXIT_FAILURE);
    }
    // Open the swap file and assign the file descriptor
    this->swapfile_fd = open(swap_file_name, O_RDWR | O_CREAT,
                             S_IRUSR | S_IWUSR); // open the swap to read and write also
    if (swapfile_fd == -1) { // if failed error and exit
        printf("ERR\n");
        exit(EXIT_FAILURE);
    }

    /*
     * The code snippet you provided writes a specified number of zero bytes to the SwapFile output file stream.
     * Each zero byte is written using the write function.
     */

    // Fill the swap file with zero
    const int numZeros = page_size * (number_of_bss_page + number_of_heap_stack_page +
                                      number_of_data_page); // Number of zeros to write
    char zero = '0';  // Zero character
    for (int i = 0; i < numZeros; i++) {    // Write zeros to the file
        write(swapfile_fd, &zero, sizeof(zero));
    }

    // Allocate memory for the page table
    page_table = new page_descriptor *[NUM_EXTERNAL_ENTRIES];
    init_page_table(page_table); // init the internal page

    int total_pages =
            number_of_data_page + number_of_bss_page + number_of_heap_stack_page; // the number of the total page
    swap_free_location = new bool[total_pages]; // array of empty index space in swap file
    for (int i = 0; i < total_pages; ++i) { //set all the value to default true
        swap_free_location[i] = true;
    }
    no_more_open_frame = MEMORY_SIZE / page_size;

}

/******************************/
/*
 * function is used to initialize the page_table
 */
void sim_mem::init_page_table(page_descriptor **page_table) {
    // Allocate memory for the internal page table
    page_table[0] = new page_descriptor[number_of_text_page];
    page_table[1] = new page_descriptor[number_of_data_page];
    page_table[2] = new page_descriptor[number_of_bss_page];
    page_table[3] = new page_descriptor[number_of_heap_stack_page];
    int internal_index = 0;
    for (int i = 0; i < NUM_EXTERNAL_ENTRIES; i++) {
        // Initialize the internal page table entries
        if (i == 0)internal_index = number_of_text_page;
        if (i == 1)internal_index = number_of_data_page;
        if (i == 2)internal_index = number_of_bss_page;
        if (i == 3)internal_index = number_of_heap_stack_page;

        // Create and initialize the page tables
        for (int j = 0; j < internal_index; j++) {
            page_table[i][j].valid = false;
            page_table[i][j].frame = -1;
            page_table[i][j].dirty = false;
            page_table[i][j].swap_index = -1;
        }
    }
}

/******************************/
/*
 * function to print the main_memory
 */
void sim_mem::print_memory() {
    int i;
    printf("\n Physical memory\n");
    for (i = 0; i < MEMORY_SIZE; i++) { // print every memory index in the array
        printf("[%c]", main_memory[i]);
        //TODO:REMOVE THIS IF
        if (i != 0 && i % 70 == 0) {
            printf("\n");
        }
    }
}

/****************************/
/*
 * function you provided is used to print the contents of the swap memory.
 * It reads data from the swap file and displays it on the console.
 */
void sim_mem::print_swap() {
    char *str = static_cast<char *>(malloc(this->page_size *
                                           sizeof(char))); //This line dynamically allocates memory to store a page of data from the swap file.
    int i;
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while (read(swapfile_fd, str, this->page_size) == this->page_size) {
        for (i = 0; i < page_size; i++) {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }
    free(str);

}

/*****************************/
//  function to print the page descriptor table
void sim_mem::print_page_table() {
    int i;
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for (i = 0; i < number_of_text_page; i++) { // print the text page
        printf("[%d]\t      [%d]\t     [%d]\t    [%d]\n",
               page_table[0][i].valid,
               page_table[0][i].dirty,
               page_table[0][i].frame,
               page_table[0][i].swap_index);

    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for (i = 0; i < number_of_data_page; i++) { // print the data page
        printf("[%d]\t      [%d]\t     [%d]\t    [%d]\n",
               page_table[1][i].valid,
               page_table[1][i].dirty,
               page_table[1][i].frame,
               page_table[1][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for (i = 0; i < number_of_bss_page; i++) {// print the bss page
        printf("[%d]\t      [%d]\t     [%d]\t    [%d]\n",
               page_table[2][i].valid,
               page_table[2][i].dirty,
               page_table[2][i].frame,
               page_table[2][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for (i = 0; i < number_of_heap_stack_page; i++) { // print the heap stack page
        printf("[%d]\t      [%d]\t     [%d]\t    [%d]\n",
               page_table[3][i].valid,
               page_table[3][i].dirty,
               page_table[3][i].frame,
               page_table[3][i].swap_index);
    }
}

/*****************************/
/*
 * Destructor
 */
sim_mem::~sim_mem() {
    // Deallocate memory for the page table
    for (int i = 0; i < NUM_EXTERNAL_ENTRIES; i++) {
        delete[] page_table[i];
    }
    delete[] page_table;

    // Deallocate the allocated memory
    delete (second_chance);// free the array manage memory
    second_chance = nullptr;
    delete (swap_free_location);
    swap_free_location = nullptr;
    close(swapfile_fd); //close file
    close(program_fd);// close file
}

/*****************************/
/*
 * this method converts from int to binary number and return it
 */
vector<int> sim_mem::convertToBinary(int number) {
    std::vector<int> binaryArray(12, 0); // reset vector to zero

    for (int i = 12 - 1; i >= 0; i--) {
        binaryArray[i] = number % 2; //This will store the current binary digit (0 or 1) in the array.
        number /= 2; //This line divides number by 2, effectively shifting it one bit to the right. T
    }
    return binaryArray;
}

/*****************************/
/*
 * This function is responsible for loading data from memory based on the given address.
 */
char sim_mem::load(int address) {
    std::vector<int> address_vector = convertToBinary(
            address); //converting the decimal address into a binary representation
    MemorySection section = getMemorySection(
            address_vector); //It then determines the memory section based on the address
    int frame_size =
            11 - count_bits(this->page_size); //calculated as the number of bits required to represent the page size.
    int start_index_frame = 2;
    int page_location = binaryToDecimal(address_vector, start_index_frame, frame_size +
                                                                           1); //The page location is calculated by converting a portion of the binary address to decimal, using the binaryToDecimal
    int valid_number_page = 0;
    switch (section) { //check the section and update the num_page
        case TEXT:
            valid_number_page = number_of_text_page;
            if (page_location >= number_of_text_page) {
                printf("ERR\n");
                return '\0';
            }
            break;
        case BSS:
            if (page_location >= number_of_bss_page) {
                printf("ERR\n");
                return '\0';
            }
            break;
        case DATA:
            if (page_location >= number_of_data_page) {
                printf("ERR\n");
                return '\0';
            }
            break;
        case HEAP_STACK:
            if (page_location >= number_of_heap_stack_page) {
                printf("ERR\n");
                return '\0';
            }
            break;
    }
    int start_index_offset = (12 - count_bits(this->page_size -
                                              1)); //is calculated based on the number of bits required to represent the page size minus 1.
    int index_to_ram;
    int offset = binaryToDecimal(address_vector, start_index_offset,
                                 11); //The offset is calculated by converting a portion of the binary address to decimal, using the binaryToDecimal function. The start index is start_index_offset, and the size is 11.
    if (page_table[section][page_location].valid) { // if the page is on the memory ram
        int index_ram = page_table[section][page_location].frame * page_size + offset - 1;
        if(no_more_open_frame<=0){
            for (int i = 0; i < MEMORY_SIZE/page_size; ++i) {
                if(i==page_table[section][page_location].frame){
                    second_chance[i].counter_iteration=MEMORY_SIZE/page_size-1;
                }
                else if(second_chance[i].counter_iteration!=0){
                    second_chance[i].counter_iteration--;
                }
            }
        }
        return main_memory[index_ram]; // return the letter
    } else if (section == TEXT) { // if is text
        off_t start;
        char page_temporary[page_size];
        start = page_location * this->page_size;  // Starting offset
        read_page_from_file(program_fd, start, this->page_size, page_temporary); // bring the page form the exec file
        if ((page_location + 1) * page_size > text_size) {
            int gap = (page_location + 1) * page_size - text_size;
            for (int i = 0; i < gap; ++i) {
                page_temporary[page_size - 1 - i] = 0;
            }
        }
        index_to_ram = update_Ram_Memory(section, page_location); // return which frame is open in the ram
        for (int i = 0; i < page_size; ++i) { // copy to memory ram the value
            main_memory[index_to_ram * page_size + i] = page_temporary[i];
        }
        page_table[section][page_location].valid = true; // valid true because its on memory
        page_table[section][page_location].frame = index_to_ram; // where the frame
        return main_memory[index_to_ram * page_size + offset]; // return the index
    } else {
        if (page_table[section][page_location].dirty) {//if the page is dirty he is in the swap
            off_t start;
            char page_temporary[page_size];
            read_page_from_file(swapfile_fd, page_table[section][page_location].swap_index, this->page_size,
                                page_temporary); // bring the page from the swap file
            int new_free_spot =
                    (page_table[section][page_location].swap_index) / page_size; // new free spot in swap file
            char zeros[page_size];
            memset(zeros, '0', page_size);
            write_to_swap(zeros); // write to swap file zero
            swap_free_location[new_free_spot] = true;

            char zeroes[page_size];
            memset(zeroes, '0', page_size);
            int swap_location = write_to_swap(zeroes); // write to swap
            index_to_ram = update_Ram_Memory(section, page_location); // return which frame is open in the ram
            for (int i = 0; i < page_size; ++i) { //  copy to memory ram the value
                main_memory[index_to_ram * page_size + i] = page_temporary[i];
            }
            page_table[section][page_location].frame = index_to_ram;
            page_table[section][page_location].valid = true;
            page_table[section][page_location].swap_index=-1;
            return main_memory[index_to_ram * page_size + offset];
        }
            //if the file is not dirty he is in the exec file
        else {
            int section_temp = section;
            // Declare the variables outside the switch statement
            off_t start;
            char page_temporary[page_size];
            int index_to_ram;

            // Switch statement to check the value of section_1
            switch (section_temp) {
                case 1: //DATA
                    start = page_size * number_of_text_page + page_location * page_size;  // Starting offset
                    read_page_from_file(program_fd, start, this->page_size, page_temporary); // read from exec file
                    if ((page_location + 1) * page_size > data_size) {
                        int gap = (page_location + 1) * page_size - data_size;
                        for (int i = 0; i < gap; ++i) {
                            page_temporary[page_size - 1 - i] = 0;
                        }
                    }
                    index_to_ram = update_Ram_Memory(section, page_location); // return which frame is open in the ram
                    for (int i = 0; i < page_size; ++i) {
                        main_memory[index_to_ram * page_size + i] = page_temporary[i];
                    }
                    page_table[section][page_location].frame = index_to_ram;
                    page_table[section][page_location].valid = true;
                    return main_memory[index_to_ram * page_size + offset];

                case 2: //BSS
                    index_to_ram = update_Ram_Memory(section, page_location); // return which frame is open in the ram
                    for (int i = 0; i < page_size; ++i) {
                        main_memory[index_to_ram * page_size + i] = '0';
                    }
                    page_table[section][page_location].frame = index_to_ram;
                    page_table[section][page_location].valid = true;
                    return main_memory[index_to_ram * page_size + offset];
                case 3: // HEAP_STACK
                    printf("ERR\n");
                    break;

            }
        }
    }
    return '\0';
}

/*****************************/
/*
 * The code you provided is a member function getMemorySection of the sim_mem class.
 * It takes a vector of integers address_vector as input and returns the corresponding
 * MemorySection enum based on the binary representation of the two elements in the vector.

 */
MemorySection sim_mem::getMemorySection(const vector<int> &address_vector) {
    int binaryValue = address_vector[0] << 1 | address_vector[1]; // conact the to first bit in array

    switch (binaryValue) {
        case 0b00:
            return TEXT;
        case 0b01:
            return DATA;
        case 0b10:
            return BSS;
        case 0b11:
            return HEAP_STACK;
        default:
            // Handle invalid case
            return TEXT; // Return a default value or throw an exception
    }
}
/*****************************/
/*
 * The function sim_mem::binaryToDecimal takes a vector address_vector containing binary digits,
 * along with the startIndex and endIndex representing the range of the binary digits to convert to a decimal number.
 * The function iterates over the specified range in reverse order and converts the binary digits to a decimal number
 * using the formula decimalNumber += bit * pow(2, endIndex - i). Finally, it returns the resulting decimal number.
 */
int sim_mem::binaryToDecimal(const vector<int> &address_vector, int startIndex, int endIndex) {
    int decimalNumber = 0;

    for (int i = endIndex; i >= startIndex; i--) {
        int bit = address_vector[i];
        decimalNumber += bit * pow(2, endIndex -
                                      i); //This line calculates the decimal value of the current bit and adds it to the decimalNumber
    }

    return decimalNumber;
}

/*****************************/
/*
 * A function that receives an index to start reading the page from, reads and inserts into an array
 * whose pointer is also sent to the function.
 * After reading, we will update the location we read from with zeros and mark it as a free place
 */
void sim_mem::read_page_from_file(int file_descriptor, int start_index, int page_size, char page[]) {
    /*
     * line seeks to the desired position in the file specified by the file_descriptor.
     * The start_index indicates the offset from the beginning of the file where the seeking operation should be performed.
     * The SEEK_SET flag specifies that the offset should be relative to the start of the file.
     */
    off_t newPosition = lseek(file_descriptor, start_index, SEEK_SET);
    if (newPosition == -1) {
        printf("ERR\n");
        close(file_descriptor);
    }
    // char buffer[page_size];  // Buffer to store the read data
    read(file_descriptor, page, page_size); // read the data

}

/*****************************/
/*
 * updates the RAM memory by assigning a memory section (sec) to a specific page location
 * (page_location) using a second chance algorithm.
 */
int sim_mem::update_Ram_Memory(MemorySection sec, int page_location) {
    int len = (MEMORY_SIZE / this->page_size); //  This line calculates the number of frames (pages) in the RAM memory.
    int flag_frame_open = 0;
    int index_return = -1;
    for (int i = 0; i < len; ++i) { // oop iterates over the frames in the RAM memory
        if (second_chance[i].counter_iteration == 0 && flag_frame_open != 1) { // find  open place in the array
            if (no_more_open_frame <= 0)index_return = findMinField(this->page_size, page_location, i);
            second_chance[i].counter_iteration = len - 1;
            second_chance[i].section = sec;
            no_more_open_frame--;
            flag_frame_open = 1;
            index_return = i;
        } else { // if not down the bit in one and give " second chance"
            if (second_chance[i].counter_iteration > 0)
                second_chance[i].counter_iteration--;
        }
    }
    return index_return;

}

/*****************************/
/*
 * used to find the frame with the smallest value in a specific field (size and page location) within the second_chance array.
 *
 */
int sim_mem::findMinField(int page_size, int page_location, int index_free) {

    ram_memory_control ram_control;  // Create an instance of ram_memory_control

    int len = MEMORY_SIZE / page_size;
    int flag_dirty = 0;
    int num_pages = 0;

    switch (second_chance[index_free].section) { //check the section and update the num_page
        case TEXT:
            num_pages = number_of_text_page;
            break;
        case BSS:
            num_pages = number_of_bss_page;
            break;
        case DATA:
            num_pages = number_of_data_page;
            break;
        case HEAP_STACK:
            num_pages = number_of_heap_stack_page;
            break;
    }
    int frame_not_valid = 0;
    for (int i = 0; i < num_pages; ++i) { // check if this frame is dirty them flag rise to 1
        if (page_table[second_chance[index_free].section][i].frame == index_free &&
            page_table[second_chance[index_free].section][i].dirty) {
            flag_dirty = 1;
            frame_not_valid = i;
            break;
        } else if (page_table[second_chance[index_free].section][i].frame == index_free) {
            frame_not_valid = i;
            break;
        }


    }
    //return the index free and set the valid to false and set frame to zero
    if (second_chance[index_free].section == TEXT) { //TEXT delete the page
        page_table[second_chance[index_free].section][frame_not_valid].valid = false;
        page_table[second_chance[index_free].section][frame_not_valid].frame = -1;
        return index_free;
    } else if (second_chance[index_free].section == DATA && flag_dirty == 0) { //DATA and not  dirty delete him
        page_table[second_chance[index_free].section][frame_not_valid].valid = false;
        page_table[second_chance[index_free].section][frame_not_valid].frame = -1;
        return index_free;
    } else if (flag_dirty == 1) { //page that dirty -- data/ bss /heap stack
        int start = page_table[second_chance[index_free].section][frame_not_valid].frame * this->page_size;
        char ppage[this->page_size];
        for (int i = 0; i < page_size; ++i) {
            ppage[i] = main_memory[start + i];
        }
        int swap_location = write_to_swap(ppage);
        page_table[second_chance[index_free].section][frame_not_valid].valid = false;
        page_table[second_chance[index_free].section][frame_not_valid].frame = -1;
        page_table[second_chance[index_free].section][frame_not_valid].dirty = true;
        page_table[second_chance[index_free].section][frame_not_valid].swap_index = swap_location;
        return index_free;
    } else { // NOT DIRTY
        if (second_chance[index_free].section == HEAP_STACK ||
            second_chance[index_free].section == BSS) { //HEAP_STACK//BSS delete the page
            page_table[second_chance[index_free].section][frame_not_valid].valid = false;
            page_table[second_chance[index_free].section][frame_not_valid].frame = -1;
            return index_free;
        }
    }
    return 0; // not need to get to here its just that  don't get warning

}

/*****************************/
/*
 * We keep a boolean array of the length of the number of pages that go into the swap file,
 * in this function we will check which space is free, and in the first free space we find
 * we will write our page and of course we will update the same index to false
 * The write_to_swap function is responsible for writing a page (ppage) to the swap file.
 */
int sim_mem::write_to_swap(char *ppage) {
    int first_free_location; //store the index of the first available free
    int total_pages = number_of_data_page + number_of_bss_page + number_of_heap_stack_page;
    for (first_free_location = 0; first_free_location <
                                  total_pages; ++first_free_location) { //Iterate from first_free_location equals 0 to total_pages - 1 to find the first free location in the swap file.
        if (swap_free_location[first_free_location]) {
            //mark the location as used by setting swap_free_location[first_free_location] to false, and break out of the loop.
            swap_free_location[first_free_location] = false;
            break;
        }
    }
    /*
     * use lseek to move the file offset to the desired position in the swap file, which is calculated as first_free_location * page_size.
       Check if lseek was successful by comparing newPosition to -1. If it's -1, an error occurred while seeking,
       so print an error message, close the swap file, and return -1 to indicate an error.
     */
    off_t newPosition = lseek(swapfile_fd, first_free_location * page_size, SEEK_SET);
    if (newPosition == -1) {
        perror("ERR");
        close(swapfile_fd);
        return -1;
    }
    //Use the write function to write the contents of ppage to the swap file at the current file offset.
    ssize_t bytes_written = write(swapfile_fd, ppage, page_size);
    if (bytes_written == -1) {
        std::cerr << "Error writing to file." << std::endl;
        //close(swapfile_fd);
        return -1;
    }

    return first_free_location * page_size;
}

/*****************************/
// function is used to count the number of bits required to represent a decimal number in binary.
int sim_mem::count_bits(int decimal_number) {
    if (decimal_number == 0) {
        return 1;  // Special case for 0
    }

    int count = 0;
    int number = abs(decimal_number);  // Ignore the sign for the calculation

    while (number > 0) {
        count++;
        number /= 2;
    }

    return count;
}

/*****************************/
/*
 * function definition of the store function in the sim_mem class.
 * This function is responsible for storing a value into memory at the specified address
 */
void sim_mem::store(int address, char value) {
    std::vector<int> address_vector = convertToBinary(
            address); //converting the decimal address into a binary representation
    MemorySection section = getMemorySection(
            address_vector); //It then determines the memory section based on the address
    int frame_size =
            11 - count_bits(this->page_size); //calculated as the number of bits required to represent the page size.
    int start_index_frame = 2;
    int page_location = binaryToDecimal(address_vector, start_index_frame, frame_size +
                                                                           1); //The page location is calculated by converting a portion of the binary address to decimal, using the binaryToDecimal
    int start_index_offset = (12 - count_bits(this->page_size -
                                              1)); //is calculated based on the number of bits required to represent the page size minus 1.
    int index_to_ram;
    int offset = binaryToDecimal(address_vector, start_index_offset,11);  //The offset is calculated by converting a portion of the binary address to decimal, using the binaryToDecimal function. The start index is start_index_offset, and the size is 11.
    int valid_number_page = 0;
    switch (section) { //check the section and update the num_page
        case TEXT:
            valid_number_page = number_of_text_page;
            if (page_location >= number_of_text_page) {
                printf("ERR\n");
                return;
            }
            break;
        case BSS:
            if (page_location >= number_of_bss_page) {
                printf("ERR\n");
                return;
            }
            break;
        case DATA:
            if (page_location >= number_of_data_page) {
                printf("ERR\n");
                return;
            }
            break;
        case HEAP_STACK:
            if (page_location >= number_of_heap_stack_page) {
                printf("ERR\n");
                return;
            }
            break;
    }
    if (section == TEXT) { // if text error you cant write to text
        printf("ERR\n");
        return;
    }
    if (page_table[section][page_location].valid) {  // if the page is on the memory ram store the value there
        int index_ram = page_table[section][page_location].frame * page_size + offset;
        main_memory[index_ram] = value;
        page_table[section][page_location].dirty = true;
        if(no_more_open_frame<=0){
            for (int i = 0; i < MEMORY_SIZE/page_size; ++i) {
                if(i==page_table[section][page_location].frame){
                    second_chance[i].counter_iteration=MEMORY_SIZE/page_size-1;
                }
                else if(second_chance[i].counter_iteration!=0){
                    second_chance[i].counter_iteration--;
                }
            }
        }
        return;
    } else if (page_table[section][page_location].dirty) {//if the page is dirty he is in the swap file
        off_t start;
        char page_temporary[page_size];
        read_page_from_file(swapfile_fd, page_table[section][page_location].swap_index, this->page_size,
                            page_temporary); // read from swap the value
        int new_free_spot = (page_table[section][page_location].swap_index) / page_size; // find store the new free spot
        char zeros[page_size];
        memset(zeros, 0, page_size);
        write_to_swap(zeros); // write zero to emtpy space
        swap_free_location[new_free_spot] = true;

        char zeroes[page_size];
        memset(zeroes, 0, page_size);
        int swap_location = write_to_swap(zeroes);
        index_to_ram = update_Ram_Memory(section, page_location); // return which frame is open in the ram
        for (int i = 0; i < page_size; ++i) {
            main_memory[index_to_ram * page_size + i] = page_temporary[i];
        }
        page_table[section][page_location].frame = index_to_ram;
        page_table[section][page_location].valid = true;
        main_memory[index_to_ram * page_size + offset] = value;
        page_table[section][page_location].dirty = true;
        page_table[section][page_location].swap_index=-1;
        return;
    } else {
        int section_temp = section;
        // Declare the variables outside the switch statement
        off_t start;
        char page_temporary[page_size];
        int index_to_ram;

        // Switch statement to check the value of section_1
        switch (section_temp) {
            case 1: //DATA
                start = page_size * number_of_text_page + page_location * page_size;  // Starting offset
                read_page_from_file(program_fd, start, this->page_size, page_temporary); // read from exec file
                if ((page_location + 1) * page_size > data_size) {
                    int gap = (page_location + 1) * page_size - data_size;
                    for (int i = 0; i < gap; ++i) {
                        page_temporary[page_size - 1 - i] = 0;
                    }
                }
                index_to_ram = update_Ram_Memory(section, page_location); // return which frame is open in the ram
                for (int i = 0; i < page_size; ++i) {
                    main_memory[index_to_ram * page_size + i] = page_temporary[i];
                }
                page_table[section][page_location].frame = index_to_ram;
                page_table[section][page_location].valid = true;
                main_memory[index_to_ram * page_size + offset] = value;
                page_table[section][page_location].dirty = true;
                break;

            case 2: //BSS
                index_to_ram = update_Ram_Memory(section, page_location); // return which frame is open in the ram
                for (int i = 0; i < page_size; ++i) {
                    main_memory[index_to_ram * page_size + i] = '0';
                }
                page_table[section][page_location].frame = index_to_ram;
                page_table[section][page_location].valid = true;
                main_memory[index_to_ram * page_size + offset] = value;
                page_table[section][page_location].dirty = true;
                break;
            case 3: // HEAP_STACK
                index_to_ram = update_Ram_Memory(section, page_location); // return which frame is open in the ram
                for (int i = 0; i < page_size; ++i) {
                    main_memory[index_to_ram * page_size + i] = '0';
                }
                page_table[section][page_location].frame = index_to_ram;
                page_table[section][page_location].valid = true;
                main_memory[index_to_ram * page_size + offset] = value;
                page_table[section][page_location].dirty = true;
                break;
        }
    }
}