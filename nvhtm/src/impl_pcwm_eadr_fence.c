#include "impl.h"
#include "spins.h"
#include "rdtsc.h"

#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>

#include "htm_impl.h"

typedef uintptr_t bit_array_t;

static volatile __thread uint64_t readClockVal;

static volatile __thread int writeLogStart, writeLogEnd;

static volatile __thread int canJumpToWait = 0;

static volatile __thread uint64_t timeWaitingTS1 = 0;
static volatile __thread uint64_t timeWaitingTS2 = 0;
static volatile __thread uint64_t timeWaiting = 0;
static volatile __thread uint64_t timeFlushing = 0;
static volatile __thread uint64_t timeTX = 0;

static volatile __thread uint64_t countCommitPhases = 0;

static volatile uint64_t incCommitsPhases = 0;
static volatile uint64_t incTimeTotal = 0;
static volatile uint64_t incAfterTx = 0;
static volatile uint64_t incWaiting = 0;
static volatile uint64_t incFlushing = 0;
static volatile uint64_t incTX = 0;

void install_bindings_pcwm_eadr_t2()
{
    on_before_htm_begin  = on_before_htm_begin_pcwm_eadr_t2;
    on_htm_abort         = on_htm_abort_pcwm_eadr_t2;
    on_before_htm_write  = on_before_htm_write_8B_pcwm_eadr_t2;
    on_before_htm_commit = on_before_htm_commit_pcwm_eadr_t2;
    on_after_htm_commit  = on_after_htm_commit_pcwm_eadr_t2;

    wait_commit_fn = wait_commit_pcwm_eadr_t2;
}

void state_gather_profiling_info_pcwm_eadr_t2(int threadId)
{
    __sync_fetch_and_add(&incCommitsPhases, countCommitPhases);
    __sync_fetch_and_add(&incTimeTotal, timeTotal);
    __sync_fetch_and_add(&incAfterTx, timeAfterTXSuc);
    __sync_fetch_and_add(&incWaiting, timeWaiting);
    __sync_fetch_and_add(&incFlushing, timeFlushing);
    __sync_fetch_and_add(&incTX, timeTX);
    __sync_fetch_and_add(&timeSGL_global, timeSGL);
    __sync_fetch_and_add(&timeAbortedTX_global, timeAbortedTX);

    timeSGL = 0;
    timeAbortedTX = 0;
    timeTX = 0;
    timeAfterTXSuc = 0;
    timeWaiting = 0;
    timeTotal = 0;
    countCommitPhases = 0;
}

void state_fprintf_profiling_info_pcwm_eadr_t2(char *filename)
{
    FILE *fp = fopen(filename, "a+");
    if (fp == NULL) {
        printf("Cannot open %s! Try to remove it.\n", filename);
        return;
    }
    fseek(fp, 0L, SEEK_END);
    if ( ftell(fp) < 8 ) {
        fprintf(fp, "#%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
        "NB_THREADS",
        "NB_COMMIT_PHASES",
        "TIME_TOTAL",
        "TIME_AFTER_TX",
        "TIME_TX",
        "TIME_WAIT",
        "TIME_SGL",
        "TIME_ABORTED_TX",
        "TIME_AFTER_TX_FAIL",
        "TIME_FLUSH_LOG");
    }
    fprintf(fp, "%i\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\n", gs_appInfo->info.nbThreads,
    incCommitsPhases, incTimeTotal, incAfterTx, incTX, incWaiting, timeSGL_global, timeAbortedTX_global, 0L, incFlushing);
}

static inline void fetch_log(int threadId)
{
    write_log_thread[writeLogEnd] = 0;
    write_log_thread[(writeLogEnd + 8) & (gs_appInfo->info.allocLogSize - 1)] = 0;
}

void on_before_htm_begin_pcwm_eadr_t2(int threadId)
{
    onBeforeWrite = on_before_htm_write;
    onBeforeHtmCommit = on_before_htm_commit;
    write_log_thread = &(P_write_log[threadId][0]);

    writeLogEnd = writeLogStart = G_next[threadId].log_ptrs.write_log_next;
    fetch_log(threadId);
}

void on_htm_abort_pcwm_eadr_t2(int threadId) { /* empty */ }

void on_before_htm_write_8B_pcwm_eadr_t2(int threadId, void *addr, uint64_t val)
{
    write_log_thread[writeLogEnd] = (uint64_t)addr;
    writeLogEnd = (writeLogEnd + 1) & (gs_appInfo->info.allocLogSize - 1);
    write_log_thread[writeLogEnd] = (uint64_t)val;
    writeLogEnd = (writeLogEnd + 1) & (gs_appInfo->info.allocLogSize - 1);
}

void on_before_htm_commit_pcwm_eadr_t2(int threadId)
{
    write_log_thread[writeLogEnd] = onesBit63(rdtscp());
}

static inline void smart_close_log_pcwm(uint64_t marker, uint64_t *marker_pos)
{
//   FENCE_PREV_FLUSHES();
  intptr_t lastCL  = ((uintptr_t)(&write_log_thread[writeLogEnd]) >> 6) << 6;
  intptr_t firstCL = ((uintptr_t)(&write_log_thread[writeLogStart]) >> 6) << 6;

  void *logStart = (void*) (write_log_thread + 0);
  void *logEnd   = (void*) (write_log_thread + gs_appInfo->info.allocLogSize);
  
  // TODO: Should be possible use just this instead of the if-else section below
  // FLUSH_RANGE(firstCL, lastCL, logStart, logEnd);
  // FENCE_PREV_FLUSHES();

  if (firstCL == lastCL) {
    FLUSH_RANGE(firstCL, lastCL, logStart, logEnd);  // Might not be required
  } else {
    intptr_t lastCLMinus1;
    if (lastCL == (uintptr_t)logStart) {
      lastCLMinus1 = (uintptr_t)logEnd;
    } else {
      lastCLMinus1 = lastCL - 1;
    }
    FLUSH_RANGE(firstCL, lastCLMinus1, logStart, logEnd); // Might not be required
    FENCE_PREV_FLUSHES(); // Fence required to ensure consistency of cache pages
    FLUSH_CL((void*)lastCL); // Might not be required
  }
}

void on_after_htm_commit_pcwm_eadr_t2(int threadId)
{
    INC_PERFORMANCE_COUNTER(timeTotalTS1, timeAfterTXTS1, timeTX);

    if (writeLogStart != writeLogEnd) {
        __atomic_store_n(&G_next[threadId].log_ptrs.write_log_next, (writeLogEnd + 1) & (gs_appInfo->info.allocLogSize - 1), __ATOMIC_RELEASE);
        // Smart close log might not be required since the timestamp marker was written on_before_htm_commit_pcwm_eadr_t2
        smart_close_log_pcwm(
        /* commit value */ onesBit63(readClockVal),
        /* marker position */ (uint64_t*)&(write_log_thread[writeLogEnd])
        );
    }

    MEASURE_INC(countCommitPhases);
}

void wait_commit_pcwm_eadr_t2(int threadId) { /* empty */ }
