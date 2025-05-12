/* =============================================================================
 *
 * paging.c
 *
 * Available at: https://github.com/Daniel1993/SPHT-eADR/tree/paging_exp
 *
 * =============================================================================
 *
 * Paging module (paging.c + paging.h)
 * Enables paging for different workloads
 * In its current state, is limited to 1 NUMA Node operation
 * Has a self-contained operation
 *
 * ------------------------------------------------------------------------
 *
 * USAGE:
 *   paging_init() should be called before proper execution, and after operations are done, call paging_finish()
 *   Comment "#define DEBUG_PAGE" to disable paging related debug statistics
 *
 * =============================================================================
 */

#ifdef __linux__
#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 600
#define _BSD_SOURCE 1
#define _GNU_SOURCE 1
#define _DEFAULT_SOURCE 1
#endif /* __linux__ */

// #define PAGE_INDEX 1
// #define ASYNC_PAGER 1

#define DEBUG_PAGE
//#define USE_HASHMAP
// #define USE_TRYLOCK    // note that the implementation with trylock does not appear correct

#if defined(USE_HASHMAP) || defined(USE_SIMPLEHASH)
  #include "../include/hashmap.h"
#endif

#include "impl.h"
#include "paging.h"
// #include "extra_MACROS.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>

/* Begin includes for large heap management test */
#include <signal.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
/* End includes for large heap management test */

// int ip = 0;
static void rm_page(void *addr);
static void *select_page_out();

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

/* Begin test code for large heap management */

// should this be in HD or Optane?
#define SWAP_HEAP_FILE "./swap.heap"
//
//// default values (can be changed through command line)

// https://unix.stackexchange.com/questions/509607/how-a-64-bit-process-virtual-address-space-is-divided-in-linux
#define PAGE_SIZE 4096            // has a copy on main.c (uses it here and there), maybe put in paging.h?
#define PAGE_SIZE_BITS 12         // has a copy on main.c (uses it here and there), maybe put in paging.h?
#define PAGE_MASK 0b111111111111L // ==0xFFFL (12 bits, be sure you have the L in the end)

static void *HEAP_START_ADDR;
static uint64_t memory_heap_size;
static uint64_t memory_heap_mmap;
static float addPageThreshold;
static float rmPageThreshold;

#if defined(USE_HASHMAP) || defined(USE_SIMPLEHASH)
  hashtable_t *pagein_ht;
  uint64_t ht_consult_time = 0; //add_time that global ts is faster than replay_ts
  uint64_t ht_consult_count = 0;
  extern uint64_t ht_add_count; //pages added to hs
  // uint64_t htReadClockVal;
#endif


#ifdef DEBUG_PAGE
static uint64_t nb_page_in = 0;
static uint64_t nb_page_out = 0;
static uint64_t pagerOutActivations = 0;
// uint64_t init_nb_page_in = 0;
static uint64_t page_add_ticks = 0;
static uint64_t page_rm_ticks = 0;

static uint64_t page_sel_ticks = 0;
#endif

#ifdef ASYNC_PAGER
pthread_cond_t cv_not_enough_pages = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv_without_pages = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mt_memory_used = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mt_memory_full = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mt_hashmap = PTHREAD_MUTEX_INITIALIZER;
#endif

uint64_t *pages;

#ifdef PAGE_INDEX
// size of the page list
#define PL_SIZE 1000   
// the circular list holds the index to the 'pages' bitmap vector containing
// a 64-page bucket - the replacement policy will choose one of these 64 pages 
// to evict
  uint64_t *pagelist;
// the index points to the oldest page bucket: it is used by the replacement
// code to choose a page to evict and by the add_page code to insert the
// recently added page
  volatile uint64_t cur_page;
#endif


static volatile uint64_t curr_size_of_heap = 0;
static volatile int heap_fd = -1;
#ifdef USE_SWAP
  static uint64_t *swap_pages;   //bitmap with the state of the swappable pages
  static uint64_t countswap=0;
  static char *swap_area=NULL;
#endif

static pthread_mutex_t pager_mtx;
#ifdef ASYNC_PAGER
  static pthread_t pager_thread;
#endif
static pid_t my_pid;
static int pagemap_fd;
static int countSegFault;

// extern int waitForReplayer = 0;

static void print_page_data(uint64_t address)
{
  const uint64_t OS_PAGE_SIZE = 0x1000; // TODO: get this from some system header
  uint64_t data;
  uint64_t index = (address / OS_PAGE_SIZE) * sizeof(data);
  if (pread(pagemap_fd, &data, sizeof(data), index) != sizeof(data))
  {
    perror("pread pagemap");
    return;
  }
  printf("0x%lx: pfn %lx soft-dirty %ld exclusive %ld file/shared %ld "
         "swapped %ld present %ld\n",
         address,
         data & 0x7fffffffffffff,
         (data >> 55) & 1,
         (data >> 56) & 1,
         (data >> 61) & 1,
         (data >> 62) & 1,
         (data >> 63) & 1);
}

extern volatile __attribute__((aligned(ARCH_CACHE_LINE_SIZE)))
int32_t *G_flag_checkpointer_exit;

FILE *pghd;

static void add_page(void *addr)
{
  //printf ("add_page START %lx\n", addr);
  uint64_t start, end;
  uintptr_t toMap = (uintptr_t)addr;
  toMap &= ~PAGE_MASK;
  uintptr_t a = toMap;
  uint64_t *htReadClockVal = (uint64_t*) ((uint8_t*)G_flag_checkpointer_exit + sizeof(uint64_t) * 1024);
  
#ifdef DEBUG_PAGE
  uint64_t add_start_tick, add_end_tick;
#endif  
  
  // we cannot allow a thread to mmap if there is no free space
  // wait for the pager thread to do its thing
#ifdef ASYNC_PAGER
  pthread_mutex_lock(&mt_memory_used);
//    printf("Allocating %lu of %lu\n", curr_size_of_heap, memory_heap_mmap);
    while (__atomic_load_n(&curr_size_of_heap, __ATOMIC_ACQUIRE) >= memory_heap_mmap) {
      pthread_cond_wait(&cv_without_pages, &mt_memory_used);
    }
  pthread_mutex_unlock(&mt_memory_used);
#else
  pthread_mutex_lock(&pager_mtx);
  if (__atomic_load_n(&curr_size_of_heap, __ATOMIC_ACQUIRE) >= memory_heap_mmap) {
  /*
  uint64_t thresholdSup = (uint64_t)(((float)memory_heap_mmap) * addPageThreshold);
  if (__atomic_load_n(&curr_size_of_heap, __ATOMIC_ACQUIRE) >= thresholdSup) {
   
    uint64_t thresholdInf = (uint64_t)(((float)memory_heap_mmap) * rmPageThreshold);
    while (__atomic_load_n(&curr_size_of_heap, __ATOMIC_ACQUIRE) >= thresholdInf)
    { // dudetm -> change threshold to == 100%, or maybe 99% and keep >=
      // printf ("RM_PAGE THRESHOLD\n");
      rm_page(select_page_out());
    }
  }
  */
    
    int pages_to_remove = 100;
    while (pages_to_remove--)
    { 
      rm_page(select_page_out());
#ifdef PAGE_INDEX
      // advance to the next element which should containt the oldest page
      // bucket to remove
      cur_page = (cur_page+1) % PL_SIZE;
#endif
    }
  }
  pthread_mutex_unlock(&pager_mtx);
#endif

#ifdef DEBUG_PAGE
  A_MEASURE_TS(add_start_tick);
#endif

  //////////////////////////////
#if defined(USE_HASHMAP) || defined(USE_SIMPLEHASH)
  A_MEASURE_TS(start);
  uint64_t timestamp = ht_get(pagein_ht, toMap);
  if (timestamp != 0) {
  //      printf ("hashmap get: addr: %lx\ttimestamp: %lx - replayer %lx\n", hte->key, hte->value, __atomic_load_n(htReadClockVal, __ATOMIC_ACQUIRE));
    while (timestamp > __atomic_load_n(htReadClockVal, __ATOMIC_ACQUIRE)){ //htReadClockVal = timestamp of last tx replayed
      usleep(10);
      //  printf ("hte: %lx\t\thtRCV: %lx\n", hte, htReadClockVal);
    }
    //    printf ("hashmap get: addr: %lx\ttimestamp: %lx - replayer %lx\n", hte->key, hte->value, __atomic_load_n(htReadClockVal, __ATOMIC_ACQUIRE));
  }
  A_MEASURE_TS(end); 
  A_INC_PERFORMANCE_COUNTER(start,end,ht_consult_time);
	A_MEASURE_INC(ht_consult_count);
#endif

  a -= (uintptr_t)HEAP_START_ADDR;
  off_t location_in_file = (off_t)a;
  assert(a < memory_heap_size);
  a >>= PAGE_SIZE_BITS;
  int loc64 = a & 0x3F;
  int locPage = a >> 6; // log2(64) == 6
  pthread_mutex_lock(&pager_mtx);
  if ((1L << loc64) == (pages[locPage] & (1L << loc64)))
  {
    pthread_mutex_unlock(&pager_mtx);
    return; // already mapped
  }
  pages[locPage] |= (1L << loc64);

#ifdef PAGE_INDEX
  // insert the recently mapped page in the cur_page slot and advance
  // notice that the next element is the oldest one
  pagelist[cur_page] = locPage;
  cur_page = (cur_page+1) % PL_SIZE;
#endif

  curr_size_of_heap += PAGE_SIZE;
  __atomic_thread_fence(__ATOMIC_RELEASE);
  pthread_mutex_unlock(&pager_mtx);


//static long mmap_count = 0;
//mmap_count++;
//load the page
  void *res = mmap((void *)toMap, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, heap_fd, location_in_file);
  if ((uintptr_t)res != (void *)toMap) {
    perror("Error ");
    printf("addr = %lx\n", addr);
    printf("location  = %lu\n", location_in_file);
    printf("toMap = %lx\n", toMap);
    printf("heap_fd = %d\n", heap_fd);
//    printf("mmap count = %lu\n", mmap_count);
    exit(-11);
  }
  
#ifdef USE_SWAP
    // if the page's content is in the swap area, we need to copy it over
  if (swap_pages[locPage] & (1L << loc64)) {
    memcpy(toMap, swap_area+location_in_file, PAGE_SIZE);
    countswap++;
      
  }
#endif
      
  assert((uintptr_t)res == (void *)toMap && "could not map the requested page");


#ifdef ASYNC_PAGER
#ifdef USE_TRYLOCK
  pthread_mutex_trylock(&mt_memory_used);
#else
  pthread_mutex_lock(&mt_memory_used);
#endif
        
  uint64_t thresholdSup = (uint64_t)(((float)memory_heap_mmap) * addPageThreshold);
  if (__atomic_load_n(&curr_size_of_heap, __ATOMIC_ACQUIRE) >= thresholdSup)
  {
    pthread_cond_signal(&cv_not_enough_pages); // wakes up the pager thread to remove pages from DRAM
  }
        
  pthread_mutex_unlock(&mt_memory_used);
#endif

#ifdef DEBUG_PAGE
  nb_page_in++;
  
  A_MEASURE_TS(add_end_tick);
  A_INC_PERFORMANCE_COUNTER(add_start_tick, add_end_tick, page_add_ticks);
#endif
  //  printf ("add_page FINISHED %lx\n", res);
}


/*
void prepare_heap()
{
    char fileNameBuffer[1024];
    sprintf(fileNameBuffer, "%s%s", PM_DIR "0/", NVMALLOC_THREAD_PRIV_FILE "0");

    heap_fd = open(fileNameBuffer, O_CREAT | O_TRUNC | O_RDWR, 0666);
    close(heap_fd); // writes the permissions
    if (-1 == (heap_fd = open(fileNameBuffer, O_CREAT | O_RDWR, 0666)))
    fprintf(stderr, "Error open file \"%s\": %s\n", fileNameBuffer, strerror(errno));
//  if (heap_fd == -1)  // workaround - assigns only the first call (avoid using the replayer's fd)
    if (ftruncate(heap_fd, memory_heap_size*2))
      fprintf(stderr, "Error ftruncate file \"%s\": %s\n", fileNameBuffer, strerror(errno));
    // note, nothing is allocated here

    if ((pghd = fopen("pages.txt", "w")) == NULL) {
      printf("Error opening file\n");
      exit(-1);
    }
}
*/

#ifdef USE_SWAP
void prepare_swap()
{
    int heap_fd_swap = open(SWAP_HEAP_FILE, O_CREAT | O_TRUNC | O_RDWR, 0666);
    close(heap_fd_swap); // writes the permissions
    if (-1 == (heap_fd_swap = open(SWAP_HEAP_FILE, O_CREAT | O_RDWR, 0666)))
      fprintf(stderr, "Error open swap file \"%s\": %s\n", SWAP_HEAP_FILE, strerror(errno));
    if (ftruncate(heap_fd_swap, memory_heap_size))
      fprintf(stderr, "Error ftruncate swap file \"%s\": %s\n", SWAP_HEAP_FILE, strerror(errno));

    // we allocate and map the whole swap area with MAP_SHARED
    // there is a 1-1 correspondence with the swap area and the persistent
    // heap (for simplification)
    swap_area = mmap(NULL, memory_heap_size, PROT_READ | PROT_WRITE, 
        MAP_SHARED, heap_fd_swap, 0);
    if (swap_area == -1) {
      printf("Error mmaping the swap area\n");
      exit(-1);
    }

}
#endif

static void rm_page(void *addr)
{
  //printf ("rm_page START, addr: %lx\n", addr);
  uintptr_t toUnmap = (uintptr_t)addr;
  toUnmap &= ~PAGE_MASK;
  uintptr_t a = toUnmap;
  a -= (uintptr_t)HEAP_START_ADDR;
  off_t location_in_file = (off_t)a;
  assert(a < memory_heap_size);
  a >>= PAGE_SIZE_BITS;
  int loc64 = a & 0x3F;
  int locPage = a >> 6;
#ifdef DEBUG_PAGE
  uint64_t rm_start_tick, rm_end_tick;
#endif  

//  printf("before lock --> rm_page: %lx locPage %d - loc64 %d\n", addr, locPage, loc64); fflush(stdout);

// no need for the lock in the synchronous version (actually, it will deadlock
// since the lock is not reentrant)
#ifdef ASYNC_PAGER
  #ifdef USE_TRYLOCK
    pthread_mutex_trylock(&pager_mtx);
  #else
    pthread_mutex_lock(&pager_mtx);
  #endif
#endif

#ifdef DEBUG_PAGE
  A_MEASURE_TS(rm_start_tick);
#endif
  
  assert((1L << loc64) == (pages[locPage] & (1L << loc64)) && "bit was not set!");
  if (!((1L << loc64) == (pages[locPage] & (1L << loc64))))
  {
#ifdef ASYNC_PAGER
    pthread_mutex_unlock(&pager_mtx);
#endif
    // printf("oops\n");
    return; // already unmapped
  }
  pages[locPage] &= ~(1L << loc64);
  

#ifdef USE_SWAP
    // mark the page as swapped
    swap_pages[locPage] |= (1L << loc64);

    // copy the content to disk and flush
    memcpy(swap_area+location_in_file, toUnmap, PAGE_SIZE);
    int resms =  msync(swap_area+location_in_file, PAGE_SIZE, MS_SYNC);
    if (resms == -1) {
      printf("msync() failed\n");
      exit(-1);
    }
#endif


// printf("before munmap\n");
  int err = munmap((void *)toUnmap, PAGE_SIZE);
  if (err)
  {
    printf("Something wrong with the munmap!\n");
    exit(-1);
  }

#ifdef ASYNC_PAGER
  pthread_mutex_unlock(&pager_mtx);
#endif

  // TODO: mutex
  // TODO ----------

  ///      int err = munmap((void*)toUnmap, PAGE_SIZE);
  curr_size_of_heap -= PAGE_SIZE;
  __atomic_thread_fence(__ATOMIC_RELEASE);
  assert(!err && "could not unmap the page");
#ifdef DEBUG_PAGE
  nb_page_out++;
//  printf ("RM_PAGE FINISH\n");
  A_MEASURE_TS(rm_end_tick);
  A_INC_PERFORMANCE_COUNTER(rm_start_tick, rm_end_tick, page_rm_ticks);
#endif
}

static int rightmostbit(uint64_t b) // page has to be 1
{
  for (int i = 0; i < 64; ++i)
  {
    if (b & (1L << i))
    {
      return i;
    }
  }
  return -1;
}

static void *select_page_out()
{
#ifdef DEBUG_PAGE
  uint64_t sel_start_tick, sel_end_tick;
  
  A_MEASURE_TS(sel_start_tick);
#endif  
  uint64_t total_pages = (memory_heap_size / PAGE_SIZE) / 64;
  int count = 0;

#ifdef PAGE_INDEX
  uintptr_t rnd = (uintptr_t)pagelist[cur_page];
#else
  uintptr_t rnd = RAND_R_FNC(RND_FN_seed) % total_pages;
#endif
  while (0 == pages[rnd] && count < total_pages)
  {
    rnd = (rnd + 1) % total_pages;
    //printf("page[%d] = %lu -- count %lu\n", rnd, pages[rnd], count); fflush(stdout);
    count++;
  }

  if (0 != pages[rnd])
  {
    uintptr_t rmb = RAND_R_FNC(RND_FN_seed) % 64;
    for (int i = 0; i < 64; i++)
    {
      if (pages[rnd] & (1L << rmb))
        break;
      rmb = (rmb + 1) % 64;
    }
    uintptr_t offset = 64L * rnd + rmb; // RMB NOT TRULY RANDOM
    uintptr_t pagetorm = (void *)HEAP_START_ADDR + offset * PAGE_SIZE;
    //printf("pagetorm: %p\n", (void*)pagetorm);

#ifdef DEBUG_PAGE
  A_MEASURE_TS(sel_end_tick);
  A_INC_PERFORMANCE_COUNTER(sel_start_tick, sel_end_tick, page_sel_ticks);
#endif  
    return (void *)pagetorm;
  }

  assert(0 && "there are not pages to remove!");
  return NULL;  
}


static void full_write(int fd, const char *buf, size_t len)
{
  while (len > 0)
  {
    ssize_t ret = write(fd, buf, len);

    if ((ret == -1) && (errno != EINTR))
      break;

    buf += (size_t)ret;
    len -= (size_t)ret;
  }
}

void print_backtrace(void)
{
  static const char start[] = "BACKTRACE ------------\n";
  static const char end[] = "----------------------\n";

  void *bt[1024];
  int bt_size;
  char **bt_syms;
  int i;

  bt_size = backtrace(bt, 1024);
  bt_syms = backtrace_symbols(bt, bt_size);
  full_write(STDERR_FILENO, start, strlen(start));
  for (i = 1; i < bt_size; i++)
  {
    size_t len = strlen(bt_syms[i]);
    full_write(STDERR_FILENO, bt_syms[i], len);
    full_write(STDERR_FILENO, "\n", 1);
  }
  full_write(STDERR_FILENO, end, strlen(end));
  fflush(stdout);
  fflush(stderr);
  free(bt_syms);
}

void heap_sigsegv_handler(int sig, siginfo_t *si, void *unused)
{
  uintptr_t addr = (uintptr_t)si->si_addr;

  if (addr - (uintptr_t)HEAP_START_ADDR >= memory_heap_size)
  {
    // access is outside OUR heap! Thus, it is actually a SIGSEGV!
    printf("Got a true SIGSEGV at address: 0x%lx\n", addr);
    countSegFault++;

    getchar();

    //print_backtrace();

    exit(EXIT_FAILURE);
  }

  add_page((void *)addr);
}
/* End test code for large heap management */

#ifdef ASYNC_PAGER
void *pager_thread_fn(void *arg)
{
  while (!gs_appInfo->info.isExit)
  {

 #ifdef USE_TRYLOCK
    pthread_mutex_trylock(&mt_memory_used);
  #else
    pthread_mutex_lock(&mt_memory_used);
  #endif

    //      printf("pager thread will sleep\n");
    uint64_t thresholdSup = (uint64_t)(((float)memory_heap_mmap) * addPageThreshold);
    //      printf("pager: tamanho do threshold: %lu\n", thresholdSup);
    if (__atomic_load_n(&curr_size_of_heap, __ATOMIC_ACQUIRE) < thresholdSup)
    { // dudetm -> change threshold to < 100% or 99% just like below
      pthread_cond_wait(&cv_not_enough_pages, &mt_memory_used);
    }
    //      printf("pager thread woke up\n"); fflush(stdout);

#ifdef DEBUG_PAGE
    pagerOutActivations++;
#endif

    uint64_t thresholdInf = (uint64_t)(((float)memory_heap_mmap) * rmPageThreshold);
    while (__atomic_load_n(&curr_size_of_heap, __ATOMIC_ACQUIRE) >= thresholdInf)
    { // dudetm -> change threshold to == 100%, or maybe 99% and keep >=
      // printf ("RM_PAGE THRESHOLD\n");
      rm_page(select_page_out());
#ifdef PAGE_INDEX
      // advance to the next element which should containt the oldest page
      // bucket to remove
      cur_page = (cur_page+1) % PL_SIZE;
#endif
    }
    /*
    int pages_to_remove = 100;
    while (pages_to_remove--)
    { 
      rm_page(select_page_out());
    }
    */
    pthread_cond_signal(&cv_without_pages);
    pthread_mutex_unlock(&mt_memory_used);

    // tell any possible thread waiting for page space to go
    //pthread_mutex_lock(&mt_memory_full);   // we do not need the lock?
    //pthread_cond_signal(&cv_without_pages);
    //pthread_mutex_unlock(&mt_memory_full);
    //      printf("pager thread paged out some memory\n");
  }
  // printf("csh: %ld, mhm: %ld, percentage: %ld\n", curr_size_of_heap, memory_heap_mmap, curr_size_of_heap*100/memory_heap_mmap);
  return NULL;
}
#endif

#define HASH_SIZE 500000

void paging_init(struct paging_init_var Paging)
{

#if defined(USE_HASHMAP) || defined(USE_SIMPLEHASH)
  pagein_ht = ht_new(HASH_SIZE); //bucket count
    // printf ("HASHTABLE INIT\n");
#endif
  HEAP_START_ADDR = Paging.HEAP_START_ADDR; // works as the old define, thats why i kept the capital letters
  memory_heap_size = Paging.memory_heap_size;
  memory_heap_mmap = Paging.memory_heap_mmap;
  addPageThreshold = Paging.addPageThreshold;
  rmPageThreshold = Paging.rmPageThreshold;


  printf("Initializing paging module...\n");
#ifdef USE_HASHMAP
  printf("--- HASHMAP enabled - SIZE = %ld\n", HASH_SIZE);
#endif
#ifdef USE_SIMPLEHASH
  printf("--- SIMPLEHASH enabled - SIZE = %ld\n", HASH_SIZE);
#endif
#ifdef USE_SWAP
  printf("--- SWAP enabled\n");
#endif
#ifdef ASYNC_PAGER
  printf("--- ASYNC_PAGER enabled\n");
#else
  printf("--- SYNC_PAGER enabled\n");
#endif
#ifdef PAGE_INDEX
  printf("--- PAGE_INDEX enabled\n");
#endif
  printf("--- memory_heap_mmap: %ld, memory_heap_size: %ld\n", memory_heap_mmap, memory_heap_size);


  // printf("Size of pages[] = %lu\n", (Paging.memory_heap_size/PAGE_SIZE)/64);
  pages = (uint64_t *)calloc((Paging.memory_heap_size / PAGE_SIZE) / 64, sizeof(uint64_t));
  if (pages == NULL)
  {
    perror("Could not allocate bitmap memory");
    exit(-1);
  }
#ifdef USE_SWAP
  prepare_swap();

  // bitmap for the swapped pages
  swap_pages = (uint64_t *)calloc((Paging.memory_heap_size / PAGE_SIZE) / 64, sizeof(uint64_t));
  if (swap_pages == NULL)
  {
    perror("Could not allocate bitmap memory for the swap area");
    exit(-1);
  }
#endif
#ifdef PAGE_INDEX
  pagelist = (uint64_t *)malloc(PL_SIZE*sizeof(uint64_t));
  if (pagelist == NULL)
  {
    perror("Could not allocate pagelist");
    exit(-1);
  }
  cur_page = 0;
#endif

  // my_pid = getpid();
  // char filename[128];
  // snprintf(filename, sizeof(filename), "/proc/%d/pagemap", my_pid);
  // if ((pagemap_fd = open(filename, O_RDONLY)) < 0)
  //   perror("cannot open page mapping file");

  struct sigaction sa;
  int *a;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = heap_sigsegv_handler;
  if (-1 == sigaction(SIGSEGV, &sa, NULL))
    perror("sigaction()");

// printf ("p3\n");
  void *toUnmap = HEAP_START_ADDR;
  // printf("Unmapping (mhs): %lu from base address %lx\n", memory_heap_size*0.5, toUnmap);
  int err = munmap(toUnmap, memory_heap_size);
  if (err)
    perror("munmap failed()");
  if (-1 == sigaction(SIGSEGV, &sa, NULL))
    perror("sigaction()");

// printf ("p4\n");
  /*
   * We are opening a file in the SSD (some errors with PM) but it should not
   * be an issue because we are using MAP_PRIVATE when mapping
   */
  //prepare_heap();

  // FIXME: make sure the file was already created
  char fileNameBuffer[1024];
  sprintf(fileNameBuffer, "%s%s", PM_DIR "0/", NVMALLOC_THREAD_PRIV_FILE "0");
  heap_fd = open(fileNameBuffer, O_RDWR);
  pthread_mutex_init(&pager_mtx, NULL); // TODO: check error

#ifdef ASYNC_PAGER
  if (pthread_create(&pager_thread, NULL, pager_thread_fn, NULL))
    printf("Error Create Thread\n");
#endif
  // printf ("FINISHED PAGING_INIT\n");
}

void paging_reset_stats()
{
#if defined(USE_HASHMAP) || defined(USE_SIMPLEHASH)
  ht_consult_time = 0; 
  ht_consult_count = 0;
  ht_add_count = 0; 
#endif

#ifdef DEBUG_PAGE
  nb_page_in = 0;
  nb_page_out = 0;
  pagerOutActivations = 0;
    
  page_add_ticks = 0;
  page_rm_ticks = 0;
  page_sel_ticks = 0;
#endif
}

void paging_finish()
{
  // printf ("START PAGING_FINISH\n");
  __atomic_thread_fence(__ATOMIC_RELAXED);
  gs_appInfo->info.isExit = 1;
  __atomic_thread_fence(__ATOMIC_RELEASE);
#ifdef ASYNC_PAGER
  pthread_cond_signal(&cv_not_enough_pages);
  pthread_join(pager_thread, NULL);
#endif
  pthread_mutex_destroy(&pager_mtx);
  #if defined(USE_HASHMAP) || defined(USE_SIMPLEHASH)
    // ht_print(pagein_ht);
    ht_free(pagein_ht);
  #endif

  #ifdef DEBUG_PAGE
    // printf("Final bitmap vector:\n");
    // for (int i = 0; i < memory_heap_size/PAGE_SIZE /64 ; i++) {
    //   printf("%lx\n", pages[i]);
    // }
    // printf("PI: %ld - PO: %ld - POActi: %ld\n", nb_page_in, nb_page_out, pagerOutActivations);
    // printf("PI\tPO\tPOActi\n");
    fflush(stdout);
    #if defined(USE_HASHMAP) || defined(USE_SIMPLEHASH)
        printf("\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%g\t%lu\t%g\t%lu\t%g\t%lu\t%lu\n", nb_page_in, nb_page_out, (nb_page_in*100)/(nb_page_in+nb_page_out), pagerOutActivations, (((nb_page_in - nb_page_out) * PAGE_SIZE) * 100) / memory_heap_mmap, ht_consult_time, ht_consult_count, ht_add_count, (float)page_add_ticks/3000000.0, page_add_ticks, (float)page_rm_ticks/3000000.0, page_rm_ticks, (float)page_sel_ticks/3000000.0, page_sel_ticks, curr_size_of_heap);
    #else
        #ifdef USE_SWAP
          // printf("Number of pages swapped in: %ld\n", countswap);
          printf("\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%g\t%lu\t%g\t%lu\t%g\t%lu\t%lu\n", nb_page_in, nb_page_out, (nb_page_in*100)/(nb_page_in+nb_page_out), pagerOutActivations, (((nb_page_in - nb_page_out) * PAGE_SIZE) * 100) / memory_heap_mmap, countswap, (float)page_add_ticks/3000000.0, page_add_ticks, (float)page_rm_ticks/3000000.0, page_rm_ticks, (float)page_sel_ticks/3000000.0, page_sel_ticks, curr_size_of_heap);  
          free(swap_pages);
        #else 
          printf("\t%ld\t%ld\t%ld\t%ld\t%ld\n", nb_page_in, nb_page_out, (nb_page_in*100)/(nb_page_in+nb_page_out), pagerOutActivations, (((nb_page_in - nb_page_out) * PAGE_SIZE) * 100) / memory_heap_mmap);  
        #endif
    #endif

    // printf("Time spent with add_page : %g (ms) - %lu ticks\n", (float)page_add_ticks/3000000.0, page_add_ticks);
    // printf("Time spent with rm_page  : %g (ms) - %lu ticks\n", (float)page_rm_ticks/3000000.0, page_rm_ticks);
    // printf("Time spent with page sel : %g (ms) - %lu ticks\n", (float)page_sel_ticks/3000000.0, page_sel_ticks);

    fflush(stdout);
  #endif
  
  // printf("Final working copy size is %lu\n", curr_size_of_heap);
  free(pages);
}
