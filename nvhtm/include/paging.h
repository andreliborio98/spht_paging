#ifndef PAGING_H_GUARD
#define PAGING_H_GUARD
// #ifdef __linux__
// #define _POSIX_C_SOURCE 200112L
// #define _XOPEN_SOURCE   600
// #define _BSD_SOURCE     1
// #define _GNU_SOURCE     1
// #define _DEFAULT_SOURCE 1
// // #define USE_ZIPF
// #endif /* __linux__ */

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "global_structs.h"

// #ifdef __linux__
// 	#include <unistd.h>
// #else /* !__linux__ */
// 	#error "TODO: implement a usleep function"
// #endif /* __linux__ */

// #include "htm_impl.h"

// #define HEAP_START_ADDR  0x0000001000000000L
// #define MEMORY_HEAP_FILE "./somefile.heap"

// default values (can be changed through command line)
//  #define MEMORY_HEAP_SIZE 262144  // total memory
//  #define MEMORY_HEAP_MMAP 262144  // working memory
//  #define PERC_MEMORY_HEAP_MMAP 70 // working memory percentage

struct paging_init_var
{
  void *HEAP_START_ADDR;     // TODO: the kernel may not allow (most likely it will)
  uint64_t memory_heap_size; // total memory
  uint64_t memory_heap_mmap; // default
  float addPageThreshold;    // input is here
  float rmPageThreshold;     // input is here
  int fd; //global structs' file descriptor
};

// struct paging_init_var Paging;

extern int waitForReplayer;

// extern int NB_THREADS; //       = 12;
// uint64_t memory_heap_size = MEMORY_HEAP_SIZE;
////uint64_t memory_heap_mmap = MEMORY_HEAP_MMAP;
// extern uint64_t perc_memory_heap_mmap; // = PERC_MEMORY_HEAP_MMAP;
// extern uint64_t memory_heap_mmap; // = 262144;
// extern long PagerOutActivations;// = 0;

// #define DEBUG_PAGE
// #ifdef DEBUG_PAGE
// 	extern uint64_t nb_page_in;// = 0;
// 	extern uint64_t nb_page_out;// = 0;
// #endif

// extern pthread_cond_t cv_not_enough_pages;// = PTHREAD_COND_INITIALIZER;
// extern pthread_mutex_t mt_memory_used; // = PTHREAD_MUTEX_INITIALIZER;
// extern float addPageThreshold;// = .9;
// extern float rmPageThreshold;// = .9;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  void paging_init(struct paging_init_var Paging);

  void paging_reset_stats();

  void paging_finish();

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PAGING_H_GUARD */
