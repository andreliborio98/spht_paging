
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h> 
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <emmintrin.h>
#include <sched.h>

#include <threads.h>


#include "nvm.h"

#define BSIZE    (1024*1024*1024)   // 1 GB
//#define BSIZE    (1024*1024*4)   // 1 GB
#define TSIZE    (1024*1024*512)   // target - 128MB
//#define TSIZE    (1024*128)   // target - 128MB
#define LOGNAME  "log-test"
#define SNAPNAME "snapshot-test"

#ifndef PM_DIR
#define PM_DIR "/mnt/nvram"
#endif /* PM_DIR */


int g_useNuma = 1;
int64_t *pLog[2];
int64_t *pSnapshot[2];


const char* NVRAM_REGIONS[] = {PM_DIR "0/", PM_DIR "1/"};

typedef struct thr_parms {
  long int id;            
  int64_t *start_addr;
  int64_t *dest_addr;
  size_t length;          // in words
  size_t tlength;         // in words
} thr_parms_t;


int applier(void *data)
{
  thr_parms_t *mydata = (thr_parms_t *)data;

  /*
  fprintf(stdout, "thread id %d\n", mydata->id);
  fprintf(stdout, "start addr: %p\n", mydata->start_addr);
  fprintf(stdout, "dest  addr: %p\n", mydata->dest_addr);
  fprintf(stdout, "length: %d\n", mydata->length);
  */
  
  const int *NUMApin = g_useNuma == 1 ? G_PINNING_1 : G_PINNING_2;  
  int core = NUMApin[mydata->id];

  int numaSocket = G_NUMA_PINNING[core];
    
  fprintf(stdout, "Thread %d pinned to Core %d and NUMA node %d\n",
                   mydata->id, core, numaSocket);

  //threading_pinThisThread(core);
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(core, &cpu_set);
  sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
 
  uint64_t copies = 0;

  
	uintptr_t uptr = 0;
  for (size_t s = mydata->length; s > 0; s=s-2) { 

    // read two values sequentially, write to a random position at the target

    uint64_t *addr = (uint64_t *)*(mydata->start_addr);
    uint64_t value = *(mydata->start_addr+1);

#ifdef USE_NT_STORE    
    _mm_stream_si64 ((long long int *)addr, value);
#else
    *(addr) = value;

#ifdef USE_FLUSH_PER_STORE    
    uintptr_t t = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
	  if (uptr != t) {
      uptr = t;
      pmem_clwb((char *)(uptr));
    }
#endif

#endif

    copies++;
    mydata->start_addr += 2;
  }
  


#if defined(USE_WBINVD)
  int pid, status;
  if ((pid = fork()) == 0) {
    // child
    int r = execl("/usr/bin/cat", "cat","/proc/wbinvd",NULL);
    if (r) perror("oh no\n");
  }
  else (waitpid(pid, &status, 0)); 
#endif


  return copies;
}


static void *alocateInNVRAM(const char *memRegion, const char *file, size_t bytes, long mapFlag)
{
  char fileNameBuffer[1024];
  void *res = NULL;
  int fd;

  sprintf(fileNameBuffer, "%s%s", memRegion, file);
  fd = open(fileNameBuffer, O_CREAT | O_TRUNC | O_RDWR, 0666);
  close(fd); // writes the permissions
  
  fd = open(fileNameBuffer, O_CREAT | O_RDWR, 0666);

  if (fd == -1) {
    fprintf(stderr, "Error open file %s: %s\n", fileNameBuffer, strerror(errno));
    return 0;
  }

  if (ftruncate(fd, bytes)) {
    fprintf(stderr, "Error ftruncate file %s: %s\n", fileNameBuffer, strerror(errno));
    return 0;
  }

  res = mmap(NULL, bytes, PROT_READ | PROT_WRITE, mapFlag, fd, 0);
  if (res == (void*)-1 || res == NULL) {
    fprintf(stderr, "Error mmapping file %s: %s\n", fileNameBuffer, strerror(errno));
    return 0;
  }
  
  return res;
}



static void AllocateMemory()
{
#ifdef USE_DRAM

  for (int i=0; i<g_useNuma; i++) {
    pLog[i] = (uint64_t *)calloc(BSIZE,1);
    if (pLog[i] == NULL) {
      printf("Error allocating memory for the logs\n");
      exit(1);
    }
    
    pSnapshot[i] = (uint64_t *)calloc(TSIZE,1);
    if (pSnapshot[i] == NULL) {
      printf("Error allocating memory for the heap\n");
      exit(1);
    }
  }

#else
  
  for (int i=0; i<g_useNuma; i++) {
    pLog[i] = (uint64_t *)alocateInNVRAM(NVRAM_REGIONS[i], LOGNAME, BSIZE, MAP_SHARED);
    if (pLog[i] == NULL) {
      printf("Error allocating memory for the logs\n");
      exit(1);
    }
    pSnapshot[i] = (uint64_t *)alocateInNVRAM(NVRAM_REGIONS[i], SNAPNAME, TSIZE, MAP_SHARED);
    if (pSnapshot[i] == NULL) {
      printf("Error allocating memory for the heap\n");
      exit(1);
    }
  }
  

#endif

}



int main(int argc, char *argv[])
{
  int num_threads;
  struct timeval start, end;
  double elapsed_time;
  thr_parms_t *threadData;
  thrd_t *threadId;


  if (argc <= 2) {
    fprintf(stderr, "Invalid parameter number: use <threads> <#NUMA nodes>\n");
    exit(-1);
    }

  num_threads = strtol(argv[1], NULL, 10);
  fprintf(stdout, "Using %d threads\n", num_threads);

  g_useNuma = strtol(argv[2], NULL, 10);
  fprintf(stdout, "Using %d NUMA node(s)\n", g_useNuma);


  threadData = (thr_parms_t *)malloc(sizeof(thr_parms_t)*num_threads);
  threadId = (thrd_t *)malloc(sizeof(thrd_t)*num_threads);
  // TODO: check for correct allocation

  AllocateMemory();

#define TEST_LOG

#ifdef TEST_LOG
  uint64_t *dummyHeap[2];

  for (int i=0; i<g_useNuma; i++) {
    dummyHeap[i] = (uint64_t *)calloc(TSIZE,1);
    if (dummyHeap[i] == NULL) {
      printf("Error allocating dummy heap\n");
      exit(1);
    }
  }
#endif

  int length = (BSIZE/sizeof(int64_t))/num_threads;
  int tlength = (TSIZE/sizeof(int64_t))/num_threads;
  
  srand(time(0));

  for (int i=0; i < num_threads; i++) {
    threadData[i].id = i;
    threadData[i].start_addr = pLog[i%g_useNuma]+((i/g_useNuma)*length);
    threadData[i].dest_addr = pSnapshot[i%g_useNuma]+((i/g_useNuma)*tlength);
    threadData[i].length = length; // source
    threadData[i].tlength = tlength; // target
  
#ifdef TEST_LOG
    int old_rrr;
#endif
    for (int k=0; k<length; k++) {
      int rrr = rand()%tlength;
      threadData[i].start_addr[k] = (uint64_t)(threadData[i].dest_addr+rrr);
#ifdef TEST_LOG
      if (k%2 == 1)
        dummyHeap[i%g_useNuma][old_rrr] = threadData[i].start_addr[k];
      else
        old_rrr = ((i/g_useNuma)*tlength) + rrr;
#endif
    }
  
    fprintf(stdout, "thread id %d\n", threadData[i].id);
    fprintf(stdout, "start addr: %p (NUMA %d) \n", 
        threadData[i].start_addr, i%g_useNuma);
    fprintf(stdout, "dest  addr: %p (NUMA %d) \n", 
        threadData[i].dest_addr, i%g_useNuma);
    fprintf(stdout, "length: %d\n", threadData[i].length);
    fprintf(stdout, "tlength: %d\n", threadData[i].tlength);
  }
 

  gettimeofday(&start, NULL);

  for (int i=0; i < num_threads; ++i) {
    if (thrd_create(threadId+i, applier, threadData+i) != thrd_success) {
      printf("%d-th thread create error\n", i);
      return 0;
    }
  }
        
  int total_entries_applied = 0;
  for (int i=0; i < num_threads; ++i) {
    int entries_applied;
    thrd_join(threadId[i], &entries_applied);
    total_entries_applied += entries_applied;
  }


#ifdef BATCH_FLUSH
  for (int i=0; i<g_useNuma; i++) {
    flush_clwb_nolog((void *)pSnapshot[i], TSIZE/g_useNuma);
  }
  _mm_sfence();
#endif

#ifdef USE_FLUSH_PER_STORE    
  _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif
 

  gettimeofday(&end, NULL);

  elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
  fprintf(stdout, "Elapsed time: %g s\n", elapsed_time);
  fprintf(stdout, "Entries applied: %lu\n", total_entries_applied);
  fprintf(stdout, "Throughput: %g entries/s\n", (float)total_entries_applied/elapsed_time);

 
#ifdef TEST_LOG
  length = (TSIZE/sizeof(int64_t));
  for (int n=0; n<g_useNuma; n++) {
    for (size_t s = 0; s<length/g_useNuma; s++) { 
//      fprintf(stdout, "heap  %lu (NUMA %d): %llu\n", s, n, pSnapshot[n][s]);
      if (dummyHeap[n][s] != pSnapshot[n][s])
        fprintf(stdout, "content differs (at %lu)! (NUMA %d): %llu x %llu\n", 
            s, n, dummyHeap[n][s], pSnapshot[n][s]);

    }
  }
#endif

  return 0;
}
