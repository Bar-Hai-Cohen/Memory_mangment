#include <iostream>
#include "sim_mem.h"

int main() {
    string a;
    string b;
    string c;
    string d;


    //Use to be 4 page
    for (int i = 0; i < 64; ++i) {
        a += 'a';
        b += 'b';
        c += 'c';
        d += 'd';
    }

    string final = a + b + c + d;
    // printf("%s", a.c_str());
    // Create an ofstream object
    std::ofstream file;
    // Open the file for writing
    file.open("new_file.txt");

    // Check if the file was successfully opened
    if (!file.is_open()) {
        std::cout << "Failed to create the file!" << std::endl;
        return 1;
    }

    // Write data into the file
    file << final << std::endl;

    // Close the file
    file.close();

    char val, val2, val3, val4, val5, val6, val7, val8;
    //char exe_file_name[] = "exec_file";
    char swap_file_name[] = "swap_file";

/***********====1===*****************/

    /*
     * page size
     * number of ta-vim in one page
     *
     * num of pages
     *
     *  64 -  one page
     *  128 -  two page
     *  256 - four page
     */

    sim_mem mem_sm((char*)"new_file.txt", swap_file_name, 128, 128, 64, 64, 64);
    //===Load===

    //Text
    val = mem_sm.load(3); // --> aaa ( first page 1 text)
    val2 = mem_sm.load(65); // --> bbb ( two page 2 text )

    //val3=mem_sm.load(150); // --> error

    //DATA
    val3 = mem_sm.load(1025); // --> ccc (first page 1 data)
    val4 = mem_sm.load(1088); // --> ddd ( two page 2 data )

    //BSS
    val5 = mem_sm.load(2048); // --> _ (first page 1 bss )

    //HEAP_STACK
    //val6=mem_sm.load(3072); // --> error heap_stack

    //===Store===

    //mem_sm.store(5,'x'); // -- > ERROR
    mem_sm.store(1087,'X'); // -- > last index in last page C (data -- > val3)

    //============test swap=======
    mem_sm.store(2048, 'X'); // -- > _ (bss -- > val5) -- > dirty now
    val3 = mem_sm.load(1025); // --> ccc (first page 1 data)
    val4 = mem_sm.load(1088); // --> ddd ( two page 2 data )
    val = mem_sm.load(3); // --> aaa ( first page 1 text)
    //====OUT:d..c..a
    //SWAP:bss
    mem_sm.store(1025, 'Y'); // -- > ccc (data -- > val4) -- > dirty now
    val2 = mem_sm.load(65); // --> bbb ( two page 2 text )
    //====OUT:c..b..a
    //SWAP:bss and ccc
    mem_sm.store(1026, 'W'); // -- > ccc (data -- > val4) -- > need to come from swap
    val2 = mem_sm.load(2048); // --> bss ( two page 2 text )
    //====OUT:bss..c..a
    //SWAP:bss
    //============test swap=======

    mem_sm.store(2114, 'R'); // -- > error , not vaild address

    mem_sm.store(1090, 'R');






    mem_sm.print_memory();
    mem_sm.print_swap();
    mem_sm.print_page_table();

/***********====2===*****************/






}