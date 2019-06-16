#include "VirtualMemory.h"

#include <cstdio>
#include <cassert>
#include <iostream>

using std::cout;
using std::endl;

int main(int argc, char **argv) {
    cout << "Started test" << endl;
    std::cout << "TABLE_DEPTH: " << TABLES_DEPTH <<std::endl ;
    std::cout << "NUM_PAGES: " << NUM_PAGES <<std::endl ;
    std::cout << "NUM_FRAMES: " << NUM_FRAMES <<std::endl ;
    std::cout << "PAGE_SIZE: " <<  PAGE_SIZE<<std::endl ;
    std::cout << "VIRTUAL_MEMORY_SIZE: " <<  VIRTUAL_MEMORY_SIZE<<std::endl ;
    std::cout << "RAM_SIZE: " <<  RAM_SIZE<<std::endl ;
    fflush(stdout);

    VMinitialize();
    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
        printf("writing to %llu\n", (long long int) i);
        VMwrite(5 * i * PAGE_SIZE, i);
    }

    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
        word_t value;
        VMread(5 * i * PAGE_SIZE, &value);
        printf("reading from %llu %d\n", (long long int) i, value);
        assert(uint64_t(value) == i);
    }
    printf("success\n");

    return 0;
}