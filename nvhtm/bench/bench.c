#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE   600
#define _BSD_SOURCE     1
#define _GNU_SOURCE     1
#define _DEFAULT_SOURCE 1

//#define USE_HASHMAP

#include "bench.h"
#include "spins.h"
#include "impl.h"
#include "htm_impl.h"
#include "global_structs.h"
#include <setjmp.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h> 
#include <math.h>

#if defined(USE_HASHMAP) || defined(USE_SIMPLEHASH)
#include "../include/hashmap.h"
#define HT_MASK ~0x0FFFUL
extern hashtable_t *pagein_ht; //bucket count
#endif

#  include <mod_mem.h>
#  include <mod_stats.h>
#  include <stm.h>

#define USE_ZIPF

//zipf, qualquer alpha, mas n vai ser proporcional a faixa de enderecamento (tamanho da memoria total)
#define RAND_R_FNC(seed) ({ \
    uint64_t next = seed; \
    uint64_t result; \
    next *= 1103515245; \
    next += 12345; \
    result = (uint64_t) (next / 65536) % 2048; \
    next *= 1103515245; \
    next += 12345; \
    result <<= 10; \
    result ^= (uint64_t) (next / 65536) % 1024; \
    next *= 1103515245; \
    next += 12345; \
    result <<= 10; \
    result ^= (uint64_t) (next / 65536) % 1024; \
    seed = next; \
    result; \
})

static __thread uint64_t seed = 12345;
static __thread uint64_t seedSet = 0;
static __thread long x = 12345;

#define  FALSE          0       // Boolean false
#define  TRUE           1       // Boolean true

double rand_val()//int seed)
{
  const long  a =      16807;  // Multiplier
  const long  m = 2147483647;  // Modulus
  const long  q =     127773;  // m div a
  const long  r =       2836;  // m mod a
  // static long x;               // Random int value
  long        x_div_q;         // x divided by q
  long        x_mod_q;         // x modulo q
  long        x_new;           // New x value

  // Set the seed if argument is non-zero and then return zero
  // if (seed > 0)
  // {
  //   x = seed;
  //   return(0.0);
  // }
  // RNG using integer arithmetic
  x_div_q = x / q;
  x_mod_q = x % q;
  x_new = (a * x_mod_q) - (r * x_div_q);
  if (x_new > 0)
    x = x_new;
  else
    x = x_new + m;
  // Return a random value between 0.0 and 0.7
  return((double) x / m);
}

int zipf(double alpha, int n){
  static int first = TRUE;      // Static first time flag
  static double c = 0;          // Normalization constant
  static double *sum_probs;     // Pre-calculated sum of probabilities
  double z;                     // Uniform random number (0 < z < 1)
  int zipf_value;               // Computed exponential value to be returned
  int i;                        // Loop counter
  int low, high, mid;           // Binary-search bounds

  // Compute normalization constant on first call only
  if (first == TRUE)
  {
    for (i=1; i<=n; i++)
      c = c + (1.0 / pow((double) i, alpha));
    c = 1.0 / c;
    sum_probs = malloc((n+1)*sizeof(*sum_probs));
    sum_probs[0] = 0;
    for (i=1; i<=n; i++) {
      sum_probs[i] = sum_probs[i-1] + c / pow((double) i, alpha);
    }
    first = FALSE;
  }
  // Pull a uniform random number (0 < z < 1)
  do{
    z = rand_val();//0);
    // z = RAND_R_FNC(seed);
  }while ((z == 0) || (z == 1));

  // Map z to the value
  low = 1, high = n, mid;
  do{
    mid = floor((low+high)/2);
    if (sum_probs[mid] >= z && sum_probs[mid-1] < z) {
      zipf_value = mid;
      break;
    }else if (sum_probs[mid] >= z) {
      high = mid-1;
    }else {
      low = mid+1;
    }
  }while (low <= high);
  // // Assert that zipf_value is between 1 and N
  // assert((zipf_value >=1) && (zipf_value <= n));
  return(zipf_value);
}

void bench_no_conflicts(void *largeMemRegion, int threadId, long spinsExec, void(*inTxFn)(void* arg), void* arg)
{
  uint64_t * addr1 = (uint64_t *) (&((int8_t*)largeMemRegion)[256 * threadId]);
  uint64_t * addr2 = (uint64_t *) (&((int8_t*)largeMemRegion)[256 * threadId + 8]);

  NV_HTM_BEGIN(threadId);

  onBeforeWrite(threadId, addr1, threadId);
  *addr1 = threadId;

  onBeforeWrite(threadId, addr2, threadId);
  *addr2 = threadId;

  inTxFn(arg);
  spin_fn(spinsExec);

  NV_HTM_END(threadId);
}

__thread long bench_read_time = 0;
__thread long bench_tot_time = 0;
__thread long bench_read_time_samples = 0;
static __thread long sampleCount = 0;
#define SAMPLE_COUNT 32

void bench_no_conflicts_with_reads(
  void *largeMemRegion,
  long size_region,
  int threadId,
  long nbReads,
  long nbWrites,
  void(*inTxFn)(void* arg),
  void* arg) //,
  // int percent_page_faults)
{
  // each thread has 1MB
  // 2147483648
  // uint64_t threadArea = 16777216L / sizeof(uint64_t) / gs_appInfo->info.nbThreads;
  // extern double zipf_alpha;
  uint64_t threadArea = size_region / sizeof(uint64_t);
  uint64_t readAddrAcc = 0;
  int index, indexb;
  uint64_t *addr;
  volatile uint64_t ts0, ts1, ts2, ts3;

  if (!seedSet) {
    seed *= (threadId + 12345);
    seedSet = 1;
  }

  uint64_t save_seed = seed;
  // // prefetch
  // for (int i = 0; i < nbReads + nbWrites; ++i) {
  //   index = (RAND_R_FNC(seed) % threadArea);
  //   addr = &((uint64_t*)largeMemRegion)[index];
  //   __builtin_prefetch(addr, 1, 3);
  //   // readAddrAcc += *addr;
  //   // *addr = readAddrAcc; // TODO: writing here reduces spurious aborts...
  // }
  // // ---------
// printf ("\nbncwr1\n");

  sampleCount++;
  if ((sampleCount & (SAMPLE_COUNT-1)) == (SAMPLE_COUNT-1)) {
    ts0 = rdtscp();
  }

  // extern uint64_t *pages;
  // index = zipf(0.7,threadArea)-1;
  // indexb = index >> 9; //removes offset
  // int loc64    = indexb & 0x3F;
  // int locPage  = indexb >> 6;
  // //pages[locPage] &= ~(1L<<loc64);
  // printf("%d - %d\n", index, pages[locPage] & (1L<<loc64)?1:0); //save to file


  NV_HTM_BEGIN(threadId);
  seed = save_seed;

  if ((sampleCount & (SAMPLE_COUNT-1)) == (SAMPLE_COUNT-1)) {
    ts1 = rdtscp();
  }
  readAddrAcc = 0;
  
  // printf ("\nbncwr2\n");

  // index = (RAND_R_FNC(seed) % threadArea);
  // index = zipf(0.7,threadArea)-1;
  // printf("%p %d\n", largeMemRegion, index);
  for (int i = 0; i < nbReads; ++i) {
    // index = (RAND_R_FNC(seed) % threadArea);
    // int p = (RAND_R_FNC(seed) % 100)+1;
    // index = return_page_address (index, p <= percent_page_faults ? 1 : 0);
    #ifdef USE_ZIPF
      index = zipf(zipf_alpha,threadArea)-1;
    #else
      index = (RAND_R_FNC(seed) % threadArea);
    #endif
        // printf ("\nbncwr3\n");

    // printf("l226 lmrindex %p, lmr %p, index %ld\n\n", &((void*)largeMemRegion)[index], (void*)largeMemRegion, index);
    // printf("%d %d\n",zipf(0.7,size_region) % threadArea, index);
    addr = &((uint64_t*)largeMemRegion)[index];

    // printf("l230 readAddrAcc1 %ld, addr %p, addrData %ld\n", readAddrAcc, addr, *addr);
    readAddrAcc += *addr;
    // printf("l232 readAddrAcc2 %ld\n", readAddrAcc);

    // printf("[%i] readAddrAcc = %lx\n", threadId, readAddrAcc);
    // index = (index + 1) % threadArea;
  }
  // printf ("\nbncwr3.5\n");
  if ((sampleCount & (SAMPLE_COUNT-1)) == (SAMPLE_COUNT-1)) {
    ts2 = rdtscp();
    bench_read_time += ts2 - ts1;
  }
    // printf ("\nbncwr4\n");

  // if ((RAND_R_FNC(seed) % 1024) == 0) {
    for (int i = 0; i < nbWrites; ++i) {
      // index = (RAND_R_FNC(seed) % threadArea);
      // index /= 1024; // TODO: makes transactions abort less
      //addr = &((uint64_t*)largeMemRegion)[index];
      // int p = (RAND_R_FNC(seed) % 100)+1;
      // index = return_page_address (index, p <= percent_page_faults ? 1 : 0);
      #ifdef USE_ZIPF
        index = zipf(zipf_alpha,threadArea)-1; //--> thread area = 131072
      #else
        index = (RAND_R_FNC(seed) % threadArea);
      #endif
      // printf("%d %d\n",zipf(0.7,size_region) % threadArea, index);
      addr = &((uint64_t*)largeMemRegion)[index];
      uint64_t value_to_write = readAddrAcc + 1;
      onBeforeWrite(threadId, addr, value_to_write);
      *addr = value_to_write;
#if defined(USE_HASHMAP) || defined(USE_SIMPLEHASH)
        uintptr_t ht_addr = (uintptr_t)addr & (HT_MASK);
        ht_add(pagein_ht, ht_addr, ts2);
      #endif
      // printf("addr: %p || vtw: %lx\n", addr, value_to_write);
      // index = (index + 1) % threadArea;

      // index = i + threadId * threadArea;
      // addr = &((uint64_t*)largeMemRegion)[index];
      // onBeforeWrite(threadId, addr, readAddrAcc + threadId + 1);
      // *addr = readAddrAcc + threadId + 1;
    }
  // }
      // printf ("\nbncwr5\n");
  
  inTxFn(arg);

  NV_HTM_END(threadId);

  if ((sampleCount & (SAMPLE_COUNT-1)) == (SAMPLE_COUNT-1)) {
    ts3 = rdtscp();
    bench_tot_time += ts3 - ts0;
    bench_read_time_samples++;
  }
}

void* bench_stm_init(int sameMemPool, long size)
{
  stm_init();
  mod_mem_init(0);
  mod_stats_init();
  if (sameMemPool) {
    return pstm_nvmalloc(size);
  } else {
    return NULL;
  }
}

void* bench_palloc(int id, long size)
{
  return pstm_local_nvmalloc(id, size);
}

void bench_stm_exit()
{
  stm_exit();
}

void bench_stm_init_thread()
{
  stm_init_thread();
}

void bench_stm_exit_thread()
{
  stm_exit_thread();
}

void bench_stm_print(int nbThreads, double duration)
{
  unsigned long exec_commits, exec_aborts;
  stm_get_global_stats("global_nb_commits", &exec_commits);
  stm_get_global_stats("global_nb_aborts", &exec_aborts);
  printf("#"
    "THREADS\t"
    "THROUGHPUT\t"
    "TIME\t"
    "COMMITS\t"
    "ABORTS\n"
  );
  printf("%i\t", nbThreads);
  printf("%f\t", (double)exec_commits / duration);
  printf("%f\t", duration);
  printf("%li\t", exec_commits);
  printf("%li\t", exec_aborts);
  printf("%li\t", 0L);
  printf("%li\n", 0L);
  printf("%li\t", 0L);
}

void bench_no_conflicts_with_reads_stm(
  void *largeMemRegion,
  long size_region,
  int threadId,
  long nbReads,
  long nbWrites,
  void(*inTxFn)(void* arg),
  void* arg)
{
  // each thread has 1MB

  // 2147483648
  // uint64_t threadArea = 16777216L / sizeof(uint64_t) / gs_appInfo->info.nbThreads;
  uint64_t threadArea = size_region / sizeof(uint64_t);
  uint64_t readAddrAcc = 0;
  int index;
  uint64_t *addr;
  volatile uint64_t ts0, ts1, ts2, ts3;
  // extern double zipf_alpha;

  if (!seedSet) {
    seed *= (threadId + 12345);
    seedSet = 1;
  }

  uint64_t save_seed = seed;
  // // prefetch
  // for (int i = 0; i < nbReads + nbWrites; ++i) {
  //   index = (RAND_R_FNC(seed) % threadArea);
  //   addr = &((uint64_t*)largeMemRegion)[index];
  //   __builtin_prefetch(addr, 1, 3);
  //   // readAddrAcc += *addr;
  //   // *addr = readAddrAcc;
  // }
  // // ---------

  sampleCount++;
  if ((sampleCount & (SAMPLE_COUNT-1)) == (SAMPLE_COUNT-1)) {
    ts0 = rdtscp();
  }

  stm_tx_attr_t _a = {};
  sigjmp_buf *buf = stm_start(_a);
  sigsetjmp(buf, 0);

  seed = save_seed;

  if ((sampleCount & (SAMPLE_COUNT-1)) == (SAMPLE_COUNT-1)) {
    ts1 = rdtscp();
  }
  // index = (RAND_R_FNC(seed) % threadArea);
  readAddrAcc = 0;
  for (int i = 0; i < nbReads; ++i) {
    #ifdef USE_ZIPF
      index = (zipf(zipf_alpha,threadArea)); //--> thread area = 131072
    #else
      index = (RAND_R_FNC(seed) % threadArea);
    #endif
    // printf("%d %d\n",zipf(0.7,size_region) % threadArea, index);
    addr = &((uint64_t*)largeMemRegion)[index];
    readAddrAcc += stm_load((volatile stm_word_t *)(void *)addr);
    index = (index + 1) % threadArea;
  }
  if ((sampleCount & (SAMPLE_COUNT-1)) == (SAMPLE_COUNT-1)) {
    ts2 = rdtscp();
    bench_read_time += ts2 - ts1;
  }
  for (int i = 0; i < nbWrites; ++i) {
    #ifdef USE_ZIPF
      index = (zipf(zipf_alpha,threadArea)); //--> thread area = 131072
    #else
      index = (RAND_R_FNC(seed) % threadArea);
    #endif
    // printf("%d %d\n",zipf(0.7,size_region) % threadArea, index);
    // index /= 1024; // TODO: makes transactions abort less
    addr = &((uint64_t*)largeMemRegion)[index];
    uint64_t value_to_write = readAddrAcc + 1;
    // index = (index + 1) % threadArea;

    stm_store((volatile stm_word_t *)(void *)addr, (stm_word_t)value_to_write);

    // index = i + threadId * threadArea;
    // addr = &((uint64_t*)largeMemRegion)[index];
    // onBeforeWrite(threadId, addr, readAddrAcc + threadId + 1);
    // *addr = readAddrAcc + threadId + 1;
  }

  inTxFn(arg);

  stm_commit();

  if ((sampleCount & (SAMPLE_COUNT-1)) == (SAMPLE_COUNT-1)) {
    ts3 = rdtscp();
    bench_tot_time += ts3 - ts0;
    bench_read_time_samples++;
  }
}

void bench_random_with_reads(void *largeMemRegion, int threadId, long nbReads, long nbWrites, void(*inTxFn)(void* arg), void* arg)
{
  // TODO:
  // static __thread int seed = 0x1234; // TODO: need to be different for each thread
}
