
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <emmintrin.h>

#include "nvm.h"
#include "log-generate.h"


//#define DEBUG_LOG_BUILDER

//#define GENERATE_DUMP_FILE


// PARAMETERS TO CONTROL THE GENERATED LOG
#define MAX_INT64       99999999
#define WRITE_SET_MAX   10



typedef struct thrp {
  long int id;            
  long int index;
  size_t max_pos; 
  uint64_t *memp;
} thrp_t;



/*
 * Generates a distributed log. Logs are linked through a pointer.
 */
void generate_log_vanilla()
{
  thrp_t *mdata = (thrp_t *) malloc(sizeof(thrp_t)*g_workerThreads);

  const int *NUMApin = g_logNuma == 1 ? G_PINNING_1a : G_PINNING_2;  

#ifdef GENERATE_DUMP_FILE 
  FILE *f = fopen("dump-gen.txt", "w");
  if (f == NULL)
  {
    fprintf(stdout, "Error opening dump file!\n");
    exit(1);
  }

  uint64_t *dump[2];

  dump[0] = (uint64_t *)calloc(g_heapSize, 1);
  dump[1] = (uint64_t *)calloc(g_heapSize, 1);
  if (dump[0] == NULL || dump[1] == NULL) {
    fprintf(stdout, "Could not allocate memory for the dump memory (gen)\n");
    exit(1);
  }
#endif

  int length = g_logSize/sizeof(uint64_t)/g_workerThreads;
#ifdef DEBUG_LOG_BUILDER
  fprintf(stdout, "partition length = %d\n", length);
#endif
  for (int i=0; i<g_workerThreads; i++) {
    mdata[i].id = i;
    mdata[i].index = 0;
    mdata[i].max_pos = length-1;

    int numaSocket = G_NUMA_PINNING[NUMApin[i]];
//    fprintf(stdout, "Thread %d pinned to Core %d and NUMA node %d\n",
//                     i, NUMApin[i], numaSocket);
    mdata[i].memp = g_logRegion[numaSocket] + i*length;
    /* TODO (also for sharding)
     * When NUMA partition is used for the log structure, the area used is not
     * contiguous in the log (there are holes...) - this does not affect the
     * correctness of the code
     *
     */
#ifdef DEBUG_LOG_BUILDER
    fprintf(stdout, "mdata[%d] pointer = %p\n", i, mdata[i].memp);
    fprintf(stdout, " initial index    = %d\n", mdata[i].index);
    fprintf(stdout, " max index        = %d\n", mdata[i].max_pos);
#endif
  }
  
  int numLogs = 0;
  int numEntries = 0;


  //srand(time(0));
  srand(10);

  // first entry of the first thread has only the timestamp and a pointer 
  // to the next valid log entry
  uint64_t pos;
  uint64_t *next_entry;
  int next_tid, last_tid;

  *mdata[0].memp = 0xdeadbeef | (1UL<<63);  // timestamp
  next_entry = mdata[0].memp+1;             // position of the pointer to the next entry
  mdata[0].index += 2;

  int num_retries = 0;
  while(1) {

    // choose a thread number
    next_tid = rand() % g_workerThreads;
    // pick the number of entries to add
    int wr_set = (rand() % WRITE_SET_MAX)+1;
    // check if there is enough room to store the data in the choosen thread
    // log
    if ((mdata[next_tid].index+(wr_set*2)+2) > mdata[next_tid].max_pos) {
      // if not enough space, try another thread id
      num_retries++;
      if (num_retries == g_workerThreads) break; // all logs are full, quit
      continue;
    }
    num_retries = 0;
    last_tid = next_tid;
    
    // first position of the new log entry
    pos = mdata[next_tid].index;

    // points to the next log entry
    *next_entry = (uint64_t)&(mdata[next_tid].memp[pos]);
      
#ifdef DEBUG_LOG_BUILDER
    fprintf(stdout, "tid %d index %lu (max %lu)  -- At address: %p = %p\n", 
        next_tid, pos, mdata[next_tid].max_pos, next_entry, mdata[next_tid].memp + pos);
#endif

    // fill in the log
    for (int i=0; i<wr_set; i++) {
   
      uint64_t address = rand() % (g_heapSize>>3);

      /*
       * This will randomly generate access to the different NUMA nodes
       * (unless of course there is only one NUMA node)
       * Another possibility would be to generate access to the NUMA node a
       * specific thread/core is physically attached to.
       */
      int numaNode = rand() % g_heapNuma; // should be 0 or 1 

      address = (address<<3) + (uint64_t)g_heapRegion[numaNode];
      uint64_t value = rand() % MAX_INT64;

#ifdef DEBUG_LOG_BUILDER
      fprintf(stdout, "  tid %d  [%p] Adding value %llu at address %p\n", 
          next_tid, &(mdata[next_tid].memp[pos]), value, address);
#endif
        
      mdata[next_tid].memp[pos++] = address;   // 8-byte aligned addresses
      mdata[next_tid].memp[pos++] = value;

#ifdef GENERATE_DUMP_FILE 
      dump[numaNode][(address-((uint64_t)g_heapRegion[numaNode]))>>3] = value;
#endif

      numEntries++;
    }
    // timestamp
    mdata[next_tid].memp[pos++] = (uint64_t)rand() | (uint64_t)(1UL<<63); // make sure most significant bit is 1

    // address to store the address of the next log entry
    next_entry = mdata[next_tid].memp+pos;
    
    // update the index
    mdata[next_tid].index = pos+1;
//    mdata[next_tid].memp += pos+1;

    numLogs++;
  }
  // timestamp of the last entry is special (so the appliers can know when to
  // finish
#ifdef DEBUG_LOG_BUILDER
  fprintf(stdout, "Writing: %p at %p \n", LAST_TIMESTAMP, &mdata[last_tid].memp[pos-1]);
#endif
  mdata[last_tid].memp[pos-1] = LAST_TIMESTAMP;


  fprintf(stdout, "logs: %d, entries: %lu\n", numLogs, numEntries);

#ifndef USE_DRAM  
  for (int i=0; i<g_logNuma; i++) {
    flush_clwb_nolog((void *)g_logRegion[i], g_logSize);
  }
  _mm_sfence();
#endif

#ifdef DEBUG_LOG_BUILDER
  fprintf(stdout, "pointer to first entry: %p\n", *(g_logRegion[0]+1));
#endif

#ifdef GENERATE_DUMP_FILE 

  for (int i=0; i<g_heapNuma; i++) {

    fprintf(f, "NUMA [%d]:\n", i);
    for (uint64_t j=0; j<g_heapSize/sizeof(uint64_t); j++) {
      fprintf(f, "[%p] = %llu\n", &g_heapRegion[i][j], dump[i][j]);
    }

    free(dump[i]);
  }
  fclose(f);
#endif

  free(mdata);
}


// ******************************
// no fragmentation allowed
void generate_opt_log_shards()
{

#ifdef GENERATE_DUMP_FILE 
  FILE *f = fopen("dump-gen.txt", "w");
  if (f == NULL)
  {
    fprintf(stdout, "Error opening dump file!\n");
    exit(1);
  }

  uint64_t *dump[2];

  dump[0] = (uint64_t *)calloc(g_heapSize, 1);
  dump[1] = (uint64_t *)calloc(g_heapSize, 1);
  if (dump[0] == NULL || dump[1] == NULL) {
    fprintf(stdout, "Could not allocate memory for the dump memory (gen)\n");
    exit(1);
  }
#endif

  //int nshards = 32;
  int nshards = g_workerThreads;
  
  const int *NUMApin = g_logNuma == 1 ? G_PINNING_1 : G_PINNING_2;  
  
  thrp_t *mdata = (thrp_t *) malloc(sizeof(thrp_t)*g_workerThreads);

  int length = g_logSize/sizeof(uint64_t)/g_workerThreads;
  for (int i=0; i<g_workerThreads; i++) {
    mdata[i].id = i;
    mdata[i].index = 0;
    mdata[i].max_pos = length-1;


    int numaSocket = G_NUMA_PINNING[NUMApin[i]];
//    fprintf(stdout, "Thread %d pinned to Core %d and NUMA node %d\n",
//                     i, NUMApin[i], numaSocket);
    mdata[i].memp = g_logRegion[numaSocket] + i*length;
    //mdata[i].memp = g_logRegion[0] + i*length;

#ifdef DEBUG_LOG_BUILDER
    fprintf(stdout, "thread %d, start %p, end %p\n", i, mdata[i].memp, mdata[i].memp+(length-1));
#endif
  }

  int numLogs = 0;
  int numEntries = 0;

  srand(10);


  // first entry of the first thread has only the timestamp and a pointer 
  // to the next valid log entry
  uint64_t *logAddr;           
  uint64_t *lastTxPointer;

  *mdata[0].memp = 0xdeadbeef | (1UL<<63);  // timestamp
  lastTxPointer = mdata[0].memp+1;          // position of the pointer to the next entry
  mdata[0].index += 2;
  mdata[0].memp += 2;

  int num_retries = 0;
  while(1) {
  
    int shardOffset[64];     // wont work for more than 64 shards
    for (int i=0; i<nshards; i++)
      shardOffset[i] = 0;

    // choose a thread number
    int next_tid = rand() % g_workerThreads;

    int wr_set = nshards;

    // first position of the new log entry
    logAddr = mdata[next_tid].memp;

#ifdef DEBUG_LOG_BUILDER
    fprintf(stdout, "Starting - thread %d address %p\n", next_tid, logAddr);
#endif
    
    // check if there is enough room to initially store the minimum data
    // (initial field + 2 full shards)
    if ((mdata[next_tid].index+1+(2*nshards)) > mdata[next_tid].max_pos) {
      // if not enough space, try another thread id
      num_retries++;
      if (num_retries == g_workerThreads) break; // all logs are full, quit
      continue;
    }
    num_retries = 0;

    // fill in the log
    int entriesAdded = 0;
    for (int i=0; i<wr_set; i++) {

      uint64_t address = rand() % ((g_heapSize>>3)-i);
      
      address = (address & ~(nshards-1)) + i;
      //address = (address & ~(nshards-1));

      address = address << 3; // 8-byte aligned

      int numaNode = rand() % g_heapNuma; // should be 0 or 1 
      //address += (uint64_t)g_heapRegion[numaNode];
      // even shards acess same NUMA node
      const int *Npin = g_heapNuma == 1 ? G_PINNING_1 : G_PINNING_2;  
      int numaSocket = G_NUMA_PINNING[Npin[i]];
      address += (uint64_t)g_heapRegion[numaSocket];
//      if ((i%2) == 0) address += (uint64_t)g_heapRegion[0];
//      else address += (uint64_t)g_heapRegion[1];

      uint64_t value = (rand() % MAX_INT64);
 
      //int shardId = address % nshards;
      int shardId = i;
      int offsetToWrite = shardId + nshards*shardOffset[shardId];

      offsetToWrite *= 2;  // each entry occupies two positions

#if 0      
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
#endif

#ifdef DEBUG_LOG_BUILDER
      fprintf(stdout, "shard %d offset %d\n", shardId, offsetToWrite);
#endif

      // +1 because we are skipping the next pointer field
      logAddr[offsetToWrite+1] = address;
      logAddr[offsetToWrite+1+1] = value;

#ifdef GENERATE_DUMP_FILE 
      dump[numaNode][(address-((uint64_t)g_heapRegion[numaNode]))>>3] = value;
#endif

      shardOffset[shardId]++;

#ifdef DEBUG_LOG_BUILDER
      fprintf(stdout, "[%p] = (%p %llu)\n", logAddr+offsetToWrite+1, address, value);
#endif
      entriesAdded++;
      numEntries++;
    }

    if (num_retries == g_workerThreads) break; // all logs are full, quit
    
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
      
#ifdef DEBUG_LOG_BUILDER
      fprintf(stdout, "[%p] = Timestamp %lu\n", logAddr+offsetToWrite+1, timestamp);
#endif      
    }

    // update the index
    mdata[next_tid].index += max_offset+1;
    mdata[next_tid].memp += max_offset+1;
      

    if (mdata[next_tid].index-1 > mdata[next_tid].max_pos) {
      fprintf(stdout, "oops.. should not happen... index %lu, maxpos %lu \n", 
          mdata[next_tid].index, mdata[next_tid].max_pos);
    }
      

    numLogs++;
#ifdef DEBUG_LOG_BUILDER
    fprintf(stdout, "Finished log. Next index: %d (%p) - Entries: %d\n", 
        mdata[next_tid].index, mdata[next_tid].memp, entriesAdded);
#endif
  }

  // timestamp of the last entry is special (so the appliers can know when to
  // finish)
  *lastTxPointer = LAST_TIMESTAMP;

#ifdef DEBUG_LOG_BUILDER
  fprintf(stdout, "Finishing: %p with %p\n", lastTxPointer, LAST_TIMESTAMP);
#endif

#ifndef USE_DRAM  
///  flush_clwb_nolog((void *)pmem_pointer, g_logSize);
  _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif


  fprintf(stdout, "logs: %d, entries: %lu\n", numLogs, numEntries);

#ifdef GENERATE_DUMP_FILE 

  for (int i=0; i<g_heapNuma; i++) {
    fprintf(f, "NUMA [%d]:\n", i);
    for (uint64_t j=0; j<g_heapSize/sizeof(uint64_t); j++) {
      fprintf(f, "[%p] = %llu\n", &g_heapRegion[i][j], dump[i][j]);
    }

    free(dump[i]);
  }
  fclose(f);
    
#endif

  free(mdata);
}

