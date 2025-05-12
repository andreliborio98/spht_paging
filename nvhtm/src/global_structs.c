/* This document contains the basis for memory allocation, regarding to logs and actual transactions*/

// SHARED = memory pool is shared through all threads
/*

*/
// PRIVATE = memory pool is restricted to each thread
/*
 */

//"Comment:" tag shows if the commentary is part of this documentation
// Questions are tagged with "C?"
// definition + C? = couldnt explain properly
//////////////////////////////////////////////////////////////////////////////////////////////////////

#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 600
#define _BSD_SOURCE 1
#define _GNU_SOURCE 1
#define _DEFAULT_SOURCE 1

#include "global_structs.h"
#include "impl.h" /* also includes global_structs */
#include "containers.h"
#include "htm_impl.h"
///#ifdef USE_PAGING
///  #include "paging.h"
///#endif
// #include "hashmap.h"

#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <string.h>

// TODO: just added this for vcode to stop complaining (not needed)
// #include "htm_arch.h"

// Comment: Determines cache line size
//#define USE_REPLAYER //TODO, FIX AND BUT IT AS COMP FLAG (MAKEFILE)

#ifndef ARCH_CACHE_LINE_SIZE
#define ARCH_CACHE_LINE_SIZE 64
#endif

#ifndef NVMALLOC_FILE
#define NVMALLOC_FILE "nvmalloc_file"
#endif /* NVMALLOC_FILE */

//#ifndef NVMALLOC_THREAD_PRIV_FILE
//#define NVMALLOC_THREAD_PRIV_FILE "andre_nvmalloc_file_priv_t"
//#endif /* NVMALLOC_THREAD_PRIV_FILE */

//#ifndef NVMALLOC_THREAD_SHAR_FILE
//#define NVMALLOC_THREAD_SHAR_FILE "andre_nvmalloc_file_shar_t"
//#endif /* NVMALLOC_THREAD_SHAR_FILE */

//breaks paging.c.... why?
// #ifndef MEMORY_HEAP_FILE
// #define MEMORY_HEAP_FILE "andre_nvmalloc_file_heap"
// #endif /* MEMORY_HEAP_FILE */

#ifndef NVMALLOC_SIZE
#define NVMALLOC_SIZE 16777216L /* 1048576L - 1MB */
#endif                          /* NVMALLOC_SIZE */

#ifndef NVMALLOC_THREAD_PRIV_SIZE
#define NVMALLOC_THREAD_PRIV_SIZE 3221225472L /* 1GB */
#endif                                        /* NVMALLOC_THREAD_PRIV_SIZE */

#ifndef NVMALLOC_THREAD_SHAR_SIZE
#define NVMALLOC_THREAD_SHAR_SIZE 1048576L /* 1MB + logs to be defined in init */
#endif                                     /* NVMALLOC_THREAD_SHAR_SIZE */

/*extern*/ __thread long nbTransactions = 0;

static uint64_t nvmalloc_count = 0;
static void *nvmalloc0_base_ptr;
static void *nvmalloc0_current_ptr;
static void *nvmalloc1_base_ptr;
static void *nvmalloc1_current_ptr;
static size_t nvmalloc_size = NVMALLOC_SIZE;

static void **nvmalloc_thr_priv_base_ptr;
static void **nvmalloc_thr_shar_base_ptr;
static __thread void *nvmalloc_thr_priv_base_ptr2 = NULL;
static __thread void *nvmalloc_thr_priv_current_ptr = NULL;
static __thread void *nvmalloc_thr_shar_base_ptr2 = NULL;
static __thread void *nvmalloc_thr_shar_current_ptr = NULL;
static long nvmalloc_thr_priv_size = NVMALLOC_THREAD_PRIV_SIZE;
static long nvmalloc_thr_shar_size = NVMALLOC_THREAD_SHAR_SIZE;

static int pidChildProc;

// used to grap information about priv memory used
static __thread uint64_t myId = 0;

volatile __thread uint64_t timeTotalTS1 = 0;
volatile __thread uint64_t timeAfterTXTS1 = 0;
volatile __thread uint64_t timeTotalTS2 = 0;
volatile __thread uint64_t timeTotal = 0;

__thread uint64_t timeSGL_TS1 = 0;
__thread uint64_t timeSGL_TS2 = 0;
__thread uint64_t timeSGL = 0;
uint64_t timeSGL_global = 0;

__thread uint64_t timeAbortedTX_TS1 = 0;
__thread uint64_t timeAbortedTX_TS2 = 0;
__thread uint64_t timeAbortedTX = 0;
uint64_t timeAbortedTX_global = 0;

volatile __thread uint64_t timeAfterTXSuc = 0;
volatile __thread uint64_t timeAfterTXFail = 0;

static void *nvramRanges[1024];
static int nbNvramRanges;

static const int EPOCH_TIMOUT = 32;

/* extern */void(*on_before_non_tx_write)(int threadId, void *addr, uint64_t val); //sgl or outside


// Comment: Sets pinning for the CPU cores, check when NUMA and logical cores behaviour are of interest

// Comment: 1. Thread pinning

// pinning matrix, G_PINNING[threadID] <-- coreID
// const int G_PINNING_0[] = { // intel14_v1
//    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,
//   14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
//   28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
//   42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55
// };
// const int G_PINNING_0[] = { // ngstorage (NUMA not available)
//    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
//   20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39
// };

/* extern */ const int G_PINNING_0[] = { // nvram (12C/24T +NUMA) - node05
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};

// const int G_PINNING_1[] = { // intel14_v1 (HT first, NUMA second)
//     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,
//    28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
//    14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
//    42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55
//  };
// const int G_PINNING_1[] = { // node02
//    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17,
//   36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
//   18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
//   54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71
// };
// const int G_PINNING_1[] = { // ngstorage (NUMA not available)
//   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
//  20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39
// };
/* extern */ const int G_PINNING_1[] = { // nvram (12C/24T +NUMA) - node05
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};

// const int G_PINNING_2[] = { // intel14_v1 (pair with HT)
//   0,  28, 1,  29, 2,  30, 3,  31, 4,  32, 5,  33, 6,  34,
//   7,  35, 8,  36, 9,  37, 10, 38, 11, 39, 12, 40, 13, 41,
//   14, 42, 15, 43, 16, 44, 17, 45, 18, 46, 19, 47, 20, 48,
//   21, 49, 22, 50, 23, 51, 24, 52, 25, 53, 26, 54, 27, 55
// };
// const int G_PINNING_2[] = { // ngstorage (NUMA not available)
//   0,  20, 1,  21, 2,  22, 3,  23, 4,  24, 5,  25, 6,  26,
//   7,  27, 8,  28, 9,  29, 10, 30, 11, 31, 12, 32, 13, 33,
//   14, 34, 15, 35, 16, 36, 17, 37, 18, 38, 19, 39
// };
/* extern */ const int G_PINNING_2[] = { // nvram (12C/24T +NUMA) - node05
    0, 24, 1, 25, 2, 26, 3, 27, 4, 28, 5, 29,
    6, 30, 7, 31, 8, 32, 9, 33, 10, 34, 11, 35,
    12, 36, 13, 37, 14, 38, 15, 39, 16, 40, 17, 41,
    18, 42, 19, 43, 20, 44, 21, 45, 22, 46, 23, 47};

// Comment: 2. NUMA pinning

// NUMA matrix G_NUMA_PINNING[coreID] <-- numa_node_id
// /* extern */ const int G_NUMA_PINNING[] = { // nvram (12C/24T +NUMA) - node05
    // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

 /* extern */const int G_NUMA_PINNING[] = { // nvram (12C/24T +NUMA) - node05
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1
 };

///* extern */const int G_NUMA_PINNING[] = { // nvram (12C/24T +NUMA) - node05
//   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
//   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
//   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
//   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1
//};

// Comment: If using PM, use its directory, in the case of NODE05, "/mnt/nvram", if not, use "./pmdata"
//#ifndef PM_DIR
//#define PM_DIR "/mnt/nvram"
//#endif /* PM_DIR */

// Comment: Contextualizes the number of existing NUMA nodes/nvram regions
/* extern */ const char *NVRAM_REGIONS[] = {PM_DIR "0/", PM_DIR "1/"};

/* extern */ wait_commit_fn_t wait_commit_fn = wait_commit_pc_simple;
/* extern */ prune_log_fn_t try_prune_log_fn = try_prune_log_epoch_impa;
/* extern */ prune_log_fn_t prune_log_fn = prune_log_forward_epoch_impa;

// one must call install_bindings_<solution> to set this
/* extern */ void (*on_htm_abort)(int threadId);
/* extern */ void (*on_before_htm_begin)(int threadId);
/* extern */ void (*on_before_htm_write)(int threadId, void *addr, uint64_t val);
/* extern */ void (*on_before_htm_commit)(int threadId);
/* extern */ void (*on_after_htm_commit)(int threadId);
/*extern */ replay_log_next_entry_s (*log_replay_next_tx_search)();
/*extern */ uint64_t (*log_replay_next_tx_apply)(replay_log_next_entry_s curPtr);

/* extern */ void (*on_after_sgl_begin)(int threadId);
/* extern */ void (*on_before_sgl_commit)(int threadId);

// 1 cache line per thread to flag the current state
/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
large_cache_line_s *gs_ts_array;

/*extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
large_cache_line_s *G_observed_ts;

/*extern */ volatile pcwc_info_s **gs_pcwc_info;

/*extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
large_cache_line_s *P_last_safe_ts;

/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
uint64_t **P_epoch_ts;

/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
uint64_t *P_epoch_persistent; /* persistent */

/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
cache_line_s *G_next;

/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
cache_line_s *gs_appInfo;

/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
cache_line_s gs_log_data;

/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
uint64_t **P_write_log;

/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE))) int P_start_epoch = 0; /* persistent */

/* extern */ volatile int *G_epoch_lock;

// TODO: these are also extern
/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
int32_t *G_flag_checkpointer_exit;
/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
int32_t *G_flag_checkpointer_ready;
/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
int32_t *G_flag_checkpointer_done;
/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
cache_line_s **G_flag_checkpointer_G_next;
/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
uint64_t ***G_flag_checkpointer_P_write_log;
/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
uint64_t **G_flag_checkpointer_P_last_safe_ts;
/* extern */ volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
uint64_t **G_flag_checkpointer_P_last_safe_ts;

/* extern */ volatile __thread void (*onBeforeWrite)(int, void *, uint64_t); /* = on_before_htm_write*/
/* extern */ volatile __thread void (*onBeforeHtmCommit)(int);               /* = on_before_htm_commit */
/* extern */ volatile __thread uint64_t *write_log_thread;                   /* = &(P_write_log[threadId][0]); */

/*
Comment: allocateInNVRAM = does proper memory allocation
memRegion = which nvram region (0 or 1)
file = logMallocFile (0 or 1)
bytes = qtt of bytes needed to be allocated
mapFlag = MAP_SHARED_VALIDATE|MAP_SYNC|MAP_SHARED
addr = which addr data will be allocated
*/
static void *allocateInNVRAM(const char *memRegion, const char *file, size_t bytes, long mapFlag, void *addr)
{
  char fileNameBuffer[1024];
  void *res = NULL;
  int fd;


  sprintf(fileNameBuffer, "%s%s", memRegion, file);
  fd = open(fileNameBuffer, O_CREAT | O_TRUNC | O_RDWR, 0666);
  close(fd); // writes the permissions
  fd = open(fileNameBuffer, O_CREAT | O_RDWR, 0666);

  if (fd == -1)
  {
    fprintf(stderr, "Error open file %s: %s\n", fileNameBuffer, strerror(errno));
  }

  // TODO: ftruncate needed after munmap...
  if (ftruncate(fd, bytes))
  { // if address != NULL there was a ftruncate before
    fprintf(stderr, "Error ftruncate file %s: %s\n", fileNameBuffer, strerror(errno));
  }
  // Comment: If address if not occupied, mmap it
  if (addr != NULL)
  {
    res = mmap(addr, bytes, PROT_READ | PROT_WRITE, mapFlag | MAP_FIXED, fd, 0);
    if (res != addr)
    {
      fprintf(stderr, "Error getting the requested address %p (got %p): %s\n", addr, res, strerror(errno));
    }
    // Comment: Else, write to null address C? (why?)
  }
  else
  {
    res = mmap(NULL, bytes, PROT_READ | PROT_WRITE, mapFlag, fd, 0);
  }
  
  if (res == (void *)-1 || res == NULL)
  {
    fprintf(stderr, "Error mmapping file %s: %s\n", fileNameBuffer, strerror(errno));
  }
  // printf("Tamanho mmap: %lu\n", bytes);

  return res;
}

void *internal_replay_log_apply_fn(void *replayerId); // implemented on impl_log_replayer.cpp

static void checkpointer()
{
  // TODO: pin this thread to some unused slot
  __atomic_store_n(G_flag_checkpointer_ready, 1, __ATOMIC_RELEASE);

  // #if 0
  pthread_t threads[gs_appInfo->info.nbReplayers];
  if (log_replay_flags & LOG_REPLAY_PARALLEL)
  {
    for (int i = 0; i < gs_appInfo->info.nbReplayers - 1; i++)
    {
      pthread_create(&(threads[i]), NULL, internal_replay_log_apply_fn, (void *)((uintptr_t)i + 1));
    }
  }

  if (log_replay_flags == 0 || isCraftySet)
  {
    __atomic_store_n(G_flag_checkpointer_done, 1, __ATOMIC_RELEASE);
    exit(EXIT_SUCCESS);
  }

  if (!(log_replay_flags & (LOG_REPLAY_CC_HTM | LOG_REPLAY_CONCURRENT)))
  {
    while (!__atomic_load_n(G_flag_checkpointer_exit, __ATOMIC_ACQUIRE))
    {
      pthread_yield();
      usleep(100);
    }
  }
  // after exit replay the log (assumes very long logs)

  //   /* ----------------------------- */
  // // fork again for perf
  // if (fork() == 0) {
  //   char buf[1024];
  //   sprintf(buf, "perf stat -p %d -e L1-dcache-load-misses,L1-dcache-loads,LLC-load-misses,LLC-loads sleep 5 ", pidChildProc);
  //   printf("[PERF] attaching to proc %d\n!", pidChildProc);
  //   execl("/bin/sh", "sh", "-c", buf, NULL);
  //   // done with this code
  //   exit(0);
  // }
  // /* ----------------------------- */

  // TODO: strange bug: the log is not visible on replay_log_init
  while (!(__atomic_load_n(G_flag_checkpointer_P_write_log, __ATOMIC_ACQUIRE)))
    ; // must not be NULL!

  replay_log_init(gs_appInfo->info.nbThreads, gs_appInfo->info.nbReplayers, gs_appInfo->info.allocLogSize,
                  *G_flag_checkpointer_G_next, *G_flag_checkpointer_P_write_log, *G_flag_checkpointer_P_last_safe_ts,
                  ccHTM_Q, log_replay_flags, nvramRanges, nbNvramRanges);

  replay_log_apply();

  if (log_replay_flags & LOG_REPLAY_PARALLEL)
  {
    for (int i = 0; i < gs_appInfo->info.nbReplayers - 1; i++)
    {
      pthread_join(threads[i], NULL);
    }
  }

  replay_log_print_stats(log_replay_stats_file);
  // #endif

  __atomic_store_n(G_flag_checkpointer_done, 1, __ATOMIC_RELEASE);
  exit(EXIT_SUCCESS);
}

/*
Comment: nvalloc_init = gets the right addresses for memory allocation
nbThreads = number of threads
logBytesPerThread = bytes allocated to log per thread
sharedBytes = sharedMallocSize
pinning = check global_structs_init
numa_nodes = check global_structs_init
nvram_regions = check global_structs_init
*/
void *nvmalloc_init(
    int nbThreads,
    uint64_t logBytesPerThread,
    uint64_t sharedBytes,
    int *pinning,
    int *numa_nodes,
    char *nvram_regions[])
{
  // These assignments cause issues with large heap allocation on synthetic benchmarks, but they might be required for STAMP/TPC-C tests
  // TODO: Move this initialization to bench/backends/nvhtm/global_structs_init() in tm.h
  // - 'sharedMallocSize' initialized from global_structs_init() is used for nvmalloc and probably needs to be updated as well

  // nvmalloc_thr_priv_size = NVMALLOC_THREAD_PRIV_SIZE;
  // nvmalloc_thr_shar_size = NVMALLOC_THREAD_SHAR_SIZE;

  // TODO: fork the process here, map shared in the child process (checkpointer), and private in the father
  volatile intptr_t readSharedMem = 0;

  // put this in a non-DAX fs to make use of shared DRAM mappings
  G_flag_checkpointer_exit = (int *)allocateInNVRAM("./", "flag_checkpointer",
                                                    4096 /* var space */ + sizeof(cc_htm_queue_s), MAP_SHARED, NULL);
  char *beginFlags = (char *)G_flag_checkpointer_exit;
  int sizeOfExitFlag = sizeof(__typeof__(G_flag_checkpointer_exit));
  G_flag_checkpointer_ready = (__typeof__(G_flag_checkpointer_ready))(beginFlags + sizeOfExitFlag);
  G_flag_checkpointer_done = (__typeof__(G_flag_checkpointer_done))(beginFlags + 2 * sizeOfExitFlag);
  G_flag_checkpointer_P_write_log = (__typeof__(G_flag_checkpointer_P_write_log))(beginFlags + 4 * sizeOfExitFlag);
  G_flag_checkpointer_P_last_safe_ts = (__typeof__(G_flag_checkpointer_P_last_safe_ts))(beginFlags + 6 * sizeOfExitFlag);
  G_flag_checkpointer_G_next = (__typeof__(G_flag_checkpointer_G_next))(beginFlags + 8 * sizeOfExitFlag); // sizeof(int32_t) != sizeof(void*)
  ccHTM_Q = (__typeof__(ccHTM_Q))(beginFlags + 4096);

  ccHTM_Q->redHeadIdx = CC_HTM_RED_Q_HEAD;
  ccHTM_Q->txCounter = 0;
  ccHTM_Q->txMin = 1;

  *G_flag_checkpointer_exit = 0;
  *G_flag_checkpointer_ready = 0;
  *G_flag_checkpointer_done = 0;

  // Comment: allocate base memory values for priv and shar configs, based on the number of threads
  nvmalloc_thr_priv_base_ptr = malloc(nbThreads * sizeof(void *));
  nvmalloc_thr_shar_base_ptr = malloc(nbThreads * sizeof(void *));
  // Comment: shared, add to the previous define (1048576L) pool, but logBytesPerThread is empty here
  nvmalloc_thr_shar_size += logBytesPerThread;
  // Comment: priv, increments define (16777216L) with sharedBytes if needed, will add up to allocateInNvram later
  //   nvmalloc_size += sharedBytes;
  nvmalloc_size = sharedBytes;

  char localMallocFile0[1024];
  char localMallocFile1[1024];
  sprintf(localMallocFile0, "%s%i", NVMALLOC_THREAD_PRIV_FILE, 0);
  sprintf(localMallocFile1, "%s%i", NVMALLOC_THREAD_PRIV_FILE, 1);

  // poolAreas should be equal to the number of threads per NUMA, but this raises complications
  // in systems with multiple NUMAs since the number of threads per NUMA needs to match
  // the allocated space.
  // long poolAreas = nbThreads / NUMAs;

  // Determine how many pool areas are assigned to each NUMA node
  long i = 0;
  long poolAreasNUMA0 = 0;
  long poolAreasNUMA1 = 0;
  for (i = 0; i < nbThreads; i++)
  {
    if (numa_nodes[i] == 0)
    {
      poolAreasNUMA0++;
    }
    else if (numa_nodes[i] == 1)
    {
      poolAreasNUMA1++;
    }
  }

  // Ensure there's always at least 1 pool per NUMA node
  poolAreasNUMA0 = poolAreasNUMA0 == 0 ? 1 : poolAreasNUMA0;
  poolAreasNUMA1 = poolAreasNUMA1 == 0 ? 1 : poolAreasNUMA1;

  // long poolAreas = nbThreads <= 1 ? 1 : 32; // TODO
  // long poolAreasNUMA0 = nbThreads <= 32 ? nbThreads : 32;
  // long poolAreasNUMA1 = nbThreads > 32 ? 1 : nbThreads - 32;

  // Comment: MAP_PRIVATE is not defined here... uses null? C?
  long mapFlag = MAP_SHARED;
  if (isCraftySet || isSharedHTM)
    mapFlag = /*MAP_SHARED_VALIDATE|MAP_SYNC*/ MAP_SHARED;

  // Comment: allocate memory pool for each NUMA node (priv)
  //  printf("Esse allocate!\n");
  // printf ("node0\n");
  void *addrNUMA0 = allocateInNVRAM(nvram_regions[0], localMallocFile0, nvmalloc_thr_priv_size * poolAreasNUMA0 + nvmalloc_size / 2, mapFlag, NULL);
  // printf("node1\n");
  void *addrNUMA1 = allocateInNVRAM(nvram_regions[1], localMallocFile1, nvmalloc_thr_priv_size * poolAreasNUMA1 + nvmalloc_size / 2, mapFlag, NULL);

  // Comment: determines base addr for both NUMA pools (nvmalloc_thr_priv_size = define)
  //  TODO: test if using HTM on MAP_SHARED is slower
  nvmalloc0_base_ptr = ((char *)addrNUMA0) + nvmalloc_thr_priv_size * poolAreasNUMA0;
  nvmalloc1_base_ptr = ((char *)addrNUMA1) + nvmalloc_thr_priv_size * poolAreasNUMA1;
  // for (long i = 0; i < nvmalloc_size / sizeof(intptr_t); i += 4096 / sizeof(intptr_t)) {
  //   readSharedMem = ((intptr_t*)nvmalloc_base_ptr)[i];
  //   __atomic_store_n(&((intptr_t*)nvmalloc_base_ptr)[i], readSharedMem, __ATOMIC_RELEASE);
  //   if (i > 1048576) break;
  // }

  // Comment: copies to current var so it can change accordingly and still keep the base value
  nvmalloc0_current_ptr = nvmalloc0_base_ptr;
  nvmalloc1_current_ptr = nvmalloc1_base_ptr;

  char logMallocFile0[1024];
  char logMallocFile1[1024];
  sprintf(logMallocFile0, "%s%i", NVMALLOC_THREAD_SHAR_FILE, 0);
  sprintf(logMallocFile1, "%s%i", NVMALLOC_THREAD_SHAR_FILE, 1);

  // Comment: does memory allocation for the shared pool variant
  void *sharNUMA0 = allocateInNVRAM(nvram_regions[0], logMallocFile0,
                                    nvmalloc_thr_shar_size * poolAreasNUMA0, /*MAP_SHARED_VALIDATE|MAP_SYNC*/ MAP_SHARED, NULL);
  void *sharNUMA1 = allocateInNVRAM(nvram_regions[1], logMallocFile1,
                                    nvmalloc_thr_shar_size * poolAreasNUMA1, /*MAP_SHARED_VALIDATE|MAP_SYNC*/ MAP_SHARED, NULL);

  int countNUMAThr0 = 0;
  int countNUMAThr1 = 0;
  // Comment: for each thread, determine an ID to each one depending on the NUMA node
  for (int i = 0; i < nbThreads; ++i)
  {
    // int coreId = pinning[i];
    // int nodeId = numa_nodes[coreId];
    int nodeId = numa_nodes[i];
    // int nodeId = i % 2; // TODO: this one is round-robin

    // Comment: if == 0, its considered to be in NUMA node 1 (NUMA0), and keeps counting the threads in that NUMA node
    // if != 0, its in NUMA node 2 (NUMA1), and has the same counting behaviour
    if (nodeId == 0)
    {
      nvmalloc_thr_priv_base_ptr[i] = ((char *)addrNUMA0) + nvmalloc_thr_priv_size * countNUMAThr0;
      nvmalloc_thr_shar_base_ptr[i] = ((char *)sharNUMA0) + nvmalloc_thr_shar_size * countNUMAThr0;
      // for (long j = 0; j < nvmalloc_thr_priv_size / sizeof(intptr_t) - 1; j += 4096 / sizeof(intptr_t)) {
      //   readSharedMem = ((intptr_t*)nvmalloc_thr_priv_base_ptr[countNUMAThr0])[j];
      //   __atomic_store_n(&((intptr_t*)nvmalloc_thr_priv_base_ptr[countNUMAThr0])[j], readSharedMem, __ATOMIC_RELEASE);
      //   // if (j > 1048576) break;
      // }
      countNUMAThr0++;
    }
    else
    {
      nvmalloc_thr_priv_base_ptr[i] = ((char *)addrNUMA1) + nvmalloc_thr_priv_size * countNUMAThr1;
      nvmalloc_thr_shar_base_ptr[i] = ((char *)sharNUMA1) + nvmalloc_thr_shar_size * countNUMAThr1;
      // for (long j = 0; j < nvmalloc_thr_priv_size / sizeof(intptr_t) - 1; j += 4096 / sizeof(intptr_t)) {
      //   readSharedMem = ((intptr_t*)nvmalloc_thr_priv_base_ptr[countNUMAThr1])[j];
      //   __atomic_store_n(&((intptr_t*)nvmalloc_thr_priv_base_ptr[countNUMAThr1])[j], readSharedMem, __ATOMIC_RELEASE);
      //   // if (j > 1048576) break;
      // }
      countNUMAThr1++;
    }
  }

  // printf("Criando processo de replayer\n");

  // fflush(stdout);
  // fflush(stderr);

//put it even on replay_log-init() but it was after the fork, here it doesnt break either, so it seems good
// printf ("G_flag_checkpointer_exit (SHOULD BE BEFORE FORK)\n");
__atomic_load_n(G_flag_checkpointer_exit, __ATOMIC_ACQUIRE);


  #ifdef USE_REPLAYER
  // Comment: unmap and re-mmap if child process in MAP_SHARED | MAP_FIXED mode
  if (fork() == 0)
  { // child process
    // VERY IMPORTANT!!!
    // munmap the previous addresses and mmap them again in MAP_SHARED | MAP_FIXED mode
    // printf ("FORK REPLAYER\n");
    pidChildProc = getpid();

    // cpu_set_t mask;
    // CPU_ZERO(&mask);
    // CPU_SET(47, &mask);
    // sched_setaffinity(pidChildProc, sizeof(mask), &mask);

    void *prevAddr0 = addrNUMA0;
    void *prevAddr1 = addrNUMA1;

    if (munmap(addrNUMA0, nvmalloc_thr_priv_size * poolAreasNUMA0 + nvmalloc_size / 2))
    {
      fprintf(stderr, "Error munmap %s: %s\n", NVMALLOC_FILE, strerror(errno));
    }
    if (munmap(addrNUMA1, nvmalloc_thr_priv_size * poolAreasNUMA1 + nvmalloc_size / 2))
    {
      fprintf(stderr, "Error munmap %s: %s\n", NVMALLOC_FILE, strerror(errno));
    }

    // Comment: allocate memory (priv) according to NUMA pool
    addrNUMA0 = allocateInNVRAM(nvram_regions[0], localMallocFile0, nvmalloc_thr_priv_size * poolAreasNUMA0 + nvmalloc_size / 2,
                                /*MAP_SHARED_VALIDATE|MAP_SYNC*/ MAP_SHARED, prevAddr0);
    addrNUMA1 = allocateInNVRAM(nvram_regions[1], localMallocFile1, nvmalloc_thr_priv_size * poolAreasNUMA1 + nvmalloc_size / 2,
                                /*MAP_SHARED_VALIDATE|MAP_SYNC*/ MAP_SHARED, prevAddr1);
    // paging_init(addr);

    nvmalloc0_base_ptr = ((char *)addrNUMA0) + nvmalloc_thr_priv_size * poolAreasNUMA0;
    nvmalloc1_base_ptr = ((char *)addrNUMA1) + nvmalloc_thr_priv_size * poolAreasNUMA1;
    nvmalloc0_current_ptr = nvmalloc0_base_ptr;
    nvmalloc1_current_ptr = nvmalloc1_base_ptr;

    nvramRanges[2 * nbNvramRanges] = addrNUMA0;
    nvramRanges[2 * nbNvramRanges + 1] = (void *)(((uintptr_t)addrNUMA0) + nvmalloc_thr_priv_size * poolAreasNUMA0 + nvmalloc_size / 2);
    nbNvramRanges++;

    nvramRanges[2 * nbNvramRanges] = addrNUMA1;
    nvramRanges[2 * nbNvramRanges + 1] = (void *)(((uintptr_t)addrNUMA1) + nvmalloc_thr_priv_size * poolAreasNUMA1 + nvmalloc_size / 2);
    nbNvramRanges++;

    checkpointer();
  } // END if child process

  //  TODO: if the
  while (!__atomic_load_n(G_flag_checkpointer_ready, __ATOMIC_ACQUIRE))
  {
    pthread_yield(); // wait the checkpointer
    usleep(100);
  }
  #endif

  return (void *)(readSharedMem ^ readSharedMem); /* make sure the child reads the mem! */
}

/*
Comment:
threadId: 0 or 1 depending on the NUMA node
bytes: how many bytes it should offset C?
useThreadLocalVars: 0 or 1 (couldnt find definition C?)
isShared: priv (0) or shared (1)
*/
static void *nvmalloc_local_impl(int threadId, size_t bytes, int useThreadLocalVars, int isShared)
{
  intptr_t curPtr, basePtr;
  volatile void *addr;
  // static __thread uint64_t accumulatedBytes = 0;

  // Comment: updates curPtr and basePtr addresses depending on the isShared and useThreadLocalVars variables
  // and sets addr, that will be returned with the current address pointer after the increments
  if (isShared)
  {
    // shared
    if (useThreadLocalVars)
    {
      // Comment: in this case, it uses 2 separate variables shar_base_ptr2 as shar_base_ptr[threadId], that will become a different basePtr, with values not based uniquely on threadId
      if (nvmalloc_thr_shar_current_ptr == NULL)
      {
        myId = threadId;
        nvmalloc_thr_shar_current_ptr = nvmalloc_thr_shar_base_ptr[threadId];
        nvmalloc_thr_shar_base_ptr2 = nvmalloc_thr_shar_base_ptr[threadId];
      }
      addr = nvmalloc_thr_shar_current_ptr;
      // printf("isShared nvlocalimpl %p\n", &addr);
      nvmalloc_thr_shar_current_ptr = (void *)((intptr_t)nvmalloc_thr_shar_current_ptr + bytes);
      curPtr = (intptr_t)nvmalloc_thr_shar_current_ptr;
      basePtr = (intptr_t)nvmalloc_thr_shar_base_ptr2;
    }
    else
    {
      // Comment: points to base shar addr provided by threadId (describes NUMA node)
      addr = nvmalloc_thr_shar_base_ptr[threadId];
      // Comment: Overwrite previour addr with the one with the "bytes" offset applied
      nvmalloc_thr_shar_base_ptr[threadId] = (void *)((intptr_t)nvmalloc_thr_shar_base_ptr[threadId] + bytes);
      // Comment: updates curr and base addresses with the new one
      curPtr = (intptr_t)nvmalloc_thr_shar_base_ptr[threadId];
      basePtr = (intptr_t)nvmalloc_thr_shar_base_ptr[threadId];
    }

    if (curPtr > basePtr + nvmalloc_thr_shar_size)
    {
      addr = (void *)-1;
      fprintf(stderr, "[nvmalloc_local]: shared alloc out of space\n");
    }
  }
  else
  {
    // private
    // Comment: same as shared but with priv variables
    if (useThreadLocalVars)
    {
      if (nvmalloc_thr_priv_current_ptr == NULL)
      {
        myId = threadId;
        nvmalloc_thr_priv_current_ptr = nvmalloc_thr_priv_base_ptr[threadId];
        nvmalloc_thr_priv_base_ptr2 = nvmalloc_thr_priv_base_ptr[threadId];
      }
      addr = nvmalloc_thr_priv_current_ptr;
      // printf("isPriv nvlocalimpl %p\n", &addr);
      nvmalloc_thr_priv_current_ptr = (void *)((intptr_t)nvmalloc_thr_priv_current_ptr + bytes);
      curPtr = (intptr_t)nvmalloc_thr_priv_current_ptr;
      basePtr = (intptr_t)nvmalloc_thr_priv_base_ptr2;
    }
    else
    {
      addr = nvmalloc_thr_priv_base_ptr[threadId];
      nvmalloc_thr_priv_base_ptr[threadId] = (void *)((intptr_t)nvmalloc_thr_priv_base_ptr[threadId] + bytes);
      curPtr = (intptr_t)nvmalloc_thr_priv_base_ptr[threadId];
      basePtr = (intptr_t)nvmalloc_thr_priv_base_ptr[threadId];
    }

    // Comment: checks if it cant do more memory alloc
    if (curPtr > basePtr + nvmalloc_thr_priv_size)
    {
      addr = (void *)-1;
      // printf("nvlocalIfMax %p\n", &addr);
      fprintf(stderr, "[nvmalloc_local]: private alloc out of space (MAX = %lu)\n", nvmalloc_thr_priv_size);
      exit(-1);
    }
  }
  // accumulatedBytes += bytes;
  // printf("[%i] nvmalloc_local = %p (%zu B, acc = %zu)\n", threadId, addr, bytes, accumulatedBytes);
  // printf("nvlocalimplFinaladdr %p\n", &addr);
  return (void *)addr;
}

// Comment: only adds useThreadLocalVars and isShared toggles to nvmalloc_local_impl and returns current mem addr to be allocated
void *nvmalloc_local(int threadId, size_t bytes)
{
  // This only works correctly when used with multiple real threads. There's an issue if nvmalloc_local() or
  // are called with different threadIds from the same real thread since some of the variables used in nvmalloc_local_impl()
  // are __thread local.
  void *addr = nvmalloc_local_impl(threadId, bytes, 1 /* useThreadLocalVars */, 0 /* !isShared */);
  // printf("nvmalloc local: %p\n", &addr);
  // return nvmalloc_local_impl(threadId, bytes, 1/* useThreadLocalVars */, 0/* !isShared */);
  return (void *)addr;
}

// Called as in: P_epoch_ts = nvmalloc(sizeof(uint64_t*) * nbThreads);
// Comment: used to check if out of space after __sync_fetch_and_add
void *nvmalloc(size_t bytes)
{
  intptr_t addr;

  // if (__sync_fetch_and_add(&nvmalloc_count, 1) % 2) {
  // Comment: adds bytes to nvmalloc0_current_ptr addr
  addr = __sync_fetch_and_add((intptr_t *)&nvmalloc0_current_ptr, bytes);
  if (addr + bytes > (intptr_t)nvmalloc0_base_ptr + (nvmalloc_size / 2))
  {
    addr = -1;
    fprintf(stderr, "[nvmalloc]: out of space (total space = %zu, alloc = %zu)\n",
            nvmalloc_size / 2, bytes);
    exit(-1);
  }
  /*
} else {
  addr = __sync_fetch_and_add((intptr_t*)&nvmalloc1_current_ptr, bytes);
  if (addr + bytes > (intptr_t)nvmalloc1_base_ptr + (nvmalloc_size / 2)) {
    addr = -1;
    fprintf(stderr, "[nvmalloc]: out of space (total space = %zu, alloc = %zu)\n",
      nvmalloc_size / 2, bytes);
  }
}
*/
  return (void *)addr;
}

void nvfree(void *ptr)
{
  /* TODO */
}

/*
Comment: Initializes the global structs module, called from nvhtm/src/main.c

nbThreads: number of threads
nbReplayers: number of replayers
allocEpochs: set to 1 in main.c C?
allocLogSize: number of log entries
localMallocSize/nvmalloc_thr_priv_size: local memory region for each thread (memory_heap_size/nb_threads)
sharedMallocSize: total memory size (memory_heap_size)
spinsFlush: defined = 500 C?
pinning: set the pinning being used
numa_nodes: set the NUMA configuration being used
nvram_regions: quantity of NUMA nodes is usually a correct correlation
*/
void global_structs_init(
    int nbThreads,
    int nbReplayers,
    uint64_t allocEpochs,
    uint64_t allocLogSize,
    uint64_t localMallocSize,
    uint64_t sharedMallocSize,
    int spinsFlush,
    int *pinning,
    int *numa_nodes,
    char *nvram_regions[])
{

  // printf("Entrando global_structs_init\n");

  // TODO: check whether the log pointers are persistent or not

  nvmalloc_thr_priv_size = localMallocSize;

  /*Comment:
    HTM_read_only_storage1_size = htm external file C?
    cache_line_s = ARCH_CACHE_LINE_SIZE (define)
    EASY_MALLOC = allocs a simple mem space given the data
  */
  if (HTM_read_only_storage1_size > sizeof(cache_line_s))
  {
    gs_appInfo = (cache_line_s *)HTM_read_only_storage1;
  }
  else
  {
    EASY_MALLOC(gs_appInfo, 1);
  }

  // Comment: gs_appInfo = (cache_line_s*)HTM_read_only_storage1; C?
  gs_appInfo->info.isExit = 0;
  gs_appInfo->info.nbThreads = nbThreads;
  gs_appInfo->info.nbReplayers = nbReplayers;
  gs_appInfo->info.allocEpochs = allocEpochs;
  gs_appInfo->info.allocLogSize = allocLogSize;
  gs_appInfo->info.localMallocSize = localMallocSize;
  gs_appInfo->info.epochTimeout = EPOCH_TIMOUT;
  gs_appInfo->info.spinsFlush = spinsFlush;

  gs_log_data.log.epoch_end = allocEpochs - 1;
  gs_log_data.log.who_is_pruning = -1;

  // Comment: does memory allocation for priv/shared and child processes
  nvmalloc_init(nbThreads, allocLogSize * sizeof(uint64_t) + allocEpochs * sizeof(uint64_t), sharedMallocSize,
                pinning, numa_nodes, nvram_regions);

  // printf("Passou do nvmalloc_init\n");

  // Comment: EASY_MALLOCs
  EASY_MALLOC(gs_ts_array, nbThreads);
  memset((void *)gs_ts_array, 0, sizeof(large_cache_line_s) * nbThreads);

  EASY_MALLOC(G_observed_ts, nbThreads);
  memset((void *)G_observed_ts, 0, sizeof(large_cache_line_s) * nbThreads);

  EASY_MALLOC(gs_pcwc_info, nbThreads);

  // EASY_MALLOC(P_epoch_ts, nbThreads);
  // EASY_MALLOC(P_epoch_persistent, allocEpochs);
  // EASY_MALLOC(P_write_log, nbThreads);

  // Comment: using nvmalloc_local_impl returns an address after incrementing the 2nd argument and copies 0 to this memory space (basePtr->currPtr)
  G_next = nvmalloc_local_impl(0, sizeof(cache_line_s) * nbThreads, 0, 1);
  memset((void *)G_next, 0, sizeof(cache_line_s) * nbThreads);
  *G_flag_checkpointer_G_next = G_next;

  P_epoch_ts = nvmalloc(sizeof(uint64_t *) * nbThreads);
  P_epoch_persistent = nvmalloc(sizeof(uint64_t) * allocEpochs);

  P_write_log = nvmalloc_local_impl(0, sizeof(uint64_t *) * nbThreads, 0, 1);
  *G_flag_checkpointer_P_write_log = P_write_log;

  P_last_safe_ts = nvmalloc_local_impl(0, sizeof(large_cache_line_s), 0, 1);
  *G_flag_checkpointer_P_last_safe_ts = &(P_last_safe_ts->ts);

  memset((void *)P_last_safe_ts, 0, sizeof(large_cache_line_s));

  EASY_MALLOC(phtm_logs, nbThreads);

  EASY_MALLOC(G_epoch_lock, allocEpochs);
  memset((void *)G_epoch_lock, 0, sizeof(int) * allocEpochs);

  // Comment: Log alloc C?
  for (int i = 0; i < nbThreads; ++i)
  {

    // TODO: this is local memory, so make sure this does not hit the same cache-line
    EASY_MALLOC(gs_pcwc_info[i], nbThreads + 64); // each thread has the state of each other
    gs_pcwc_info[i] = &(gs_pcwc_info[i][16]);

    P_epoch_ts[i] = nvmalloc_local_impl(i, sizeof(uint64_t) * allocEpochs, 0, 1);
    memset((void *)P_epoch_ts[i], 0, sizeof(uint64_t) * allocEpochs);

    uint64_t *startLogMarker = nvmalloc_local_impl(i, sizeof(uint64_t) * 3, 0, 1);
    startLogMarker[0] = (uint64_t)-1;
    startLogMarker[1] = (uint64_t)-1;
    startLogMarker[2] = (uint64_t)-1;
    P_write_log[i] = nvmalloc_local_impl(i, sizeof(uint64_t) * allocLogSize, 0, 1);

    phtm_logs[i] = (PHTM_log_s *)&(P_write_log[i][0]);
    phtm_logs[i]->size = 0;
    phtm_logs[i]->is_persistent = 0;

    // P_write_log does not need initialization
    G_observed_ts[i].ts = (uint64_t)-1;
    G_next[i].log_ptrs.flush_epoch = allocEpochs;
  }

  // --------------------------------
  // PHTM markers
  int i, nb_cache_lines = PHTM_NB_MARKERS;

  // The markers can be on volatile memory
  // phtm_markers = (PHTM_marker_pool_s*) ((char*)P_write_log[0] + sizeof (PHTM_log_s) + 128);
  // phtm_markers->markers = (PHTM_marker_s*) ((char*)P_write_log[0] + sizeof (PHTM_log_s) + sizeof (PHTM_marker_pool_s) + 128);
  phtm_markers = (PHTM_marker_pool_s *)malloc(sizeof(PHTM_marker_pool_s));
  phtm_markers->markers = (PHTM_marker_s *)malloc(sizeof(PHTM_marker_s) * PHTM_NB_MARKERS);
  phtm_markers->nb_markers = nb_cache_lines;

  for (i = 0; i < nb_cache_lines; ++i)
  {
    phtm_markers->markers[i].tid = -1;
  }
  // --------------------------------
  // printf("Saindo global_structs_init\n");
  // hashmap_init();
}

void nvmalloc_print_stats(char *filename)
{
  FILE *fp = fopen(filename, "a+");
  if (fp == NULL)
  {
    printf("Cannot open %s! Try to remove it.\n", filename);
    return;
  }
  fseek(fp, 0L, SEEK_END);
  if (ftell(fp) < 8)
  {
    fprintf(fp, "#%s\t%s\t%s\n",
            "NB_MALLOCS",
            "NB_BYTES_NUMA0",
            "NB_BYTES_NUMA1");
  }
  fprintf(fp, "%lu\t%lu\t%lu\n", nvmalloc_count,
          (uintptr_t)nvmalloc0_current_ptr - (uintptr_t)nvmalloc0_base_ptr,
          (uintptr_t)nvmalloc1_current_ptr - (uintptr_t)nvmalloc1_base_ptr);
}

void global_structs_destroy()
{
  // hashmap_finish();
  // printf("gstsarray %lx\n", (void *)gs_ts_array);
  free((void *)gs_ts_array);
  nvfree((void *)G_next); // TODO
  for (int i = 0; i < gs_appInfo->info.nbThreads; ++i)
  {
    // printf("epochts %lx\n", (void *)P_epoch_ts[i]);
    // printf("writelog %lx\n", (void *)P_write_log[i]);
    nvfree((void *)P_epoch_ts[i]);
    nvfree((void *)P_write_log[i]);
  }
  nvfree((void *)P_epoch_ts);
  nvfree((void *)P_write_log);
  if (gs_appInfo != (cache_line_s *)HTM_read_only_storage1)
  {
    free((void *)gs_appInfo);
  }
  __atomic_store_n(G_flag_checkpointer_exit, 1, __ATOMIC_RELEASE);

  #ifdef USE_REPLAYER
    while (!__atomic_load_n(G_flag_checkpointer_done, __ATOMIC_ACQUIRE))
    {
      pthread_yield(); // wait the checkpointer
      usleep(100);
    }
  #endif
  // TODO: erase the file
}

// Comment: returns base_ptr address on NUMA0, used as base addr
//             (HEAP_START_ADDR in stamp's tm.h -> nvmalloc_thr_priv_base_ptr = malloc(nbThreads * sizeof(void*)))
void *getNumaBaseAddress()
{
  // return (void*)nvmalloc0_current_ptr;
  // printf ("\ngetNumaBaseAddrPriv %p Shar:%p\n", (void*)nvmalloc_thr_priv_base_ptr[0], (void*)nvmalloc_thr_shar_base_ptr[0]);
  // printf ("\ngetNumaBaseAddrPriv %p baseptr:%p\n", (void*)nvmalloc_thr_priv_base_ptr[0], (void*)nvmalloc0_base_ptr);
  return (void *)nvmalloc_thr_priv_base_ptr[0]; //(void*)nvmalloc_thr_priv_base_ptr[0]) adds 6 f's in the front
}

// returns the "local" memory already used by the current thread
uint64_t get_local_memory_used()
{
  if (nvmalloc_thr_priv_current_ptr == NULL) return 0;
  return  (uintptr_t)nvmalloc_thr_priv_current_ptr - (uintptr_t)nvmalloc_thr_priv_base_ptr[myId];
}

// returns the shared memory already used
uint64_t get_shared_memory_used()
{
  return (uintptr_t)nvmalloc0_current_ptr - (uintptr_t)nvmalloc0_base_ptr;
}

