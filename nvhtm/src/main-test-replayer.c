#ifdef __linux__
#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 600
#define _BSD_SOURCE 1
#define _GNU_SOURCE 1
#define _DEFAULT_SOURCE 1
#endif /* __linux__ */

#define USE_ZIPF // defining it changes from normal paging distribution to zipfian (Pareto distribution)

#include "spins.h"
#include "impl.h"
#include "threading.h"
#include "rdtsc.h"
#include "input_handler.h"
#include "bench.h"
#ifdef USE_PAGING
  #include "paging.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* Begin includes for large heap management test */
#include <signal.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
/* End includes for large heap management test */

static __thread unsigned long RND_FN_seed = 5678; // TODO: put some timer //seed based on threadid
#define RAND_R_FNC(seed) ({                       \
  unsigned long next = seed;                      \
  unsigned long result;                           \
  next *= 1103515245;                             \
  next += 12345;                                  \
  result = (unsigned long)(next / 65536) % 2048;  \
  next *= 1103515245;                             \
  next += 12345;                                  \
  result <<= 10;                                  \
  result ^= (unsigned long)(next / 65536) % 1024; \
  next *= 1103515245;                             \
  next += 12345;                                  \
  result <<= 10;                                  \
  result ^= (unsigned long)(next / 65536) % 1024; \
  seed = next;                                    \
  result;                                         \
})

#ifdef __linux__
#include <unistd.h>
#else /* !__linux__ */
#error "TODO: implement a usleep function"
#endif /* __linux__ */

#include "htm_impl.h"

// DANGER: make sure log sizes are base 2
// #define USE_BITWISE_FOR_MOD2

// #define DO_PAGING 0

// how much to wait until we get impatient (in cycles)?

int test_spins_stats;
int NB_THREADS = 12;
int NB_REPLAYERS = 1;
unsigned int EXPERIMENT_TIME = 10e6; // in micro-seconds
int USE_MAIN_THREAD = 0;
int TID0_SLOWDOWN = 0;
long EPOCH_TIMEOUT = 50000;
int SAME_MEM_POOL = 0;
long FIX_NUMBER_OF_TXS = -1;
int FIX_NUMBER_OF_TXS_COMMITTED_THREADS = 0;

long SPINS_EXEC = 1;
long NB_LOG_ENTRIES = 2048L; // 67108864L;
long NB_READS = 1;
long NB_WRITES = 1;
long SPINS_FLUSH = 500; // learned_spins
int numReplayers = 1;
// int FLUSH_LAT    = 500;
// int FORCE_LEARN  = 0;
char PROFILE_FILE[1024] = "./profile.tsv";
char PROFILE_NVRAM_READS_FILE[1024] = "./profile_nvram_pstm.tsv";
char ERROR_FILE[1024] = "stderr"; // if not set prints to stderr
int PINNING = 1;                  // 0 == default, 1 == Fill CPU/SMT/NUMA, 2 == SMT/CPU/NUMA
int *PINNING_MAT = NULL;
int usePSTM = 0;

static __thread void *large_mem_region = NULL;
static void *shared_mem_region = NULL;

extern __thread long bench_read_time;
extern __thread long bench_tot_time;
extern __thread long bench_read_time_samples;
static long tot_bench_read_time = 0;
static long tot_bench_tot_time = 0;
static long tot_bench_read_time_samples = 0;
// extern long PagerOutActivations;// = 0;
double zipf_alpha = 2.0;

#ifdef USE_PAGING
  static struct paging_init_var Paging;
#endif

// #define USE_IMPROVED_PAGING
// #define SYNC_PAGEOUT

// #ifdef USE_IMPROVED_PAGING

// NOTE: DO_PAGING must be set and SYNC_PAGEOUT must be unset in order for
// this to work
// extern pthread_cond_t cv_not_enough_pages;// = PTHREAD_COND_INITIALIZER;
// extern pthread_mutex_t mt_memory_used;// = PTHREAD_MUTEX_INITIALIZER;
// extern float Paging.addPageThreshold;// = .9; //input is here
// extern float Paging.rmPageThreshold;// = .9; //input is here
// #endif

// #define DEBUG_PAGE
// #ifdef DEBUG_PAGE
//   extern uint64_t nb_page_in;// = 0;
//   extern uint64_t nb_page_out;// = 0;
// #endif

/* performance counters */
long nbExecStates = 0;
double timeBench = 0;
int startNow = 0;
__thread int isStarted = 0;

long nbSuccess = 0, nbFallback = 0, nbAbort = 0, nbConfl = 0, nbCapac = 0, nbExpli = 0, nbOther = 0;

unsigned long *count_txs;

static volatile int countNbThreadsReady = 0;

void (*state_profile)(int);
void (*state_print_profile)(char *);

// static void wait_fn()
// {
//   // struct timespec sleepTime = {.tv_sec = 0, .tv_nsec = 100000};
//   // nanosleep(&sleepTime, NULL); // sleeps for 100us
//   pthread_yield();
// }

static void inTxFn(void *arg)
{
  intptr_t id = (intptr_t)arg;
  if (id == 0)
  {
    spin_fn(TID0_SLOWDOWN);
  }
  spin_fn(SPINS_EXEC);
}

// #ifdef DO_PAGING
///* Begin test code for large heap management */
//
//// should this be in HD or Optane?
// #define MEMORY_HEAP_FILE "./somefile.heap"
//

// typedef struct paging_init_var
// {
//   static uint64_t HEAP_START_ADDR; // TODO: the kernel may not allow (most likely it will)
//   static uint64_t memory_heap_size; // total memory
//   static uint64_t memory_heap_mmap; //default
//   static float addPageThreshold; //input is here
//   static float rmPageThreshold; //input is here
// };

//// default values (can be changed through command line)
// #define MEMORY_HEAP_SIZE 262144  // total memory
// #define MEMORY_HEAP_MMAP 262144  // working memory
#define PERC_MEMORY_HEAP_MMAP 70 // working memory percentage

// #define Paging.HEAP_START_ADDR  0x0000001000000000L // TODO: the kernel may not allow (most likely it will)
// extern uint64_t Paging.memory_heap_size;// = MEMORY_HEAP_SIZE; // total memory
// uint64_t Paging.memory_heap_mmap = MEMORY_HEAP_MMAP; //working memory
uint64_t perc_memory_heap_mmap = PERC_MEMORY_HEAP_MMAP; // working memory percentage of total memory
// extern uint64_t Paging.memory_heap_mmap; // = 262144; //default
//
//// https://unix.stackexchange.com/questions/509607/how-a-64-bit-process-virtual-address-space-is-divided-in-linux
#define PAGE_SIZE 4096
#define PAGE_SIZE_BITS 12
// #define PAGE_MASK        0b111111111111L // ==0xFFFL (12 bits, be sure you have the L in the end)
//
//
// static volatile uint64_t pages[NB_PAGES_64BITS] = {0};
// static uint64_t *pages;
// uint64_t *pages;
// static volatile uint64_t curr_size_of_heap = 0;
// static volatile int heap_fd = -1;
//
// static pthread_mutex_t pager_mtx;
// static pthread_t pager_thread;
// static pid_t my_pid;
// static int pagemap_fd;
// static int countSegFault;
//
// static void print_page_data(uint64_t address)
//{
//  const uint64_t OS_PAGE_SIZE = 0x1000; // TODO: get this from some system header
//  uint64_t data;
//  uint64_t index = (address / OS_PAGE_SIZE) * sizeof(data);
//  if(pread(pagemap_fd, &data, sizeof(data), index) != sizeof(data)) {
//    perror("pread pagemap");
//    return;
//  }
//  printf("0x%lx: pfn %lx soft-dirty %ld exclusive %ld file/shared %ld "
//         "swapped %ld present %ld\n",
//    address,
//    data & 0x7fffffffffffff,
//    (data >> 55) & 1,
//    (data >> 56) & 1,
//    (data >> 61) & 1,
//    (data >> 62) & 1,
//    (data >> 63) & 1
//  );
//}
//
// static void add_page(void *addr)
//{
//    // printf("Adding page 0x%p\n", addr);
//    // printf("{ IN}: ");
//    // print_page_data(addr);
//    uintptr_t toMap = (uintptr_t)addr;
//    toMap &= ~PAGE_MASK;
//    uintptr_t a = toMap;
//    a -= Paging.HEAP_START_ADDR; // subtracts base addr
//    off_t location_in_file = (off_t)a;
/////    printf("add_page toMap = %lx, a = %lx (MEMORY_HEAP_SIZE = %lx)\n", toMap, a, MEMORY_HEAP_SIZE);
//    // assert(a < MALLOC_SIZE);
//    assert(a < Paging.memory_heap_size);
//    a >>= PAGE_SIZE_BITS;
//    int loc64    = a & 0x3F;
//    int locPage  = a >> 6; // log2(64) == 6
//
//    // TODO ----------
//    // TODO: test if mutex adds to much overhead
//    pthread_mutex_lock(&pager_mtx);
//    //printf("toMap = %lx, a = %lx (loc64 = %i, locPage = %i)\n", toMap, a, loc64, locPage);
//    // assert(!((1L<<loc64) == (pages[locPage] & (1L<<loc64))) && "bit was already set!");
//    if ((1L<<loc64) == (pages[locPage] & (1L<<loc64)))
//    {
//      pthread_mutex_unlock(&pager_mtx);
//      return; // already mapped
//    }
//    pages[locPage] |= (1L<<loc64);
/////    printf("after add_page = %lx (locPage = %i, addr = %p)\n", pages[locPage], locPage, addr);
//    pthread_mutex_unlock(&pager_mtx);
//
//    // TODO: mutex
//    // TODO ----------
//
//// #ifndef NDEBUG
////     off_t off_res =
//// #endif
////     // lseek(large_mem_region, location_in_file, SEEK_SET); // TODO: how does this work with PM?
////     lseek(heap_fd, location_in_file, SEEK_SET); // TODO: how does this work with PM?
////     assert(-1 != off_res && "could not set position in file");
//
//    // can we wait inside a signal handler?
//    // __atomic_thread_fence(__ATOMIC_ACQUIRE);
//    // while (curr_size_of_heap > Paging.memory_heap_mmap);
//
// #ifndef NDEBUG
//    void *res =
// #endif
//    // mmap((void*)toMap, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED, large_mem_region, 0);
//    // mmap((void*)toMap, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED, heap_fd, 0);
//    mmap((void*)toMap, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED, heap_fd, location_in_file);
//
//
// #ifdef USE_IMPROVED_PAGING
//    pthread_mutex_lock(&mt_memory_used);
//    curr_size_of_heap += PAGE_SIZE;
//
//    if (curr_size_of_heap >= ((uint64_t)(Paging.memory_heap_mmap*Paging.addPageThreshold)))
//      pthread_cond_signal(&cv_not_enough_pages); // wake up the pager thread to remove pages from DRAM
//
//    pthread_mutex_unlock(&mt_memory_used);
////    printf("%lu- %lu\n", curr_size_of_heap, (uint64_t)(Paging.memory_heap_mmap*0.9L));
////    while (curr_size_of_heap >= (uint64_t)(Paging.memory_heap_mmap*0.9L));
// #else
//     curr_size_of_heap += PAGE_SIZE;
//     __atomic_thread_fence(__ATOMIC_RELEASE);
// #endif
//
//     // perror("mmap()");
//     assert((uintptr_t)res == toMap && "could not map the requested page");
//     // int wasPageIn = (1<<loc64) == (pages[locPage] & (1<<loc64)); // TODO: the == can be avoided
//     // return wasPageIn;
// #ifdef DEBUG_PAGE
//     nb_page_in++;
// #endif
// }
//
// static void rm_page(void *addr)
//{
//     //printf("{OUT}: ");
//     //print_page_data(addr);
//     uintptr_t toUnmap = (uintptr_t)addr;
//     toUnmap &= ~PAGE_MASK;
//     uintptr_t a = toUnmap;
//     a -= Paging.HEAP_START_ADDR;
//     // if (!(a < MEMORY_HEAP_SIZE))
//     //     printf("before rm_page %p toUnmap = %lx a = %lx Paging.HEAP_START_ADDR = %p\n", addr, toUnmap, a, (void*)Paging.HEAP_START_ADDR);
//     // assert(a < MALLOC_SIZE);
//     assert(a < Paging.memory_heap_size);
//     a >>= PAGE_SIZE_BITS;
//     int loc64    = a & 0x3F;
//     int locPage  = a >> 6;
////     printf("before rm_page %p (bitmap = %lx, loc64 = %i)\n", addr, pages[locPage], loc64);
//
//    // TODO ----------
//    // TODO: test if mutex adds to much overhead
//
//    pthread_mutex_lock(&pager_mtx);
//     assert((1L<<loc64) == (pages[locPage] & (1L<<loc64)) && "bit was not set!");
//    if (!((1L<<loc64) == (pages[locPage] & (1L<<loc64))))
//    {
//      pthread_mutex_unlock(&pager_mtx);
//      return; // already unmapped
//    }
//    pages[locPage] &= ~(1L<<loc64);
/////     printf("after rm_page %p (bitmap = %lx, mask = %lx)\n", addr, pages[locPage], ~(1L<<loc64));
//    pthread_mutex_unlock(&pager_mtx);
//
//    // TODO: mutex
//    // TODO ----------
//
//    int err = munmap((void*)toUnmap, PAGE_SIZE);
//    curr_size_of_heap -= PAGE_SIZE;
//    __atomic_thread_fence(__ATOMIC_RELEASE);
//    assert(!err && "could not unmap the page");
////    printf("done\n");
// #ifdef DEBUG_PAGE
//     nb_page_out++;
// #endif
// }
//
// static int rightmostbit(uint64_t b) //page has to be 1
//{
//     for (int i = 0; i < 64; ++i)
//     {
//         if (b & (1L<<i)) //maybe here is the problem? always gets the 1st number
//         {
//             return i;
//         }
//     }
//     return -1;
// }
//
//
// static void *select_page_out() // just throws away the first page it finds (try random or some statistic)
//{
//     //unsigned long RND_FN_seed = getpid();
//     // printf("%lu\n", RND_FN_seed);
//     uintptr_t rnd = RAND_R_FNC(RND_FN_seed) % ((Paging.memory_heap_size/PAGE_SIZE)/64);
//     int count = 0;
//
//     while (0 == pages[rnd] && count < 64)
//     {
//       rnd = (rnd + 1) % ((Paging.memory_heap_size/PAGE_SIZE)/64);
//       count++;
//     }
//     if (0 != pages[rnd])
//     {
//         uintptr_t rmb = RAND_R_FNC(RND_FN_seed) % 64;
//         // printf ("pagesrnd: %lx, rmb init: %lu, bit: %lu, %lx\n", pages[rnd], rmb, (pages[rnd] & (1L<<rmb)), (1L<<rmb));
//         for (int i=0; i<64; i++){
//             if (pages[rnd] & (1L<<rmb))
//                 break;
//             // printf ("- %lu ", rmb);
//             rmb = (rmb+1)%64;
//         }
//         // printf ("rmb final: %lu\n", rmb);
//         // uintptr_t rmb = rightmostbit(pages[rnd]); // TODO: throw some access statistic here
//         uintptr_t offset = 64L*rnd + rmb; //RMB NOT TRULY RANDOM
//         // printf ("%lu\n", offset);
//         uintptr_t pagetorm = Paging.HEAP_START_ADDR + offset*PAGE_SIZE;
//         // printf("select_page_out rmb = %i page = %i, addr = %p\n", rmb, offset, (void*)pagetorm);
//         return (void*)pagetorm;
//     }
//
//     assert(0 && "there are not pages to remove!");
//     return NULL;
// }
//
// static void prepare_heap()
//{
//     heap_fd = open(MEMORY_HEAP_FILE, O_CREAT | O_TRUNC | O_RDWR, 0666);
//     close(heap_fd); // writes the permissions
//     if (-1 == (heap_fd = open(MEMORY_HEAP_FILE, O_CREAT | O_RDWR, 0666)))
//         fprintf(stderr, "Error open file \"%s\": %s\n", MEMORY_HEAP_FILE, strerror(errno));
//     if (ftruncate(heap_fd, Paging.memory_heap_size))
//         fprintf(stderr, "Error ftruncate file \"%s\": %s\n", MEMORY_HEAP_FILE, strerror(errno));
//     // note, nothing is allocated here
// }
//
//
// static void heap_sigsegv_handler(int sig, siginfo_t *si, void *unused)
//{
//     uintptr_t addr = (uintptr_t)si->si_addr;
//     if (addr - Paging.HEAP_START_ADDR >= Paging.memory_heap_size)
//     {
//         // access is outside OUR heap! Thus, it is actually a SIGSEGV!
//         printf("Got a true SIGSEGV at address: 0x%lx\n", addr);
//         countSegFault++;
//         exit(EXIT_FAILURE);
//     }
// #ifdef SYNC_PAGEOUT
//     if (curr_size_of_heap > Paging.memory_heap_mmap)
//       rm_page(select_page_out());
// #endif
//     // printf("curr_size_of_heap = %li MALLOC_SIZE = %li\n", curr_size_of_heap, MALLOC_SIZE);
//     add_page((void*)addr);
// }
///* End test code for large heap management */
//
////static void *pager_thread_fn(void *arg)
////{
////#ifdef USE_IMPROVED_PAGING
////
////  while (!gs_appInfo->info.isExit) {
////
////    pthread_mutex_lock(&mt_memory_used);
////
////    if (curr_size_of_heap < (uint64_t)(Paging.memory_heap_mmap*Paging.addPageThreshold)) {
//////      printf("Sleeping\n");
////      pthread_cond_wait(&cv_not_enough_pages, &mt_memory_used);
////    }
////
////    //pthread_mutex_unlock(&mt_memory_used); //UNLOCK_TOGGLE 0 //COULD HELP, WHILE WE PG OUT WE CAN PG IN
//////    printf("Woke up\n");
//    #ifdef DEBUG_PAGE
//      PagerOutActivations++;
//    #endif
//
//    while (curr_size_of_heap >= (uint64_t)(Paging.memory_heap_mmap*Paging.rmPageThreshold)){ //reduce the threshold to 85/80%, make 2 vars, one for each threshold setting, this and line 415
//      rm_page(select_page_out());
//    }
//
//    pthread_mutex_unlock(&mt_memory_used); //UNLOCK_TOGGLE 1
//  }
//
// #else
//
//  while (!gs_appInfo->info.isExit || (curr_size_of_heap > Paging.memory_heap_mmap))
//  {
//    // printf("Passed here with current size %d and nmap %d\n", curr_size_of_heap, MEMORY_HEAP_MMAP);
//    __atomic_thread_fence(__ATOMIC_ACQUIRE);
//    while (curr_size_of_heap > Paging.memory_heap_mmap)
//      rm_page(select_page_out());
//    usleep(/*TODO: tune*/1000); // 1ms sleep
//
//  }
// #endif
//}
// #endif

static void thread_main(int id, int nb_thrs, void *arg)
{
  int core = id;
  if (!usePSTM)
  {
    HTM_thr_init(id);
    HTM_set_is_record(1); // collects aborts statistics
  }
  else
  {
    bench_stm_init_thread();
  }

  // -----------------------
  core = PINNING_MAT[id];
  // -----------------------

  threading_pinThisThread(core); // coreID == threadID (affinity only for 1 core)

#ifdef USE_PAGING
  long alloc_size = Paging.memory_heap_size / nb_thrs;
  printf ("mhs %ld  nb_thrs %d\n", Paging.memory_heap_size, nb_thrs);
#else 
  long alloc_size = (8*1024*1024) / nb_thrs;
#endif

  if (SAME_MEM_POOL)
  {
    large_mem_region = shared_mem_region;
  }
  else
  {
    if (!usePSTM)
    {

      // TODO: remove this heap address reset and move it to alocateInNVRAM in global_structs.c
      // #ifdef DO_PAGING
      // TODO: do we need to segment the working set so that each thread will not
      // conflict with each other? In this case, how do we control that a page
      // in procedure brings a page from the correct segment? (need to change
      // return_page_address procedure)
      // TODO: move to main?
      // large_mem_region = Paging.HEAP_START_ADDR;
      // large_mem_region = Paging.HEAP_START_ADDR + (id*(Paging.memory_heap_mmap/nb_threads));
      // large_mem_region = (void*)Paging.HEAP_START_ADDR + (id*(Paging.memory_heap_size/nb_thrs));
      // #else
      // printf("nvmalloc_local %p\n", nvmalloc_local(id, alloc_size));
      large_mem_region = nvmalloc_local(id, alloc_size); // shared region by all threads
      // printf ("\nlmr %d %p\n", id, large_mem_region);

      // #endif
    }
    else
    {
      large_mem_region = bench_palloc(id, alloc_size); // shared region by all threads
    }
    // memset((void*)large_mem_region, 0, sharedMemRegionSize);
    //((int64_t*)large_mem_region)[(alloc_size) / sizeof(int64_t) - 1] = 0;
  }

  // for (int i = 0; i < Paging.memory_heap_size/PAGE_SIZE /64 ; i++) {
  //   printf("%lx\n", pages[i]);
  // }

  // Pre-touch all pages to force copy-on-write
  // TODO: add flag verification to only execute this code when requested
  // TODO: move to main?

  // if (id == 0) {  // only one thread needs to do this - TODO: syncronize the threads
  //   long i;
  //   //for (i = 0; i < (alloc_size) / sizeof(int64_t) ; i += 4096 / sizeof(int64_t)) {
  //   for (i = 0; i < (Paging.memory_heap_mmap) / sizeof(int64_t) ; i += 4096 / sizeof(int64_t)) {
  //       ((int64_t*)large_mem_region)[i] = 0;
  //   }

  //   printf("Initial bitmap vector:\n");
  //   for (i = 0; i < Paging.memory_heap_size/PAGE_SIZE /64 ; i++) {
  //     printf("%lx\n", pages[i]);
  //   }

  //   //printf("AFTERPRETOUCH: pagein: %d, pageout: %d\n", nb_page_in, nb_page_out);
  //   // nb_page_in=0;
  //   // nb_page_out=0;
  // }

  // printf ("\n1\n");
  __atomic_fetch_add(&countNbThreadsReady, 1, __ATOMIC_RELAXED);
  while (!(countNbThreadsReady == nb_thrs))
    pthread_yield();

  // printf("thread_area %d\n",  (int)Paging.memory_heap_size/nb_thrs);
  // uint64_t ts0,ts1, tot = 0;
  while (!gs_appInfo->info.isExit)
  {
    // ts0 = rdtscp();
    if (!usePSTM && FIX_NUMBER_OF_TXS == -1 && startNow && !isStarted)
    {
      isStarted = 1;
      HTM_reset_status_count();
      count_txs[id] = 0;
    }
    // printf ("2\n");
    volatile uint64_t* addr;
    volatile uint64_t value_to_write;
    volatile uint64_t value_to_read;
    #ifdef USE_HASHMAP
    #include "../include/hashmap.h"
    #define HT_MASK ~0x0FFFUL
    extern hashtable_t *pagein_ht; //bucket count
    #endif
    volatile uint64_t ts0, ts1, ts2, ts3;

    // bench_no_conflicts(large_mem_region, id, SPINS_EXEC, inTxFn, (void*)(intptr_t)id);
    if (!usePSTM)
    {
      int threadId=id;
      printf ("tx1\n");

      NV_HTM_BEGIN(threadId);
      ts2 = rdtscp();
      value_to_write = 0xdeadbeef;
      addr = &((uint64_t*)large_mem_region)[255]; //////
      // onBeforeWrite(threadId, addr, value_to_write);
      on_before_non_tx_write_8B_pcwm(threadId, addr, value_to_write);
      printf ("tx1m\n");
      *addr = value_to_write;
      printf ("tx1end\n");
      NV_HTM_END(threadId);
      // printf("t_spins addr: %p || vtw: %lx || ts: %lx\n", addr, value_to_write, ts2);

      printf ("tx2\n");
      ts2 = rdtscp();
      NV_HTM_BEGIN(threadId);
      addr = &((uint64_t*)large_mem_region)[255+512];
      value_to_write = 0xbeefdead;
      // onBeforeWrite(threadId, addr, value_to_write);
      on_before_non_tx_write_8B_pcwm(threadId, addr, value_to_write);
      *addr = value_to_write;
      NV_HTM_END(threadId);
      // printf("t_spins addr: %p || vtw: %lx || ts: %lx\n", addr, value_to_write, ts2);

      printf ("tx3\n");
      NV_HTM_BEGIN(threadId);
      addr = &((uint64_t*)large_mem_region)[255];
      // onBeforeWrite(threadId, addr, value_to_write);
      value_to_read = *addr;
      // inTxFn(arg);
      NV_HTM_END(threadId);
      printf("t_spins addr: %p || vtr: %lx\n", addr, value_to_read);
      
      
      printf ("tx4\n");
      NV_HTM_BEGIN(threadId);
      addr = &((uint64_t*)large_mem_region)[255+512];
      // onBeforeWrite(threadId, addr, value_to_write);
      value_to_read = *addr;
      // inTxFn(arg);
      NV_HTM_END(threadId);
      printf("t_spins addr: %p || vtr: %lx\n", addr, value_to_read);
      
    }
    else
    {
      if (SAME_MEM_POOL)
      {
        #ifdef USE_PAGING
          bench_no_conflicts_with_reads_stm(large_mem_region, Paging.memory_heap_size,
                                            id, NB_READS, NB_WRITES, inTxFn, (void *)(intptr_t)id);
        #else
          bench_no_conflicts_with_reads_stm(large_mem_region, (8*1024*1024),
                                          id, NB_READS, NB_WRITES, inTxFn, (void *)(intptr_t)id);
        #endif
      }
      else
      {
        bench_no_conflicts_with_reads_stm(large_mem_region, alloc_size,
                                          id, NB_READS, NB_WRITES, inTxFn, (void *)(intptr_t)id);
      }
    }
    // printf ("\n3\n");

    // // if we spin here the impatient does not proceed
    // state_commit(id); // post commit
    count_txs[id]++;
    // __atomic_thread_fence(__ATOMIC_ACQUIRE); // TODO: remove this fence (acquires isExit)

    if (FIX_NUMBER_OF_TXS != -1 && count_txs[id] > FIX_NUMBER_OF_TXS)
    {
      __atomic_fetch_add(&FIX_NUMBER_OF_TXS_COMMITTED_THREADS, 1, __ATOMIC_RELAXED);
      break;
    }
    // printf ("\n4\n");

    // ts1 = rdtscp();
    // tot += ts1 - ts0;
  }

  // printf("Cycles per TX = %f\n", (float)tot / (float)count_txs[id]);
  // printf("Cycles in  TX = %f\n", (float)bench_tot_time / (float)bench_read_time_samples);

  __atomic_fetch_add(&tot_bench_read_time, bench_read_time, __ATOMIC_RELAXED);
  __atomic_fetch_add(&tot_bench_tot_time, bench_tot_time, __ATOMIC_RELAXED);
  __atomic_fetch_add(&tot_bench_read_time_samples, bench_read_time_samples, __ATOMIC_RELAXED);

  // __atomic_fetch_add(&countNbThreadsReady, 1, __ATOMIC_RELAXED);
  // while (!(countNbThreadsReady == 2 * nb_thrs)) pthread_yield();

  if (!usePSTM)
  {
    __atomic_fetch_add(&nbSuccess, HTM_get_status_count(HTM_SUCCESS, NULL), __ATOMIC_RELAXED);
    __atomic_fetch_add(&nbConfl, HTM_get_status_count(HTM_CONFLICT, NULL), __ATOMIC_RELAXED);
    __atomic_fetch_add(&nbCapac, HTM_get_status_count(HTM_CAPACITY, NULL), __ATOMIC_RELAXED);
    __atomic_fetch_add(&nbExpli, HTM_get_status_count(HTM_EXPLICIT, NULL), __ATOMIC_RELAXED);
    __atomic_fetch_add(&nbOther, HTM_get_status_count(HTM_OTHER, NULL), __ATOMIC_RELAXED);
    __atomic_fetch_add(&nbFallback, HTM_get_status_count(HTM_FALLBACK, NULL), __ATOMIC_RELAXED);
    __atomic_fetch_add(&nbAbort, HTM_get_status_count(HTM_ABORT, NULL), __ATOMIC_RELAXED);

    HTM_thr_exit();
  }
  else
  {
    bench_stm_exit_thread();
  }
#ifndef NPROFILE
  if (state_profile != NULL)
    state_profile(id);
#endif
  // printf ("\n5\n");
}

static void input_handler()
{
  int usePhysicalClocks = 1;
  install_bindings_pc(); // may be overrided
  state_profile = state_gather_profiling_info_pc;
  state_print_profile = state_fprintf_profiling_info_pc;
  log_replay_flags = LOG_REPLAY_FORWARD;
  log_replay_flags |= LOG_REPLAY_CONCURRENT;

  // if (input_exists("SIZE_WORKING_SET")) {
  //   Paging.memory_heap_mmap = input_getLong("SIZE_WORKING_SET");
  // }
  // printf("SIZE_WORKING_SET = %li\n", Paging.memory_heap_mmap);

  #ifdef USE_PAGING
    if (input_exists("TOTAL_MEMORY"))
    {
      Paging.memory_heap_size = input_getLong("TOTAL_MEMORY");
    }
    printf("TOTAL_MEMORY = %li\n", Paging.memory_heap_size);

    if (input_exists("PERC_SIZE_WORKING_SET"))
    {
      perc_memory_heap_mmap = input_getLong("PERC_SIZE_WORKING_SET");
      Paging.memory_heap_mmap = perc_memory_heap_mmap * Paging.memory_heap_size * 0.01;
    }
    printf("PERC_SIZE_WORKING_SET = %li\n", perc_memory_heap_mmap);
    printf("SIZE_WORKING_SET = %li\n", Paging.memory_heap_mmap);

    if (input_exists("ZIPF_ALPHA"))
    {
      zipf_alpha = input_getLong("ZIPF_ALPHA");
      zipf_alpha = zipf_alpha / 100;
    }
    printf("ZIPF_ALPHA = %f\n", zipf_alpha);

    if (input_exists("RM_PAGE_THRESHOLD"))
    {
      Paging.rmPageThreshold = input_getLong("RM_PAGE_THRESHOLD");
      Paging.rmPageThreshold = Paging.rmPageThreshold / 100;
    }
    printf("RM_PAGE_THRESHOLD = %f\n", Paging.rmPageThreshold);

    if (input_exists("ADD_PAGE_THRESHOLD"))
    {
      Paging.addPageThreshold = input_getLong("ADD_PAGE_THRESHOLD");
      Paging.addPageThreshold = Paging.addPageThreshold / 100;
    }
    printf("ADD_PAGE_THRESHOLD = %f\n", Paging.addPageThreshold);
  #endif

  if (input_exists("SPINS_STATS"))
  {
    test_spins_stats = input_getLong("SPINS_STATS");
  }
  printf("SPINS_STATS = %d\n", test_spins_stats);

  if (input_exists("LOG_REPLAY_BACKWARD") || input_exists("usePCWM3"))
  {
    log_replay_flags = LOG_REPLAY_BACKWARD;
    printf("LOG_REPLAY_BACKWARD is set\n");
  }

  if (input_exists("LOG_REPLAY_BUFFER_WBINVD"))
  {
    log_replay_flags |= LOG_REPLAY_BUFFER_WBINVD;
    printf("LOG_REPLAY_BUFFER_WBINVD is set\n");
  }
  else if (input_exists("LOG_REPLAY_BUFFER_FLUSHES"))
  {
    log_replay_flags |= LOG_REPLAY_BUFFER_FLUSHES;
    printf("LOG_REPLAY_BUFFER_FLUSHES is set\n");
  }
  else if (input_exists("LOG_REPLAY_RANGE_FLUSHES"))
  {
    log_replay_flags |= LOG_REPLAY_RANGE_FLUSHES;
    printf("LOG_REPLAY_RANGE_FLUSHES is set\n");
  }

  if (input_exists("LOG_REPLAY_ASYNC_SORTER"))
  {
    log_replay_flags |= LOG_REPLAY_ASYNC_SORTER;
    printf("LOG_REPLAY_ASYNC_SORTER is set\n");
  }

  if (input_exists("LOG_REPLAY_STATS_FILE"))
  {
    input_getString("LOG_REPLAY_STATS_FILE", log_replay_stats_file);
  }
  printf("LOG_REPLAY_STATS_FILE = %s\n", log_replay_stats_file);

  if (input_exists("PINNING"))
  {
    PINNING = input_getLong("PINNING");
  }
  printf("PINNING is set to %i /* 0 = CPU/NUMA/SMT | 1 = CPU/SMT/NUMA | 2 = SMT/CPU/NUMA */\n", PINNING);
  // 0=SMT/CORES/NUMA, 1=CORES/SMT/NUMA, 2=NUMA/CORES/SMT
  if (PINNING == 0)
  {
    PINNING_MAT = (int *)G_PINNING_0;
  }
  else if (PINNING == 1)
  {
    PINNING_MAT = (int *)G_PINNING_1;
  }
  else if (PINNING == 2)
  {
    PINNING_MAT = (int *)G_PINNING_2;
  }

  if (input_exists("EXPERIMENT_TIME"))
  {
    EXPERIMENT_TIME = input_getLong("EXPERIMENT_TIME");
  }
  printf("EXPERIMENT_TIME = %u us\n", EXPERIMENT_TIME);

  if (input_exists("SPINS_EXEC"))
  {
    SPINS_EXEC = input_getLong("SPINS_EXEC");
  }
  printf("SPINS_EXEC = %li\n", SPINS_EXEC);

  if (input_exists("NB_READS"))
  {
    NB_READS = input_getLong("NB_READS");
  }
  printf("NB_READS = %li\n", NB_READS);

  if (input_exists("NB_WRITES"))
  {
    NB_WRITES = input_getLong("NB_WRITES");
  }
  printf("NB_WRITES = %li\n", NB_WRITES);

  if (input_exists("NB_LOG_ENTRIES"))
  {
    NB_LOG_ENTRIES = input_getLong("NB_LOG_ENTRIES");
  }
  printf("NB_LOG_ENTRIES = %li\n", NB_LOG_ENTRIES);

  if (input_exists("PROFILE_NVRAM_READS_FILE"))
  {
    if (input_getString("PROFILE_NVRAM_READS_FILE", PROFILE_NVRAM_READS_FILE) >= 1024)
    {
      fprintf(stderr, "string copy exceeded the capacity of the buffer\n");
    }
  }
  printf("PROFILE_NVRAM_READS_FILE = %s\n", PROFILE_NVRAM_READS_FILE);

  if (input_exists("NB_THREADS"))
  {
    NB_THREADS = input_getLong("NB_THREADS");
  }
  printf("NB_THREADS = %i\n", NB_THREADS);

  if (input_exists("NB_REPLAYERS"))
  {
    NB_REPLAYERS = input_getLong("NB_REPLAYERS");
  }
  printf("NB_REPLAYERS = %i\n", NB_REPLAYERS);

  if (input_exists("MALLOC_STATS_FILE"))
  {
    input_getString("MALLOC_STATS_FILE", malloc_stats_file);
  }
  printf("MALLOC_STATS_FILE = %s\n", malloc_stats_file);

  // if (input_exists("FLUSH_LAT")) {
  //   FLUSH_LAT = input_getLong("FLUSH_LAT");
  //   FORCE_LEARN = 1;
  // }
  // printf("FLUSH_LAT = %i\n", FLUSH_LAT);

  // if (input_exists("FORCE_LEARN")) {
  //   FORCE_LEARN = input_getLong("FORCE_LEARN");
  // }
  // printf("FORCE_LEARN = %i\n", FORCE_LEARN);

  if (input_exists("PROFILE_FILE"))
  {
    if (input_getString("PROFILE_FILE", PROFILE_FILE) >= 1024)
    {
      fprintf(stderr, "string copy exceeded the capacity of the buffer\n");
    }
  }
  printf("PROFILE_FILE = \"%s\"\n", PROFILE_FILE);

  extern FILE *error_fp;
  if (input_exists("ERROR_FILE"))
  {
    if (input_getString("ERROR_FILE", ERROR_FILE) >= 1024)
    {
      fprintf(stderr, "string copy exceeded the capacity of the buffer\n");
    }
    error_fp = fopen(ERROR_FILE, "a+");
  }
  else
  {
    error_fp = fopen("error-log-checker.log", "a+");
  }
  printf("ERROR_FILE = \"%s\"\n", ERROR_FILE);

  if (input_exists("spinInCycles"))
  {
    printf("spinInCycles is set /* use TSC instead of NOPs */\n");
    spin_fn = spin_cycles;
  }
  else
  {
    printf("spinInCycles not set /* use NOPs */\n");
  }

  if (input_exists("tid0Slowdown"))
  {
    TID0_SLOWDOWN = input_getLong("tid0Slowdown");
  }
  printf("tid0Slowdown is set to %i\n", TID0_SLOWDOWN);

  if (input_exists("EPOCH_TIMEOUT"))
  {
    EPOCH_TIMEOUT = input_getLong("EPOCH_TIMEOUT");
  }
  printf("EPOCH_TIMEOUT is set to %li\n", EPOCH_TIMEOUT);

  if (input_exists("SAME_MEM_POOL"))
  {
    SAME_MEM_POOL = 1;
    printf("SAME_MEM_POOL is set\n");
  }

  if (input_exists("LOG_REPLAY_PARALLEL"))
  {
    log_replay_flags |= LOG_REPLAY_PARALLEL;
    printf("LOG_REPLAY_PARALLEL is set\n");
  }

  extern int PCWC_haltSnoopAfterAborts;
  extern int PCWC2_haltSnoopAfterAborts;
  if (input_exists("ABORTS_BEFORE_STOP_SNOOP"))
  {
    PCWC_haltSnoopAfterAborts = input_getLong("ABORTS_BEFORE_STOP_SNOOP");
    PCWC2_haltSnoopAfterAborts = input_getLong("ABORTS_BEFORE_STOP_SNOOP");
  }
  printf("ABORTS_BEFORE_STOP_SNOOP is set to %i\n", PCWC_haltSnoopAfterAborts);

  if (input_exists("FIX_NUMBER_OF_TXS"))
  {
    FIX_NUMBER_OF_TXS = input_getLong("FIX_NUMBER_OF_TXS") / NB_THREADS;
    printf("FIX_NUMBER_OF_TXS is set to %li\n", FIX_NUMBER_OF_TXS);
  }

  if (input_exists("useLogicalClocks"))
  {
    printf("useLogicalClocks is set\n");
    usePhysicalClocks = 0;
    install_bindings_lc();
    wait_commit_fn = wait_commit_lc;
    state_profile = state_gather_profiling_info_lc;
    state_print_profile = state_fprintf_profiling_info_lc;
    log_replay_flags |= LOG_REPLAY_LOGICAL_CLOCKS;
  }
  else if (input_exists((char *)"usePCWMeADRT1"))
  {
    printf("usePCWMeADRT1 is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm_eadr_t1();
    wait_commit_fn = wait_commit_pcwm_eadr_t1;
    state_profile = state_gather_profiling_info_pcwm_eadr_t1;
    state_print_profile = state_fprintf_profiling_info_pcwm_eadr_t1;
  }
  else if (input_exists((char *)"usePCWMeADRT2"))
  {
    printf("usePCWMeADRT2 is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm_eadr_t2();
    wait_commit_fn = wait_commit_pcwm_eadr_t2;
    state_profile = state_gather_profiling_info_pcwm_eadr_t2;
    state_print_profile = state_fprintf_profiling_info_pcwm_eadr_t2;
  }
  else if (input_exists((char *)"usePCWMeADRT3"))
  {
    printf("usePCWMeADRT3 is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm_eadr_t3();
    wait_commit_fn = wait_commit_pcwm_eadr_t3;
    state_profile = state_gather_profiling_info_pcwm_eadr_t3;
    state_print_profile = state_fprintf_profiling_info_pcwm_eadr_t3;
  }
  else if (input_exists((char *)"usePCWMeADRT4"))
  {
    printf("usePCWMeADRT4 is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm_eadr_t4();
    wait_commit_fn = wait_commit_pcwm_eadr_t4;
    state_profile = state_gather_profiling_info_pcwm_eadr_t4;
    state_print_profile = state_fprintf_profiling_info_pcwm_eadr_t4;
  }
  else if (input_exists((char *)"usePCWMeADRT5"))
  {
    printf("usePCWMeADRT5 is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm_eadr_t5();
    wait_commit_fn = wait_commit_pcwm_eadr_t5;
    state_profile = state_gather_profiling_info_pcwm_eadr_t5;
    state_print_profile = state_fprintf_profiling_info_pcwm_eadr_t5;
  }
  else if (input_exists("usePCWMeADR"))
  {
    printf("usePCWMeADR is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm_eadr();
    wait_commit_fn = wait_commit_pcwm_eadr;
    state_profile = state_gather_profiling_info_pcwm_eadr;
    state_print_profile = state_fprintf_profiling_info_pcwm_eadr;
    log_replay_flags |= LOG_REPLAY_PHYSICAL_CLOCKS;
  }
  else if (input_exists("usePCWM"))
  {
    printf("usePCWM is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm();
    wait_commit_fn = wait_commit_pcwm;
    state_profile = state_gather_profiling_info_pcwm;
    state_print_profile = state_fprintf_profiling_info_pcwm;
    log_replay_flags |= LOG_REPLAY_PHYSICAL_CLOCKS;
  }
  else if (input_exists("usePCWM2"))
  {
    printf("usePCWM2 is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm2();
    wait_commit_fn = wait_commit_pcwm2;
    state_profile = state_gather_profiling_info_pcwm2;
    state_print_profile = state_fprintf_profiling_info_pcwm2;
    log_replay_flags |= LOG_REPLAY_PHYSICAL_CLOCKS_SORTED;
  }
  else if (input_exists("usePCWM3"))
  {
    printf("usePCWM3 is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm3();
    wait_commit_fn = wait_commit_pcwm3;
    state_profile = state_gather_profiling_info_pcwm3;
    state_print_profile = state_fprintf_profiling_info_pcwm3;
    log_replay_flags |= LOG_REPLAY_PHYSICAL_CLOCKS_SORTED_BACKWARD;
  }
  else if (input_exists("useHTMUndo"))
  {
    printf("useHTMUndo is set\n");
    usePhysicalClocks = 0;
    isSharedHTM = 0;
    install_bindings_htmUndo();
    wait_commit_fn = wait_commit_htmUndo;
    state_profile = state_gather_profiling_info_htmUndo;
    state_print_profile = state_fprintf_profiling_info_htmUndo;
    log_replay_flags = 0;
  }
  else if (input_exists("useSharedHTMUndo"))
  {
    printf("useSharedHTMUndo is set\n");
    usePhysicalClocks = 0;
    isSharedHTM = 1;
    install_bindings_htmUndo();
    wait_commit_fn = wait_commit_htmUndo;
    state_profile = state_gather_profiling_info_htmUndo;
    state_print_profile = state_fprintf_profiling_info_htmUndo;
    log_replay_flags = 0;
  }
  else if (input_exists("useHTM"))
  {
    printf("useHTM is set\n");
    usePhysicalClocks = 0;
    isSharedHTM = 0;
    install_bindings_htmOnly();
    wait_commit_fn = wait_commit_htmOnly;
    state_profile = state_gather_profiling_info_htmOnly;
    state_print_profile = state_fprintf_profiling_info_htmOnly;
    log_replay_flags = 0;
  }
  else if (input_exists("useSharedHTM"))
  {
    printf("useSharedHTM is set\n");
    usePhysicalClocks = 0;
    isSharedHTM = 1;
    install_bindings_htmOnly();
    wait_commit_fn = wait_commit_htmOnly;
    state_profile = state_gather_profiling_info_htmOnly;
    state_print_profile = state_fprintf_profiling_info_htmOnly;
    log_replay_flags = 0;
  }
  else if (input_exists("useCrafty"))
  {
    printf("useCrafty is set\n");
    isCraftySet = 1;
    usePhysicalClocks = 0;
    install_bindings_crafty();
    wait_commit_fn = NULL;
    state_profile = state_gather_profiling_info_crafty;
    state_print_profile = state_fprintf_profiling_info_crafty;
    log_replay_flags = 0;
  }
  else if (input_exists("useCcHTM") || input_exists("useCcHTMbest"))
  {
    printf("useCcHTM is set");
    usePhysicalClocks = 0;
    log_replay_flags = LOG_REPLAY_CC_HTM;
    install_bindings_ccHTM();
    wait_commit_fn = wait_commit_ccHTM;
    state_profile = state_gather_profiling_info_ccHTM;
    state_print_profile = state_fprintf_profiling_info_ccHTM;
    if (input_exists("useCcHTMbest"))
    {
      printf(" --- no log re-write \n");
      log_replay_flags |= LOG_REPLAY_CC_HTM_NO_W;
    }
    else
    {
      printf("\n");
    }
  }
  else if (input_exists("usePHTM"))
  {
    printf("usePHTM is set\n");
    usePhysicalClocks = 0;
    log_replay_flags = 0;
    install_bindings_PHTM();
    wait_commit_fn = wait_commit_PHTM;
    state_profile = state_gather_profiling_info_PHTM;
    state_print_profile = state_fprintf_profiling_info_PHTM;
  }
  else if (input_exists("useUpperBound"))
  {
    printf("useUpperBound is set\n");
    usePhysicalClocks = 0;
    log_replay_flags = 0; // TODO
    install_bindings_ub();
    wait_commit_fn = wait_commit_ub;
    state_profile = state_gather_profiling_info_ub;
    state_print_profile = state_fprintf_profiling_info_ub;
  }
  else if (input_exists("useEpochCommit1"))
  {
    printf("useEpochCommit1 is set /* patient version */\n");
    usePhysicalClocks = 0;
    log_replay_flags = 0; // TODO
    install_bindings_epoch_sync();
    wait_commit_fn = wait_commit_epoch_sync;
    state_profile = state_gather_profiling_info_epoch_sync;
    state_print_profile = state_fprintf_profiling_info_epoch_sync;
  }
  else if (input_exists("useEpochCommit2"))
  {
    printf("useEpochCommit2 is set /* impatient version */\n");
    usePhysicalClocks = 0;
    log_replay_flags = 0; // TODO
    install_bindings_epoch_impa();
    wait_commit_fn = wait_commit_epoch_impa;
    state_profile = state_gather_profiling_info_epoch_impa;
    state_print_profile = state_fprintf_profiling_info_epoch_impa;
  }
  else if (input_exists("usePCWC-F") || input_exists("useFastPCWC") || input_exists("usePCWCnoCheck") || input_exists("useFastPCWCnoCheck") || input_exists("usePCWCnoCheckVect"))
  {
    printf("usePCWC-F is set");
    log_replay_flags |= LOG_REPLAY_LOGICAL_CLOCKS;
    usePhysicalClocks = 0;
    install_bindings_pcwc();
    wait_commit_fn = wait_commit_pcwc;
    state_profile = state_gather_profiling_info_pcwc;
    state_print_profile = state_fprintf_profiling_info_pcwc;
    if (input_exists("useFastPCWC") || input_exists("useFastPCWCnoCheck"))
    {
      printf(" -- fast validation for upper bound\n");
      extern volatile int useFastPCWC; // hacked
      useFastPCWC = 1;
    }
    else if (input_exists("usePCWCnoCheckVect"))
    {
      printf(" -- using vectorized instructions for scanning\n");
      extern volatile int useFastPCWC; // hacked
      useFastPCWC = 2;                 // reusing the variable -> 2 means vect path
    }
    else
    {
      printf("\n");
    }
  }
  else if (input_exists("usePCWC-NF"))
  {
    printf("usePCWC-NF is set");
    log_replay_flags |= LOG_REPLAY_LOGICAL_CLOCKS;
    usePhysicalClocks = 0;
    install_bindings_pcwc2();
    wait_commit_fn = wait_commit_pcwc2;
    state_profile = state_gather_profiling_info_pcwc2;
    state_print_profile = state_fprintf_profiling_info_pcwc2;
    printf("\n");
  }
  else if (input_exists("useEpochCommit3"))
  {
    printf("useEpochCommit3 is set /* deadline version */\n");
    usePhysicalClocks = 0;
    install_bindings_epoch_static_deadline();
    wait_commit_fn = wait_commit_epoch_static_deadline;
  }
  else if (input_exists("usePSTM"))
  {
    printf("usePSTM is set\n");
    usePhysicalClocks = 0;
    log_replay_flags = 0;
    usePSTM = 1;
    state_profile = NULL;
  }

  if (usePhysicalClocks)
  {
    log_replay_flags |= LOG_REPLAY_PHYSICAL_CLOCKS;
    printf("usePhysicalClocks is set\n"); // an optimized version, but does not seem that good anymore
    if (input_exists("waitCommitBitArray"))
    {
      printf("  waitCommitBitArray is set\n");
      wait_commit_fn = wait_commit_pc_bitmap;
    }
    else
    {
      printf("  waitCommitBitArray not set\n");
    }
  }

  if (input_exists("LOG_REPLAY_CONCURRENT") && !(input_exists("useCcHTM")))
  {
    printf("LOG_REPLAY_CONCURRENT is set\n");
    log_replay_flags |= LOG_REPLAY_CONCURRENT;
  }

  if (input_exists("DISABLE_LOG_REPLAY") && !(input_exists("useCcHTM") || input_exists("useCcHTMbest")))
  {
    printf("DISABLE_LOG_REPLAY is set\n");
    log_replay_flags = 0;
  }

  printf(" --- \n");
}

// static void test_latency()
// {
//   unsigned long ts1, ts2;
//
//   ts1 = rdtscp();
//   spin_fn(learnedSpins);
//   ts2 = rdtscp();
//   printf("test 1 spin --> %f ns\n", (ts2 - ts1) / CPU_FREQ * 1e6);
//
//   ts1 = rdtscp();
//   for (int i = 0; i < 9999; ++i) {
//     spin_fn(learnedSpins);
//   }
//   ts2 = rdtscp();
//   printf("test 9999 spins --> %f ns (per spin)\n",
//     (ts2 - ts1) / CPU_FREQ * 1e6 / 9999);
// }

// int return_page_address (int index, int p) //PFPIMP
// {
//   //return index;
//   while (1)
//   { //works if total memory is larger than working memory
//     index = index % (Paging.memory_heap_size / sizeof(uint64_t));
//     int pageNumber = index >> (PAGE_SIZE_BITS-3); //index is 8 byte alighned
//     int bitVectorIndex = pageNumber >> 6;
//     int bitVectorBit = pageNumber & 0x3F;
//     uint64_t is_not_page_fault = pages[bitVectorIndex] & (uint64_t)(1L << bitVectorBit);
//     //int is_not_page_fault = 1;
//     if (p == 0) { // do not generate page fault
//       if (is_not_page_fault != 0){ // page is present, return it
//         return index;
//       }

//     } else { // we need a page fault
//       if (is_not_page_fault == 0){ // got it
//         return index;
//       }
//     }
//     //index++;
//     index += 512;

//     /*uintptr_t toMap = (uintptr_t)index*8;
//     toMap &= ~PAGE_MASK;
//     uintptr_t a = toMap;
//     assert(a < Paging.Paging.memory_heap_size);
//     a >>= PAGE_SIZE_BITS;
//     int loc64    = a & 0x3F;
//     int locPage  = a >> 6;
//     int is_not_page_fault = pages[locPage] & (1 << loc64);
//     if (is_not_page_fault == p){
//       return (index);
//     }
//     index++;*/
//   }
// }

int main(int argc, char **argv)
{
  #ifdef USE_PAGING
    // static struct paging_init_var Paging;
    // Paging.HEAP_START_ADDR = (void*)0x7f6000000000;
    Paging.memory_heap_size = 262144; // total memory
    Paging.memory_heap_mmap = 262144; // default
    Paging.addPageThreshold = 0.9;
    Paging.rmPageThreshold = 0.85;
  #endif
  input_parse(argc, argv);
  input_handler();
  
  /* Begin large heap management test */
  #ifdef USE_PAGING
    #ifdef USE_ZIPF
      zipf(zipf_alpha, (Paging.memory_heap_size / NB_THREADS / sizeof(uint64_t)));
    #else
      rand_val(100);
    #endif
  #endif

  unsigned long ts1, ts2;
  #ifdef USE_PAGING
    uint64_t memRegionSize = Paging.memory_heap_size / NB_THREADS /* local MEM_REGION (1G) */;
  #else
    uint64_t memRegionSize = (8*1024*1024) / NB_THREADS /* local MEM_REGION (1G) */;
  #endif
  uint64_t sharedMemRegionSize = 1024 * 1024 * 1024 /* shared MEM_REGION (1G) */;

  if (SAME_MEM_POOL)
  {
    #ifdef USE_PAGING
      sharedMemRegionSize = Paging.memory_heap_size;
    #else
      sharedMemRegionSize = (8*1024*1024);
    #endif
  }

  // TODO: removed emulation
  // learn_spin_nops(CPU_FREQ, FLUSH_LAT, FORCE_LEARN);
  // printf("learned 500ns --> %i spins\n", learnedSpins);
  // SPINS_FLUSH = learnedSpins;
  // test_latency();

  global_structs_init(
      NB_THREADS,
      NB_REPLAYERS,
      1 /* NB_EPOCHS: use max of 1073741824 */,
      // 268435456L
      NB_LOG_ENTRIES /* LOG_SIZE: in nb of entries (134217728) */,
      memRegionSize,
      sharedMemRegionSize,
      SPINS_FLUSH,
      (int *)PINNING_MAT,
      (int *)G_NUMA_PINNING,
      (char **)NVRAM_REGIONS);

  #ifdef USE_PAGING
    Paging.HEAP_START_ADDR = getNumaBaseAddress();
    // printf ("\nbefore paging %p gNBA:%p\n", Paging.HEAP_START_ADDR, getNumaBaseAddress());
    paging_init(Paging); // Paging.HEAP_START_ADDR, Paging.memory_heap_size, Paging.memory_heap_mmap);
  #endif
  // #ifdef DO_PAGING

  //   pthread_mutex_init(&pager_mtx, NULL); // TODO: check error
  // #ifndef SYNC_PAGEOUT
  //   pthread_create(&pager_thread, NULL, pager_thread_fn, NULL); // TODO: check error
  // #endif
  // #endif

  if (usePSTM)
  {
    #ifdef USE_PAGING
      shared_mem_region = bench_stm_init(SAME_MEM_POOL, Paging.memory_heap_size);
    #else
      shared_mem_region = bench_stm_init(SAME_MEM_POOL, (8*1024*1024));
    #endif
  }
  else
  {
    if (SAME_MEM_POOL)
    {
       #ifdef USE_PAGING
        shared_mem_region = nvmalloc(Paging.memory_heap_size);
      #else
        shared_mem_region = nvmalloc((8*1024*1024));
      #endif
    }
  }

  EASY_MALLOC(count_txs, NB_THREADS);
  memset((void *)count_txs, 0, NB_THREADS * sizeof(unsigned long));
  HTM_init(NB_THREADS);

  threading_start(NB_THREADS, USE_MAIN_THREAD, thread_main, NULL);

  while (!(countNbThreadsReady == NB_THREADS))
    ;

  if (FIX_NUMBER_OF_TXS != -1)
  {
    startNow = 1;
    __atomic_thread_fence(__ATOMIC_RELAXED);
    ts1 = rdtscp();
    while (__atomic_load_n(&FIX_NUMBER_OF_TXS_COMMITTED_THREADS, __ATOMIC_ACQUIRE) < NB_THREADS)
    {
      usleep(10);
    }
    gs_appInfo->info.isExit = 1;
    __atomic_thread_fence(__ATOMIC_RELEASE);
    ts2 = rdtscp();
  }
  else
  {
    // usleep(500000); // waits 0.5s for warm-up
    // resets statistics
    __atomic_thread_fence(__ATOMIC_RELAXED);
    startNow = 1;
    __atomic_thread_fence(__ATOMIC_RELEASE);
    ts1 = rdtscp();
    usleep(EXPERIMENT_TIME);
    gs_appInfo->info.isExit = 1;
    __atomic_thread_fence(__ATOMIC_RELEASE);
    ts2 = rdtscp();
  }

  for (int i = 0; i < NB_THREADS; ++i)
  {
    nbExecStates += count_txs[i];
  }

  timeBench = (ts2 - ts1) / CPU_FREQ;
  threading_join();

  // printf("threads joined! Checking log ...\n");
  // if (input_exists("usePCWC-NF") || input_exists("usePCWC-F")) {
  //   // TODO: check blocks
  //   gs_appInfo->info.isExit = 0;
  //   state_log_checker_pcwc(wait_fn, 1);
  //   gs_appInfo->info.isExit = 1;
  // }
  // printf("   ... log checked!\n");

  // printf("Duration: %f ms, execStates: %li (%f /s)\n",
  //   timeBench, nbExecStates, nbExecStates / timeBench * 1000.0f);

  FILE *fp = fopen(PROFILE_NVRAM_READS_FILE, "a+");
  if (fp == NULL)
  {
    printf("Cannot open %s! Try to remove it.\n", PROFILE_NVRAM_READS_FILE);
  }

  if (usePSTM)
  {
    bench_stm_exit();
    fseek(fp, 0L, SEEK_END);
    if (ftell(fp) < 8)
    {
      fprintf(fp, "#%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
              "THREADS",
              "NB_READS",
              "SAMPLES",
              "TOT_TIME",
              "READ_INSTRUMENTATION",
              "FLUSH_REDO_LOG",
              "FLUSH_DATA",
              "DESTROY_LOG",
              "ZIPF_ALPHA",
              "SIZE_WORKING_SET",
              "TOTAL_MEMORY");
    }
    extern unsigned long tot_pstm_time_flush_redo_log;
    extern unsigned long tot_pstm_time_flush_data;
    extern unsigned long tot_pstm_time_destroy_log;
    extern unsigned long tot_pstm_nb_samples;
    fprintf(fp, "%i\t%li\t%li\t%f\t%f\t%f\t%f\t%f\n", NB_THREADS, NB_READS, tot_bench_read_time_samples,
            (float)tot_bench_tot_time / (float)tot_bench_read_time_samples,
            (float)tot_bench_read_time / (float)tot_bench_read_time_samples,
            (float)tot_pstm_time_flush_redo_log / (float)tot_pstm_nb_samples,
            (float)tot_pstm_time_flush_data / (float)tot_pstm_nb_samples,
            (float)tot_pstm_time_destroy_log / (float)tot_pstm_nb_samples);
    bench_stm_print(NB_THREADS, timeBench / 1000.0);
  }
  else
  {
    fseek(fp, 0L, SEEK_END);
    if (ftell(fp) < 8)
    {
      fprintf(fp, "#%s\t%s\t%s\t%s\t%s\n",
              "THREADS",
              "NB_READS",
              "SAMPLES",
              "TOT_TIME",
              "READ_INSTRUMENTATION");
    }
    fprintf(fp, "%i\t%li\t%li\t%f\t%f\n", NB_THREADS, NB_READS, tot_bench_read_time_samples,
            (float)tot_bench_tot_time / (float)tot_bench_read_time_samples,
            (float)tot_bench_read_time / (float)tot_bench_read_time_samples);

    printf("%li commits (%li fallbacks), %li aborts (%li confl, %li capac, prob. %f) %f zipf_alpha\n",
           nbSuccess, nbFallback, nbAbort, nbConfl, nbCapac, (float)nbAbort / (nbAbort + nbSuccess), zipf_alpha);

    // NB_THREADS THROUGHPUT HTM_COMMITS SGL_COMMITS HTM_ABORTS HTM_CONFLICTS HTM_CAPACITY
    printf("%i\t%.0f\t%li\t%li\t%li\t%li\t%li\t%li\t%li\n", NB_THREADS, (double)nbExecStates / timeBench * 1000.0f,
           nbSuccess, nbFallback, nbAbort, nbConfl, nbCapac, nbExpli, nbOther);
    // printf("%i\t%.0f\n", TID0_SLOWDOWN, (double)nbExecStates / timeBench * 1000.0f);
    if (state_print_profile != NULL)
    {
      state_print_profile(PROFILE_FILE);
    }
    nvmalloc_print_stats(malloc_stats_file);
    global_structs_destroy();
  }
  #ifdef USE_PAGING
    paging_finish();
  #endif
  // #ifdef DO_PAGING
  //   pthread_mutex_destroy(&pager_mtx);
  //     #ifndef SYNC_PAGEOUT
  //         #ifdef USE_IMPROVED_PAGING
  //           pthread_cond_signal(&cv_not_enough_pages);
  //         #endif
  //         pthread_join(pager_thread, NULL);
  //     #endif
  // #endif

  // #ifdef DEBUG_PAGE
  //   printf("Final bitmap vector:\n");
  //   for (int i = 0; i < Paging.memory_heap_size/PAGE_SIZE /64 ; i++) {
  //     printf("%lx\n", pages[i]);
  //   }
  //   printf("PI: %ld - PO: %ld - POActi: %ld\n", nb_page_in, nb_page_out, PagerOutActivations);
  // #endif

  // printf(" --- latency after bench ---\n");
  // test_latency();


    //   extern uint64_t startCounter; //replayer counter
    // extern uint64_t startTxs; //txs counter
    
    // while (startTxs != startCounter){
    //   printf ("%lx %lx\n", startCounter, startTxs);
    //   usleep (10);
    // }
  return EXIT_SUCCESS;
}
