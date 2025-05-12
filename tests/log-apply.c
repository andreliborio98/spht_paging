
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
#include <sched.h>
#include <threads.h>
#include <sys/wait.h> 


#define USE_NUMA

#define BSIZE    (1024*1024*1024)   // 1 GB
//#define BSIZE    (1024*1024*8)   // 1 MB
#define LOGNAME  "log-test"
#define LOGNAMEDIST   "log-dist"
#define LOGNAMESHARD  "log-shard"
#define SNAPNAME "snapshot-test"

#ifndef PM_DIR
#define PM_DIR "/mnt/nvram"
// #define PM_DIR "./"
#endif /* PM_DIR */


// PARAMETERS TO CONTROL THE GENERATED LOG
#define MAX_INT64       99999999
#define WRITE_SET_MAX   10
// #define TARGET_MAX_ADDR (1024*1024*16)   // 128MB (each entry is 8 bytes)
// #define TARGET_MAX_ADDR (1024*8)   // 128MB (each entry is 8 bytes)
#define TARGET_MAX_ADDR (1024)   // 128MB (each entry is 8 bytes)


#define LAST_TIMESTAMP (0xDEADDEADBEEF | (uint64_t)(1UL<<63)) 


/* LIBPEM */
#define FLUSH_ALIGN ((uintptr_t)64)

#define pmem_clflushopt(addr)\
	asm volatile(".byte 0x66; clflush %0" : "+m" \
		(*(volatile char *)(addr)));
#define pmem_clwb(addr)\
	asm volatile(".byte 0x66; xsaveopt %0" : "+m" \
		(*(volatile char *)(addr)));

static inline void
flush_clflush_nolog(const void *addr, size_t len)
{
	uintptr_t uptr;

	/*
	 * Loop through cache-line-size (typically 64B) aligned chunks
	 * covering the given range.
	 */
	for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
		uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN)
		_mm_clflush((char *)uptr);
}

/*
 * flush_clflushopt_nolog -- flush the CPU cache, using clflushopt
 */
static inline void
flush_clflushopt_nolog(const void *addr, size_t len)
{
	uintptr_t uptr;

	/*
	 * Loop through cache-line-size (typically 64B) aligned chunks
	 * covering the given range.
	 */
	for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
		uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN) {
		pmem_clflushopt((char *)uptr);
	}
}

/*
 * flush_clwb_nolog -- flush the CPU cache, using clwb
 */
static inline void
flush_clwb_nolog(const void *addr, size_t len)
{
	uintptr_t uptr;

	/*
	 * Loop through cache-line-size (typically 64B) aligned chunks
	 * covering the given range.
	 */
	for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
		uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN) {
		pmem_clwb((char *)uptr);
	}
}




/* LIBPMEM */



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

const int G_PINNING_1[] = { // full socket first
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
};

const int G_PINNING_2[] = { // interchanging nodes)
  0, 32,  1, 33,  2, 34,  3, 35,  4, 36,  5, 37,  6, 38,  7, 39,
  8, 40,  9, 41, 10, 42, 11, 43, 12, 44, 13, 45, 14, 46, 15, 47,
  16, 48, 17, 49, 18, 50, 19, 51, 20, 52, 21, 53, 22, 54, 23, 55,
  24, 56, 25, 57, 26, 58, 27, 59, 28, 60, 29, 61, 30, 62, 31, 63,
};



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


int applier_dist(void *data)
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
  uint64_t *next_entry = mydata->start_addr;
  while (1) { 
 
    uint64_t addr = *next_entry;

    if (addr >> 63) { // timestamp
      
///      fprintf(stdout, "At address: %p = %p\n", next_entry, addr);
      
      if (addr == LAST_TIMESTAMP) break; // the end
      

///      fprintf(stdout, "Next address: %p = %p\n", next_entry+1, *(next_entry+1));

      // next field is the pointer
      next_entry++;
      next_entry = (uint64_t *)*next_entry;

      continue;  
    }
    else {
      next_entry++;  // points to data
      addr = addr >> 3; // assuming 8-byte (aligned) addressed data
      //if (addr%mydata->total_threads == mydata->id) {
      if ((addr>>9)%mydata->total_threads == mydata->id) {    // 4k pages

///        fprintf(stdout, "  [%p] Applying %llu at address %p\n", next_entry, *next_entry, addr);

#ifdef USE_NT_STORE
        _mm_stream_si64 ((long long int*)&mydata->dest_addr[addr], *next_entry);
#else
        //*addr = mydata->start_addr[entries++];
        mydata->dest_addr[addr] = *next_entry;
#ifndef USE_DRAM
#ifndef BATCH_FLUSH
#ifndef USE_WBINVD
    		pmem_clwb((char *)&(mydata->dest_addr[addr]));
        _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif
#endif
#endif
#endif
        entries_applied++;
      }
    }
    next_entry++;

  }



  return entries_applied;
}


//#define DEBUG_LOG_REPLAY
int applier_shards(void *data)
{
  thr_parms_t *mydata = (thr_parms_t *)data;

  int gran = mydata->granularity;
  int ns   = mydata->nshards;
  int id = mydata->id;

#ifdef USE_NUMA
  int core = G_PINNING_2[mydata->id];
#else
  int core = G_PINNING_1[mydata->id];
#endif

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
  uint64_t *next_entry = (uint64_t *)*(mydata->start_addr+1);   // pointer to the first log entry

#ifdef DEBUG_LOG_REPLAY
  printf("%d %p\n", id, next_entry);
  printf("%d %p\n", id, *next_entry);
  printf("%d %lu\n", id, *(next_entry+1));
  printf("%d %lu\n", id, *(next_entry+2));
#endif

  //int its_done = 0;
  while (1) { 


///    for (int i=id*gran; i<(id*gran)+gran; i++) {
    for (int i=0; i<gran; i++) {

      int idx = (i*mydata->total_threads)+id;

///      int offset = i*2+1;
      int offset = idx*2+1;

      // go through the elements in the shard
      uint64_t addr = next_entry[offset];

#ifdef DEBUG_LOG_REPLAY
      printf("%d -> %lu %lu\n", id, offset, addr);
#endif

/*      
#ifdef USE_NUMA
      uint64_t *target_addr = G_PINNING_2[idx] > 31 ? mydata->dest_addr2 : mydata->dest_addr;
      //uint64_t *target_addr = core > 31 ? mydata->dest_addr : mydata->dest_addr2;
#else
      uint64_t *target_addr = mydata->dest_addr;
#endif
*/

      while (!(addr>>63)) { // while there entries to be applied ...



#ifdef DEBUG_LOG_REPLAY
        fprintf(stdout, "%d Applying %lu at %p (%p)\n", id, next_entry[offset+1],
            addr, &(next_entry[offset+1]));
#endif

#ifdef USE_NT_STORE
        _mm_stream_si64 ((long long int*)addr, next_entry[offset+1]);
        //_mm_stream_si64 ((long long int*)&target_addr[addr], next_entry[offset+1]);
#else
        *(uint64_t *)addr = next_entry[offset+1];
        //target_addr[addr] = next_entry[offset+1];
#ifndef USE_DRAM
#ifndef BATCH_FLUSH
#ifndef USE_WBINVD
        pmem_clwb((char *)addr);
        //pmem_clwb((char *)&(target_addr[addr]));
  //      _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif
#endif
#endif
#endif
        entries_applied++;

        offset += ns*2;

        addr = next_entry[offset];

      }
#ifdef DEBUG_LOG_REPLAY
      fprintf(stdout, "%d End of shard -- addr: %lu \n", id, addr);
#endif

      /*
      if (addr == LAST_TIMESTAMP) {
        its_done = 1;
        break; // the end
      }
      */
      

    } // for

    // go to next tx log
    next_entry = (uint64_t *)*next_entry;
    
#ifdef DEBUG_LOG_REPLAY
    fprintf(stdout, "%d End of txlog -- next: %p \n", id, next_entry);
#endif

    if (next_entry == (uint64_t *)LAST_TIMESTAMP) break;
  }

#ifdef USE_WBINVD
  int pid, status;
  if ((pid = fork()) == 0) {
    // child
    int r = execl("/usr/bin/cat", "cat","/proc/wbinvd",NULL);
    if (r) perror("oh no\n");
  }
  else (waitpid(pid, &status, 0)); 
#endif


  return entries_applied;
}


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

/*
 * Generates a centralized log (contiguous memory, no links)
 */
void generate_log_centralized(uint64_t *pmem_pointer)
{
  uint64_t * p = pmem_pointer;

  //srand(time(0));
  srand(10);

  uint64_t position = 0;
  uint32_t max_pos = BSIZE/sizeof(uint64_t);
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
  flush_clwb_nolog((void *)pmem_pointer, BSIZE);
  _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif

}



typedef struct thrp {
  long int id;            
  long int index;
  size_t max_pos; 
  uint64_t *memp;
} thrp_t;


/*
 * Generates a distributed log (assuming 'nthreads' threads). Logs are linked
 * throught a pointer.
 */
void generate_log_distributed(uint64_t *pmem_pointer, int nthreads)
{
  uint64_t * p = pmem_pointer;

  thrp_t *mdata = (thrp_t *) malloc(sizeof(thrp_t)*nthreads);


  int length = BSIZE/sizeof(uint64_t)/nthreads;
  for (int i=0; i<nthreads; i++) {
    mdata[i].id = i;
    mdata[i].index = i*length;
    mdata[i].max_pos = i*length+(length-1);
  }
  
  int numLogs = 0;
  int numEntries = 0;


  //srand(time(0));
  srand(10);

  // first entry of the first thread has only the timestamp and a pointer 
  // to the next valid log entry
  uint64_t pos;
  uint64_t *next_entry;

  *p = 0xdeadbeef | (1UL<<63);  // timestamp
  next_entry = p+1;             // position of the pointer to the next entry
  mdata[0].index += 2;

  int num_retries = 0;
  while(1) {

    // choose a thread number
    int next_tid = rand() % nthreads;
    // pick the number of entries to add
    int wr_set = (rand() % WRITE_SET_MAX)+1;
    // check if there is enough room to store the data in the choosen thread
    // log
    if ((mdata[next_tid].index+(wr_set*2)+2) > mdata[next_tid].max_pos) {
      // if not enough space, try another thread id
      num_retries++;
      if (num_retries == nthreads) break; // all logs are full, quit
      continue;
    }
    num_retries = 0;
    
    // first position of the new log entry
    pos = mdata[next_tid].index;

    // points to the next log entry
    *next_entry = (uint64_t)(p + pos);
      
///    fprintf(stdout, "tid %d index %lu  -- At address: %p = %p\n", next_tid, pos, next_entry, p + pos);


    // fill in the log
    for (int i=0; i<wr_set; i++) {
    
      uint64_t address = (rand() % TARGET_MAX_ADDR)+1;
      uint64_t value = (rand() % MAX_INT64)+1;
        
///      fprintf(stdout, "  [%p] Adding value %llu at address %p\n", &p[pos], value, address);

      p[pos++] = address<<3;   // 8-byte aligned addresses
      p[pos++] = value;

      numEntries++;
    }
    // timestamp
    p[pos++] = (uint64_t)rand() | (uint64_t)(1UL<<63); // make sure most significant bit is 1

    // address to store the address of the next log entry
    next_entry = p+pos;
    
    // update the index
    mdata[next_tid].index = pos+1;

    numLogs++;
  }
  // timestamp of the last entry is special (so the appliers can know when to
  // finish
  p[pos-1] = LAST_TIMESTAMP;


  fprintf(stdout, "logs: %d, entries: %lu\n", numLogs, numEntries);

#ifndef USE_DRAM  
  flush_clwb_nolog((void *)pmem_pointer, BSIZE);
  _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif

  free(mdata);
}


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
  flush_clwb_nolog((void *)pmem_pointer, BSIZE);
  _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif


//  fprintf(stdout, "write-set mean %d\n", writes/2/num_logs);
  fprintf(stdout, "logs: %d, entries: %lu\n", numLogs, numEntries);

  free(mdata);
}




// ******************************
// no fragmentation allowed
// always add 1 element in a 32 shard
//#define DEBUG_LOG_BUILDER

#ifdef USE_NUMA
void generate_opt_log_shards(uint64_t *pmem_pointer, uint64_t *p2, uint64_t *target_NUMA1, uint64_t *target_NUMA2, int force_singleNUMA, 
    int nshards, int nthreads)
#else
void generate_opt_log_shards(uint64_t *pmem_pointer, uint64_t *target_pointer, int nthreads)
#endif
{
  uint64_t * p = pmem_pointer;
  
  //int nshards = 32;
//  int nshards = nthreads;
  
  thrp_t *mdata = (thrp_t *) malloc(sizeof(thrp_t)*nthreads);

#ifdef USE_NUMA
  if (force_singleNUMA){
    int length = BSIZE/sizeof(uint64_t)/nthreads;
    for (int i=0; i<nthreads; i++) {
      mdata[i].id = i;
      mdata[i].index = i*length;
      mdata[i].max_pos = i*length+(length-1);

      mdata[i].memp = p + mdata[i].index;

#ifdef DEBUG_LOG_BUILDER
      fprintf(stdout, "thread %d, start %p, end %p\n", i, mdata[i].memp, mdata[i].memp+(length-1));
#endif
    }
  }
  else {

    int length = BSIZE/sizeof(uint64_t)/(nthreads/2);
    for (int i=0; i<nthreads/2; i++) {
      mdata[i].id = i;
      mdata[i].index = i*length;
      mdata[i].max_pos = i*length+(length-1);

      mdata[i].memp = p + mdata[i].index;

#ifdef DEBUG_LOG_BUILDER
      fprintf(stdout, "thread %d, start %p, end %p\n", i, mdata[i].memp, mdata[i].memp+(length-1));
#endif
    }

    // other NUMA node
    for (int i=0; i<nthreads/2; i++) {
      mdata[i+(nthreads/2)].id = i+(nthreads/2);
      mdata[i+(nthreads/2)].index = i*length;
      mdata[i+(nthreads/2)].max_pos = i*length+(length-1);

      mdata[i+(nthreads/2)].memp = p2 + mdata[i].index;

#ifdef DEBUG_LOG_BUILDER
      fprintf(stdout, "thread %d, start %p, end %p\n", i+(nthreads/2), mdata[i+(nthreads/2)].memp, mdata[i+(nthreads/2)].memp+(length-1));
#endif
    }
  }
#else
  int length = BSIZE/sizeof(uint64_t)/nthreads;
  for (int i=0; i<nthreads; i++) {
    mdata[i].id = i;
    mdata[i].index = i*length;
    mdata[i].max_pos = i*length+(length-1);

    mdata[i].memp = p + mdata[i].index;

#ifdef DEBUG_LOG_BUILDER
    fprintf(stdout, "thread %d, start %p, end %p\n", i, mdata[i].memp, mdata[i].memp+(length-1));
#endif
  }
#endif

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
  mdata[0].memp += 2;

  int num_retries = 0;
  while(1) {
  
    int shardOffset[32];     // wont work for more than 32 shards
    for (int i=0; i<nshards; i++)
      shardOffset[i] = 0;

    // choose a thread number
    int next_tid = rand() % nthreads;

    // pick the number of entries to add
    //int wr_set = (rand() % WRITE_SET_MAX)+1;
    // always 32
    //int wr_set = 32;
    int wr_set = nshards;


    // first position of the new log entry
    //logAddr = p + mdata[next_tid].index;
    logAddr = mdata[next_tid].memp;


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

      uint64_t address = rand() % TARGET_MAX_ADDR;
      // force the address to be aligned to its shard
      //printf("was %x ", address);
#ifdef USE_NUMA    
      //address = (address & ~0x0F) + i/2;
      address = (address & ~(nshards-1)) + i/2;
#else
      address = (address & ~0x1F) + i;
#endif

      address = address << 3; // 8-byte aligned

#ifdef USE_NUMA
      // even shards acess same NUMA node
      if ((i%2) == 0) address += (uint64_t)target_NUMA1;
      else address += (uint64_t)target_NUMA2;
      // address += (uint64_t)target_NUMA1;
#else
      address += (uint64_t)target_pointer;
#endif


      //printf("new %x\n", address);
      uint64_t value = (rand() % MAX_INT64)+1;
 

      //int shardId = address % nshards;
      int shardId = i;
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
      fprintf(stdout, "[%p] = (%x %lu)\n", logAddr+offsetToWrite+1, address, value);
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
  flush_clwb_nolog((void *)pmem_pointer, BSIZE);
  _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif


//  fprintf(stdout, "write-set mean %d\n", writes/2/num_logs);
  fprintf(stdout, "logs: %d, entries: %lu\n", numLogs, numEntries);

  free(mdata);
}






int main(int argc, char *argv[])
{
  int num_threads;
  struct timeval start, end;
  double elapsed_time;
  thr_parms_t *threadData;
  thrd_t *threadId;
  int command = 0;    // 0 - generate logs,  1 - replay
  int logtype = 0;    // 0 - distributed, 1 - sharded
  int num_shards = 1; // if sharded, 3rd options is the number of shards

  if (argc <= 3) {
    fprintf(stderr, "Invalid parameter number: use <command> <logtype> [<shards>] <threads>\n");
    exit(-1);
  }

  command = strtol(argv[1], NULL, 10);
  logtype = strtol(argv[2], NULL, 10);
  if (logtype == 1) {
    if (argc <= 3) {
      fprintf(stderr, "Invalid parameter number: use <command> <logtype> [<shards>] <threads>\n");
      exit(-1);
    }
    num_shards = strtol(argv[3], NULL, 10);
    num_threads = strtol(argv[4], NULL, 10);
  } else {
    num_threads = strtol(argv[3], NULL, 10);
  }

  fprintf(stdout, "Command: %d %s\n", command, command==0?"build logs":"replay");
  fprintf(stdout, "Log type: %d %s\n", logtype, logtype==0?"distributed":"sharded");
  if (logtype == 1) fprintf(stdout, "Num shards: %d\n", num_shards);
  fprintf(stdout, "Threads: %d\n", num_threads);

  threadData = (thr_parms_t *)malloc(sizeof(thr_parms_t)*num_threads);
  threadId = (thrd_t *)malloc(sizeof(thrd_t)*num_threads);
  // TODO: check for correct allocation

#ifdef USE_DRAM
  pLog = (uint64_t *)calloc(BSIZE,1);
  pSnapshot = (uint64_t *)calloc(BSIZE,1);
#ifdef USE_NUMA
  pLog2 = (uint64_t *)calloc(BSIZE,1);
  pSnapshot2 = (uint64_t *)calloc(BSIZE,1);
#endif
#else
//#ifdef CREATE_LOG
  if (command == 0) // generate
    if (logtype == 0) {
      pLog = (uint64_t *)createEmptyNVRAM(NVRAM_REGIONS[0], LOGNAMEDIST, BSIZE, MAP_SHARED);
#ifdef USE_NUMA
      pLog2 = (uint64_t *)createEmptyNVRAM(NVRAM_REGIONS[1], LOGNAMEDIST, BSIZE, MAP_SHARED);
#endif
    }
    else {
      pLog = (uint64_t *)createEmptyNVRAM(NVRAM_REGIONS[0], LOGNAMESHARD, BSIZE, MAP_SHARED);
#ifdef USE_NUMA
      pLog2 = (uint64_t *)createEmptyNVRAM(NVRAM_REGIONS[1], LOGNAMESHARD, BSIZE, MAP_SHARED);
#endif
    }
  else             // replay
    if (logtype == 0) {
      pLog = (uint64_t *)openNVRAM(NVRAM_REGIONS[0], LOGNAMEDIST, BSIZE, MAP_SHARED);
#ifdef USE_NUMA
      pLog2 = (uint64_t *)openNVRAM(NVRAM_REGIONS[1], LOGNAMEDIST, BSIZE, MAP_SHARED);
#endif
    }
    else {
      pLog = (uint64_t *)openNVRAM(NVRAM_REGIONS[0], LOGNAMESHARD, BSIZE, MAP_SHARED);
#ifdef USE_NUMA
      pLog2 = (uint64_t *)openNVRAM(NVRAM_REGIONS[1], LOGNAMESHARD, BSIZE, MAP_SHARED);
#endif
    }
  
  if (pLog == 0) {
    fprintf(stderr, "quitting ...");
    return -1;
  }
#ifdef USE_NUMA
  if (pLog2 == 0) {
    fprintf(stderr, "quitting ...");
    return -1;
  }
#endif

  pSnapshot = (uint64_t *)createEmptyNVRAM(NVRAM_REGIONS[0], SNAPNAME, BSIZE, MAP_SHARED);
  if (pSnapshot == 0) {
    fprintf(stderr, "quitting ...");
    return -1;
  }
#ifdef USE_NUMA
  pSnapshot2 = (uint64_t *)createEmptyNVRAM(NVRAM_REGIONS[1], SNAPNAME, BSIZE, MAP_SHARED);
  if (pSnapshot2 == 0) {
    fprintf(stderr, "quitting ...");
    return -1;
  }
#endif

#endif

///  if (command == 0) {

///    fprintf(stdout, "Generating log\n");

///    if (logtype == 0)
      ///generate_log_distributed(pLog, num_threads);
      ///generate_log_distributed(pLog, 32);
      //generate_log_shards(pLog, 1, 32);
#ifdef USE_NUMA
//      generate_opt_log_shards(pLog, pLog2, pSnapshot, pSnapshot2, 0, num_shards, num_threads);
      generate_opt_log_shards(pLog, pLog2, pSnapshot, pSnapshot2, 1, num_shards, num_threads);
      //generate_opt_log_shards(pLog, pLog2, pSnapshot, pSnapshot2, 1, 2);
#else
      generate_opt_log_shards(pLog, pSnapshot, 32);
#endif
///    else
///      generate_log_shards(pLog, num_threads, num_shards);

///    fprintf(stdout, "done\n");
///    return 0;
//  }

  fprintf(stdout, "Starting... %p\n", pLog);
#ifdef USE_NUMA
  fprintf(stdout, "Starting (2)... %p\n", pLog2);
#endif
  
/*  
  int length = BSIZE/sizeof(uint64_t);
  uint32_t num_logs = 0;
  uint32_t writes = 0;
  for (int i=0; i<length; i++) {
    if (pLog[i] >> 63) { num_logs++; i++; }
    else {
      if (pLog[i]) writes++;
    }
  }
  fprintf(stdout, "write-set mean %d\n", writes/2/num_logs);
  fprintf(stdout, "logs: %d, entries: %lu\n", num_logs, writes/2);


  exit(1);
*/


  for (int i=0; i < num_threads; i++) {
    threadData[i].id = i;
    threadData[i].start_addr = pLog;
    threadData[i].dest_addr = pSnapshot;
#ifdef USE_NUMA
    threadData[i].dest_addr2 = pSnapshot2;
#endif
    threadData[i].length = BSIZE/sizeof(uint64_t);
    threadData[i].total_threads = num_threads;
    threadData[i].nshards = num_shards;
    threadData[i].granularity = threadData[i].nshards/num_threads;       // assuming max of 32.. makes sense only if power of 2
  }
  //size_t rest = (BSIZE/sizeof(int32_t))%num_threads;
  //if (rest != 0) threadData[num_threads-1].length += rest;
 

  gettimeofday(&start, NULL);

  for (int i=0; i < num_threads; ++i) {
    if (logtype == 0) {
      if (thrd_create(threadId+i, applier_dist, threadData+i) != thrd_success) {
        printf("%d-th thread create error\n", i);
        return 0;
      }
    }
    else {
      if (thrd_create(threadId+i, applier_shards, threadData+i) != thrd_success) {
        printf("%d-th thread create error\n", i);
        return 0;
      }
    }
  }
    
  int total_entries_applied = 0;
  for (int i=0; i < num_threads; ++i) {
    int entries_applied;
    thrd_join(threadId[i], &entries_applied);
    total_entries_applied += entries_applied;
  }


#ifdef BATCH_FLUSH
  flush_clwb_nolog((void *)pSnapshot, TARGET_MAX_ADDR*sizeof(uint64_t));
  _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#ifdef USE_NUMA
  flush_clwb_nolog((void *)pSnapshot2, TARGET_MAX_ADDR*sizeof(uint64_t));
  _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif
#endif

#ifndef USE_DRAM
#ifndef BATCH_FLUSH
#ifndef USE_WBINVD
  _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
#endif
#endif
#endif

  gettimeofday(&end, NULL);

  elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
  fprintf(stdout, "Elapsed time: %g s\n", elapsed_time);
  fprintf(stdout, "Entries applied: %lu\n", total_entries_applied);
  fprintf(stdout, "Throughput: %g entries/s\n", (float)total_entries_applied/elapsed_time);


/*  
  for (int i=0; i<TARGET_MAX_ADDR; i++) {
    fprintf(stdout, "[%d] = %lu\n", i, pSnapshot[i]);
  }
#ifdef USE_NUMA
  fprintf(stdout, "NUMA2\n");
  for (int i=0; i<TARGET_MAX_ADDR; i++) {
    fprintf(stdout, "[%d] = %lu\n", i, pSnapshot2[i]);
  }
#endif
*/

  return 0;
}
