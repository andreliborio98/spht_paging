

#ifndef LOG_GENERATE_H
#define LOG_GENERATE_H


extern int g_workerThreads;
extern int g_logType;
extern int g_logNuma;
extern uint64_t *g_logRegion[2];    // pointers to the beginning of each NUMA region (logs)
extern int g_heapNuma;
extern uint64_t *g_heapRegion[2];    // pointers to the beginning of each NUMA region (heap)

extern uint64_t g_logSize;
extern uint64_t g_heapSize;


void generate_log_vanilla();
void generate_opt_log_shards();










#endif /* LOG_GENERATE_H */
