
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <emmintrin.h>
#include <threads.h>
#include <sys/wait.h> 
#include <getopt.h>

#include "nvm.h"
#include "log-generate.h"
#include "log-replay.h"


#ifndef PM_DIR
#define PM_DIR "/mnt/nvram"
#endif /* PM_DIR */


#define LAST_TIMESTAMP (0xDEADDEADBEEF | (uint64_t)(1UL<<63)) 


//#define REPLAY_DUMP_FILE




const char* NVRAM_REGIONS[] = {PM_DIR "0/", PM_DIR "1/"};

typedef struct thr_parms {
  long int id;            
  uint64_t *start_addr;
  uint64_t *dest_addr;
#ifdef USE_NUMA
  uint64_t *dest_addr2;
#endif
  size_t length;          // in words
  int total_threads;
  int nshards;            // number of shards in the log
  int granularity;        // used if applying sharded logs
} thr_parms_t;



#if 0
int applier2(void *data)
{
  thr_parms_t *mydata = (thr_parms_t *)data;
  
  int core = G_PINNING_1[mydata->id];

  //threading_pinThisThread(core);
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(core, &cpu_set);
  sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);

  /*
  fprintf(stdout, "thread id %d\n", mydata->id);
  fprintf(stdout, "start addr: %p\n", mydata->start_addr);
  fprintf(stdout, "dest  addr: %p\n", mydata->dest_addr);
  fprintf(stdout, "length: %d\n", mydata->length);
  */
  
	uint64_t entries_applied = 0;
	uintptr_t entries = 0;
  while (entries < mydata->length) { 
  
    uint64_t addr = mydata->start_addr[entries++];

    if (addr >> 63) {
      entries++;  // skip the pointer to the next
      continue;  // timestamp
    }
    else {
      if ((addr >> 3)%mydata->total_threads == mydata->id) {
#ifdef USE_NT_STORE
        _mm_stream_si64 ((long long int*)&mydata->dest_addr[addr>>3], mydata->start_addr[entries++]);
#else
        //*addr = mydata->start_addr[entries++];
        mydata->dest_addr[addr>>3] = mydata->start_addr[entries++];
#ifndef USE_DRAM
#ifndef USE_WBINVD
#ifndef BATCH_FLUSH
    		pmem_clwb((char *)&(mydata->dest_addr[addr>>3]));
        _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif
#endif 
#endif
#endif
        entries_applied++;
      } else
        entries++;
    }

  }



  return entries_applied;
}


#endif

//#define DEBUG_LOG_REPLAY

static void *createEmptyNVRAM(const char *memRegion, const char *file, size_t bytes, long mapFlag)
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

static void *openNVRAM(const char *memRegion, const char *file, size_t bytes, long mapFlag)
{
  char fileNameBuffer[1024];
  void *res = NULL;
  int fd;

  sprintf(fileNameBuffer, "%s%s", memRegion, file);
  fd = open(fileNameBuffer, O_RDWR, 0666);

  if (fd == -1) {
    fprintf(stderr, "Error open file %s: %s\n", fileNameBuffer, strerror(errno));
    return 0;
  }
 /* 
  if (ftruncate(fd, bytes)) {
    fprintf(stderr, "Error ftruncate file %s: %s\n", fileNameBuffer, strerror(errno));
    return 0;
  }
  */

  res = mmap(NULL, bytes, PROT_READ | PROT_WRITE, mapFlag, fd, 0);
  if (res == (void*)-1 || res == NULL) {
    fprintf(stderr, "Error mmapping file %s: %s\n", fileNameBuffer, strerror(errno));
    return 0;
  }
  
  return res;
}


uint64_t *pLog = 0;
uint64_t *pSnapshot = 0;
#ifdef USE_NUMA
uint64_t *pLog2 = 0;
uint64_t *pSnapshot2 = 0;
#endif

#if 0
/*
 * Generates a centralized log (contiguous memory, no links)
 */
void generate_log_centralized(uint64_t *pmem_pointer)
{
  uint64_t * p = pmem_pointer;

  //srand(time(0));
  srand(10);

  uint64_t position = 0;
  uint32_t max_pos = g_logSize/sizeof(uint64_t);
  while ( position < (max_pos-3)/* least is a key-value pair + timestamp */) {
  
    int wr_set = (rand() % WRITE_SET_MAX)+1;
    if (position+(wr_set*2)+1 >= max_pos) break;
    for (int i=0; i<wr_set; i++) {
    
      uint64_t address = rand() % TARGET_MAX_ADDR;
      uint64_t value = rand() % MAX_INT64;

      p[position++] = address;
      p[position++] = value;
    }
    // timestamp
    p[position++] = (uint64_t)rand() | (uint64_t)(1LU<<63); // make sure most significant bit is 1

  }

#ifndef USE_DRAM  
  flush_clwb_nolog((void *)pmem_pointer, g_logSize);
  _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif

}
#endif




#if 0

/*
 * Generates a distributed log with shards (sublogs) so that it can be
 * replayed in parallel. The idea is that each shard holds non-conflicting
 * addresses.
 *
 * Log structure:
 *
 * First field: pointer to next log
 *
 * E-th entry of shard S will be inserted into position S+|S|*(E-1)
 *
 * Example:
 *
 *  TS | nextP | S0,E-0 S1,E-0, S2,E-0, ... SN-1,E-0 | S0,E-1 S1,E-1, ... | SN-1,E-1 | ...
 *
 * Each entry is composed of an address and value fields. 
 * Last entry in a shard has address field zeroed out.
 *
 *
 * Basic algorithm:
 *
 * LastTxPointer -> address that holds the pointer to the next transaction of
 *                  the previously generated log
 * LogAddr -> address of the log generated by current transaction
 * The offset of each shard (where to write next) is kept in a vector Shard[]
 * indexed by the shard number
 *
 * S: number of shards
 * N: number of threads
 *
 * 1. Pick a thread T to generate the log of a transaction
 *
 * 2. LogAddr = address of the log for transaction T
 *
 * 3. Pick a number of entries to be added
 *
 * 4. For each (addr,val) to be added into the log
 *   4.1 ShardId = addr%S
 *   4.2 OffsetToWrite = ShardId + S*Shard[ShardId]
 *   4.3 Check if there is enough room to write
 *   4.4 If no space available, 
 *   4.5 Write the entry in the log
 * 
 * 5. Write timestamp in first position
 *
 * 6. Write LogAddr into the preceding tx log (LastTxPointer)
 *
 *
 *
 */
//#define DEBUG_LOG_BUILDER
void generate_log_shards(uint64_t *pmem_pointer, int nthreads, int nshards)
{
  uint64_t * p = pmem_pointer;

  thrp_t *mdata = (thrp_t *) malloc(sizeof(thrp_t)*nthreads);

  int length = BSIZE/sizeof(uint64_t)/nthreads;
  for (int i=0; i<nthreads; i++) {
    mdata[i].id = i;
    mdata[i].index = i*length;
    mdata[i].max_pos = i*length+(length-1);

#ifdef DEBUG_LOG_BUILDER
    fprintf(stdout, "thread %d, start %p, end %p\n", i, &(p[i*length]), &(p[i*length+(length-1)]));
#endif
  }

  int numLogs = 0;
  int numEntries = 0;

  srand(10);


  // first entry of the first thread has only the timestamp and a pointer 
  // to the next valid log entry
  uint64_t *logAddr;           
  uint64_t *lastTxPointer;

  *p = 0xdeadbeef | (1UL<<63);    // timestamp
  lastTxPointer = p+1;             // position of the pointer to the next entry
  mdata[0].index += 2;

  int num_retries = 0;
  while(1) {
  
    int shardOffset[32];     // wont work for more than 32 shards
    for (int i=0; i<nshards; i++)
      shardOffset[i] = 0;

    // choose a thread number
    int next_tid = rand() % nthreads;

    // pick the number of entries to add
    int wr_set = (rand() % WRITE_SET_MAX)+1;


    // first position of the new log entry
    logAddr = p + mdata[next_tid].index;


#ifdef DEBUG_LOG_BUILDER
    fprintf(stdout, "Starting - thread %d address %p\n", next_tid, logAddr);
#endif
    
    // check if there is enough room to initially store the minimum data
    // (initial field + 2 full shards)
    if ((mdata[next_tid].index+1+(2*nshards)) > mdata[next_tid].max_pos) {
      // if not enough space, try another thread id
      num_retries++;
      if (num_retries == nthreads) break; // all logs are full, quit
      continue;
    }
    num_retries = 0;


    // fill in the log
    int entriesAdded = 0;
    for (int i=0; i<wr_set; i++) {
    
      uint64_t address = (rand() % TARGET_MAX_ADDR)+1;
      uint64_t value = (rand() % MAX_INT64)+1;
  
      int shardId = address % nshards;
      int offsetToWrite = shardId + nshards*shardOffset[shardId];

      offsetToWrite *= 2;  // each entry occupies two positions

      // check if there is enough room
      // we need to consider an extra position because of the timestamp
      if (mdata[next_tid].index+offsetToWrite+1+(2*nshards) >= mdata[next_tid].max_pos) {
        // if there is at least one entry already added, skip the loop and
        // finish
        if (entriesAdded) break;

        // otherwise signal it to retry
        num_retries++;
        break;
      } else num_retries = 0;

#ifdef DEBUG_LOG_BUILDER
      fprintf(stdout, "shard %d offset %d\n", shardId, offsetToWrite);
#endif

      // +1 because we are skipping the next pointer field
      logAddr[offsetToWrite+1] = address;
      logAddr[offsetToWrite+1+1] = value;

      shardOffset[shardId]++;

#ifdef DEBUG_LOG_BUILDER
      fprintf(stdout, "[%p] = (%lu %lu)\n", logAddr+offsetToWrite+1, address, value);
#endif
      entriesAdded++;
      numEntries++;
    }

    if (num_retries == nthreads) break; // all logs are full, quit
    
    // not possible to add new itens in the log, try another one
    if (num_retries) continue;


#ifdef DEBUG_LOG_BUILDER
    fprintf(stdout, "Setting prec pointer %p to %p\n", lastTxPointer, logAddr);
#endif
    // link the previous log to us
    *lastTxPointer = (uint64_t)logAddr;

    // adjust the pointer to our next field address
    lastTxPointer = logAddr;

    // add timestamp at the end of each shard
    uint64_t timestamp = (uint64_t)rand() | (uint64_t)(1UL<<63); // make sure most significant bit is 1


    int max_offset = -1;
    for (int i=0; i<nshards; i++) {

      //int offsetToWrite = i + nshards*2*shardOffset[i];
      
      int offsetToWrite = i + nshards*shardOffset[i];

      offsetToWrite *= 2;  // each entry occupies two positions
    
      if (offsetToWrite > mdata[next_tid].max_pos) {
        fprintf(stdout, "oops.. not enough space for timestamp ...\n");
        exit(1);
      }

      // remember to skip the next pointer entry
      logAddr[offsetToWrite+1] = timestamp;
      
      if (offsetToWrite+1 > max_offset)
        max_offset = offsetToWrite+1;
      //shardOffset[i]++;
      
#ifdef DEBUG_LOG_BUILDER
      fprintf(stdout, "[%p] = Timestamp %lu\n", logAddr+offsetToWrite+1, timestamp);
#endif      
    }

    // update the index
    mdata[next_tid].index += max_offset+1;
      

    if (mdata[next_tid].index-1 > mdata[next_tid].max_pos) {
      fprintf(stdout, "oops.. should not happen... index %lu, maxpos %lu \n", 
          mdata[next_tid].index, mdata[next_tid].max_pos);
//      exit(1);
    }
      

    numLogs++;
#ifdef DEBUG_LOG_BUILDER
    fprintf(stdout, "Finished log. Next index: %d (%p) - Entries: %d\n", 
        mdata[next_tid].index, p + mdata[next_tid].index, entriesAdded);
#endif
  }

  // timestamp of the last entry is special (so the appliers can know when to
  // finish)
  *lastTxPointer = LAST_TIMESTAMP;

#ifdef DEBUG_LOG_BUILDER
  fprintf(stdout, "Finishing: %p with %p\n", lastTxPointer, LAST_TIMESTAMP);
#endif

#ifndef USE_DRAM  
  flush_clwb_nolog((void *)pmem_pointer, g_logSize);
  _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif


//  fprintf(stdout, "write-set mean %d\n", writes/2/num_logs);
  fprintf(stdout, "logs: %d, entries: %lu\n", numLogs, numEntries);

  free(mdata);
}

#endif





static struct option long_options[] =
  {
    {"workers",     required_argument, 0, 'w'},
    {"logtype",     required_argument, 0, 'l'},
    {"numa",        required_argument, 0, 'n'},
    {0, 0, 0, 0}
  };

#define PARAM_DEFAULT_WORKERS   32
#define PARAM_DEFAULT_REPLAYERS 32
/*
 * 0 - vanilla
 * 1 - shards
 */
#define PARAM_DEFAULT_LTYPE   0
#define PARAM_DEFAULT_NUMA_LOGS    1
#define PARAM_DEFAULT_NUMA_HEAP    1
#define PARAM_DEFAULT_LOG_SIZE    (1024*1024*1024)   // 1 GB
#define PARAM_DEFAULT_HEAP_SIZE   (1024*1024*1024)   // 1 GB

static char *logVanillaNames[] = {"log-dist", "log_dist"};
static char *logShardNames[] = {"log-shard", "log_shard"};
static char *heapNames[] = {"snapshot-test", "snapshot-test"};

int g_workerThreads = PARAM_DEFAULT_WORKERS;
int g_replayerThreads = PARAM_DEFAULT_REPLAYERS;
int g_logType = PARAM_DEFAULT_LTYPE;
/*
 * When more than one NUMA node is specified for the logs, it means that the
 * log will be stored in the NUMA node of that core according to an
 * interleaved distribution (thread 0 -> core 0 -> NUMA 0, thread 1 -> core 16
 * -> NUMA 1, and so on according to G_PINNING_2[])
 */
int g_logNuma = PARAM_DEFAULT_NUMA_LOGS;
uint64_t *g_logRegion[2];    // pointers to the beginning of each NUMA region (logs)
int g_heapNuma = PARAM_DEFAULT_NUMA_HEAP;
uint64_t *g_heapRegion[2];    // pointers to the beginning of each NUMA region (heap)

uint64_t g_logSize = PARAM_DEFAULT_LOG_SIZE;
uint64_t g_heapSize = PARAM_DEFAULT_HEAP_SIZE;


static void
displayUsage (const char* appName)
{
    printf("Usage: %s [options]\n", appName);
    printf("\nOptions:                                     (defaults)\n");
    printf("  w <INT>   number of [w]orker threads        (%li)\n", PARAM_DEFAULT_WORKERS);
    printf("  r <INT>   number of [r]eplayer threads      (%li)\n", PARAM_DEFAULT_REPLAYERS);
    printf("  l <INT>   [l]og type (0-vanilla, 1-shards)  (%li)\n", PARAM_DEFAULT_LTYPE);
    printf("  n <INT>   number of [n]uma nodes (logs)     (%li)\n", PARAM_DEFAULT_NUMA_LOGS);
    printf("  h <INT>   number of numa nodes (heap)     (%li)\n", PARAM_DEFAULT_NUMA_HEAP);
    printf("  s <INT>   log [s]ize (per NUMA)             (%li)\n", PARAM_DEFAULT_LOG_SIZE);
    printf("  S <INT>   heap [S]ize (per NUMA)            (%li)\n", PARAM_DEFAULT_HEAP_SIZE);
    printf("\n");
    exit(1);
}

static void ParseCommandLine(int argc, char* argv[])
{

  /* getopt_long stores the option index here. */
  int option_index = 0;
  int c = 0;
  opterr = 0;

  while ((c = getopt_long(argc, argv, "w:r:l:n:h:s:S:",
                   long_options, &option_index)) != -1) {

    switch(c) {

      case 'w':
        g_workerThreads = atoi(optarg);

        if (g_workerThreads < 1 || g_workerThreads > 64) {
          fprintf(stdout, "Invalid number of workers: %d\n", g_workerThreads);
          exit(1);
        }
        break;
      
      case 'r':
        g_replayerThreads = atoi(optarg);

        if (g_replayerThreads < 1 || g_replayerThreads > 64) {
          fprintf(stdout, "Invalid number of replayers: %d\n", g_replayerThreads);
          exit(1);
        }
        break;

      case 'l':
        g_logType = atoi(optarg);

        if (g_logType < 0 || g_logType > 1) {
          fprintf(stdout, "Invalid log type: %d\n", g_logType);
          exit(1);
        }
        break;

      case 'n':
		    g_logNuma = atoi(optarg);
        if (g_logNuma < 1 || g_logNuma > 2) {
          fprintf(stdout, "Invalid number of numa nodes (log): %d\n", g_logNuma);
          exit(1);
      	}
        break;
      
      case 'h':
		    g_heapNuma = atoi(optarg);
        if (g_heapNuma < 1 || g_heapNuma > 2) {
          fprintf(stdout, "Invalid number of numa nodes (heap): %d\n", g_heapNuma);
          exit(1);
      	}
        break;

      case 's':
        g_logSize = atoi(optarg);
        break;

      case 'S':
        g_heapSize = atoi(optarg);
        break;
      
      default:
        opterr++;
        break;
    }

  }

  for (int i = optind; i < argc; i++) {
    fprintf(stderr, "Non-option argument: %s\n", argv[i]);
    opterr++;
  }

  if (opterr) {
    displayUsage(argv[0]);
  }
  

  fprintf(stdout, "Parameters in use:\n");
  fprintf(stdout, "Number of worker threads: %d\n", g_workerThreads);
  fprintf(stdout, "Number of replayer threads: %d\n", g_replayerThreads);
  fprintf(stdout, "Log type: %d (%s)\n", g_logType, g_logType==0?"vanilla":"sharded");
  fprintf(stdout, "NUMA nodes:");
  fprintf(stdout, "  logs: %d - heap: %d\n", g_logNuma, g_heapNuma);
  fprintf(stdout, "Log size: %lu\n", g_logSize);
  fprintf(stdout, "Heap size: %lu\n", g_heapSize);

//  if (logtype == 1) fprintf(stdout, "Num shards: %d\n", g_numShards);
//  fprintf(stdout, "Threads: %d\n", g_numThreads);
  
}

static void AllocateMemory()
{
#ifdef USE_DRAM

  for (int i=0; i<g_logNuma; i++) {
    g_logRegion[i] = (uint64_t *)calloc(g_logSize,1);
    if (g_logRegion[i] == NULL) {
      printf("Error allocating memory for the logs\n");
      exit(1);
    }
  }
  
  for (int i=0; i<g_heapNuma; i++) {
    g_heapRegion[i] = (uint64_t *)calloc(g_heapSize,1);
    if (g_heapRegion[i] == NULL) {
      printf("Error allocating memory for the heap\n");
      exit(1);
    }
  }

#else
  
  for (int i=0; i<g_logNuma; i++) {
    g_logRegion[i] = (uint64_t *)createEmptyNVRAM(NVRAM_REGIONS[i], logVanillaNames[i], g_logSize, MAP_SHARED);
    if (g_logRegion[i] == NULL) {
      printf("Error allocating memory for the logs\n");
      exit(1);
    }
  }
  
  for (int i=0; i<g_heapNuma; i++) {
    g_heapRegion[i] = (uint64_t *)createEmptyNVRAM(NVRAM_REGIONS[i], heapNames[i], g_heapSize, MAP_SHARED);
    if (g_heapRegion[i] == NULL) {
      printf("Error allocating memory for the heap\n");
      exit(1);
    }
  }

#endif

}


int main(int argc, char *argv[])
{
  struct timeval start, end;
  double elapsed_time;
  thr_parms_t *threadData;
  thrd_t *threadId;
  int num_shards = 1; // if sharded, 3rd options is the number of shards
	
  ParseCommandLine(argc, (char * *) argv);

  AllocateMemory();



  threadData = (thr_parms_t *)malloc(sizeof(thr_parms_t)*g_replayerThreads);
  threadId = (thrd_t *)malloc(sizeof(thrd_t)*g_replayerThreads);
  // TODO: check for correct allocation


  fprintf(stdout, "Generating log ...\n");

  if (g_logType == 0)
    generate_log_vanilla();
  else
    generate_opt_log_shards();

//  exit(-1);

  fprintf(stdout, "Replaying ...\n");

#ifdef REPLAY_DUMP_FILE 
  FILE *f = fopen("dump-replay.txt", "w");
  if (f == NULL)
  {
    fprintf(stdout, "Error opening dump file (replay)!\n");
    exit(1);
  }
#endif

  uint64_t entries_written[64]; 

  int (*replay_ptr)(void *) = g_logType == 0 ? replay_vanilla : replay_shards;  

  gettimeofday(&start, NULL);
  
  for (int i=0; i < g_replayerThreads; ++i) {
    if (thrd_create(threadId+i, replay_ptr, (void *)(uint64_t)i) != thrd_success) {
      printf("%d-th thread create error\n", i);
      return 0;
    }
  }

    
  uint64_t total_entries_applied = 0;
  for (int i=0; i < g_replayerThreads; ++i) {
    int entries_applied;
    thrd_join(threadId[i], &entries_applied);
    entries_written[i] = entries_applied;
    total_entries_applied += entries_applied;
  }


#ifdef BATCH_FLUSH
  for (int i=0; i<g_heapNuma; i++) {
    flush_clwb_nolog((void *)g_heapRegion[i], g_heapSize);
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

  for (int i=0; i < g_replayerThreads; ++i)
    fprintf(stdout, "Entries applied by thread %d =  %llu\n", 
        i, entries_written[i]);

#ifdef REPLAY_DUMP_FILE 

  for (int i=0; i<g_heapNuma; i++) {

    fprintf(f, "NUMA [%d]:\n", i);
    for (uint64_t j=0; j<g_heapSize/sizeof(uint64_t); j++) {
      fprintf(f, "[%p] = %llu\n", &g_heapRegion[i][j], g_heapRegion[i][j]);
    }

  }
  fclose(f);
#endif

  return 0;
}
