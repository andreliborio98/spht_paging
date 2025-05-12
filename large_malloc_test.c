// cd ~/projs/test_nvhtm_wait_phase/nvhtm; rm src/global_structs.c; cp src/global_structs_atomic.c src/global_structs.c; make clean; ./get_cpu_data.sh; make; cd -
// cd ~/projs/test_nvhtm_wait_phase; g++ -o mtest malloc_test.c -I deps/arch_dep/include -L ./nvhtm -l nvhtm -L ./deps/htm_alg -l htm_sgl -L ./deps/threading -l threading -lpthread -L ./deps/input_handler -l input_handler -L ./deps/tinystm/lib -l stm -lm

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "nvhtm/include/global_structs.h"

#define MEM_1GB 1073741824L
#define MEM_128GB 137438953472L
#define MEM_150GB 161061273600L
#define MEM_256GB 274877906944L
#define MEM_384GB 412316860416L

#define MEMSIZE 412316860416L

int main() {
    char* sharedPool;
    long nbThreads = 2;
    char* privatePools[nbThreads];
    long i, j;

    global_structs_init(
        nbThreads, /* nbThreads */
        0, /* nbReplayers */
        0, /* allocEpochs */
        10, /* allocLogSize */
        MEMSIZE / nbThreads, /* localMallocSize */
        0, /* sharedMallocSize */
        0, /* spinsFlush */
        (int*)G_PINNING_1, /* *pinning */
        (int*)G_NUMA_PINNING, /* *numa_nodes */
        (char**)NVRAM_REGIONS /* *nvram_regions[] */
    );

    for (i = 0; i < nbThreads; i++) {
        privatePools[i] = (char*) nvmalloc_local(i, MEMSIZE / nbThreads);

        for (j = 0; j < MEMSIZE / nbThreads; j += (4096 * 16)) {
            privatePools[i][j] = 42;
        }
    }

    // sharedPool = malloc(MEMSIZE * sizeof(char));
    // sharedPool = (char*) nvmalloc(MEMSIZE * sizeof(char));
    // for (i = 0; i < MEMSIZE; i += (4096 * 16)) {
    //     sharedPool[i] = 42;
    // }

    printf("Didn't crash.\n");

    return 0;
}
