
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h> 
#include <emmintrin.h>

#include "log-replay.h"
#include "nvm.h"





//#define DEBUG_LOG_REPLAY



int replay_vanilla(void *data)
{
  int myid = (int)(uint64_t)data;

  //const int *NUMApin = g_logNuma == 1 ? G_PINNING_1 : G_PINNING_2;  
  const int *NUMApin = g_heapNuma == 1 ? G_PINNING_1 : G_PINNING_2;  
  int core = NUMApin[myid];
    
  int numaSocket = G_NUMA_PINNING[NUMApin[myid]];
  fprintf(stdout, "Thread %d pinned to Core %d and NUMA node %d\n",
                   myid, NUMApin[myid], numaSocket);

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
  //uint64_t *next_entry = mydata->start_addr;
  uint64_t *next_entry = g_logRegion[0];
  while (1) { 
 
    uint64_t addr = *next_entry;

    if (addr >> 63) { // timestamp

#ifdef DEBUG_LOG_REPLAY
      fprintf(stdout, "tid %d  Log address: %p = %p\n", myid, next_entry, addr);
#endif

      if (addr == LAST_TIMESTAMP) break; // the end
      
#ifdef DEBUG_LOG_REPLAY
      fprintf(stdout, "tid %d  Next address: %p = %p\n", 
          myid, next_entry+1, *(next_entry+1));
#endif

      // next field is the pointer
      next_entry++;
      next_entry = (uint64_t *)*next_entry;

      continue;  
    }
    else {
      next_entry++;  // points to data

      if (g_replayerThreads == 1 || // necessary in case heapNUMA is 2 and only 1 replayer
        (((addr>>12)%(g_replayerThreads/g_heapNuma) == myid/g_heapNuma) &&
          ((addr >= (uint64_t)g_heapRegion[numaSocket]) && (addr < (uint64_t)(g_heapRegion[numaSocket]+g_heapSize/sizeof(uint64_t)) )))) {    // 4k pages

// #ifdef DEBUG_LOG_REPLAY
        fprintf(stdout, "  tid %d  [%p] = %llu\n", 
            myid, addr, *next_entry);
// #endif

#ifdef USE_NT_STORE
        _mm_stream_si64 ((long long int*)(uint64_t *)addr, *next_entry);
#else
        *(uint64_t *)addr = *next_entry;

#ifdef USE_FLUSH_PER_STORE        
    		pmem_clwb((char *)addr);
#endif
#endif /* USE_NT_STORE */
        entries_applied++;
      }
    }
    next_entry++;

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


/*
 *
 *
 *
 *
 */
int replay_shards(void *data)
{

  int ns   = g_replayerThreads;
  int gran = g_workerThreads/ns;
  int id = (int)(uint64_t)data;
  

  //const int *NUMApin = g_logNuma == 1 ? G_PINNING_1 : G_PINNING_2;  
  const int *NUMApin = g_heapNuma == 1 ? G_PINNING_1 : G_PINNING_2;  
  int core = NUMApin[id];
    
  int numaSocket = G_NUMA_PINNING[NUMApin[id]];
  fprintf(stdout, "Thread %d pinned to Core %d and NUMA node %d\n",
                   id, NUMApin[id], numaSocket);

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
  //uint64_t *next_entry = (uint64_t *)*(mydata->start_addr+1);   // pointer to the first log entry
  uint64_t *next_entry = (uint64_t *)*(g_logRegion[0]+1);   // pointer to the first log entry

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

      //int idx = (i*mydata->total_threads)+id;
      int idx = (i*g_replayerThreads)+id;

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
#ifdef USE_FLUSH_PER_STORE        
        pmem_clwb((char *)addr);
#endif
#endif /* USE_NT_STORE */

        entries_applied++;

        offset += ns*2;

        addr = next_entry[offset];

      }
#ifdef DEBUG_LOG_REPLAY
      fprintf(stdout, "%d End of shard -- addr: %lu \n", id, addr);
#endif


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
