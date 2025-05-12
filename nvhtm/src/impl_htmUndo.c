#include "impl.h"
#include "spins.h"
#include "rdtsc.h"

#include <stdio.h>
#include <limits.h>
#include <stdint.h>

#include "htm_impl.h"

typedef uintptr_t bit_array_t;

int isSharedHTM;

static volatile __thread uint64_t readClockVal;

static volatile __thread
  int writeLogStart, writeLogEnd;

static volatile __thread uint64_t timeWaitingTS1 = 0;
static volatile __thread uint64_t timeWaitingTS2 = 0;
static volatile __thread uint64_t timeWaiting = 0;

static volatile __thread uint64_t countCommitPhases = 0;

static volatile uint64_t incCommitsPhases = 0;
static volatile uint64_t incTimeTotal = 0;
static volatile uint64_t incAfterTx = 0;
static volatile uint64_t incWaiting = 0;

void install_bindings_htmUndo()
{
  on_before_htm_begin  = on_before_htm_begin_htmUndo;
  on_htm_abort         = on_htm_abort_htmUndo;
  on_before_htm_write  = on_before_htm_write_8B_htmUndo;
  on_before_htm_commit = on_before_htm_commit_htmUndo;
  on_after_htm_commit  = on_after_htm_commit_htmUndo;

  on_before_sgl_commit = on_before_sgl_commit_htmUndo;
  on_after_sgl_begin   = on_after_sgl_begin_htmUndo;

  wait_commit_fn = wait_commit_htmUndo;
}

void state_gather_profiling_info_htmUndo(int threadId)
{
    __sync_fetch_and_add(&incCommitsPhases, countCommitPhases);
    __sync_fetch_and_add(&incTimeTotal, timeTotal);
    __sync_fetch_and_add(&incAfterTx, timeAfterTXSuc);
    __sync_fetch_and_add(&incWaiting, timeWaiting);
    __sync_fetch_and_add(&timeSGL_global, timeSGL);
}

void state_fprintf_profiling_info_htmUndo(char *filename)
{
    FILE *fp = fopen(filename, "a+");
    if (fp == NULL) {
      printf("Cannot open %s! Try to remove it.\n", filename);
      return;
    }
    fseek(fp, 0L, SEEK_END);
    if ( ftell(fp) < 8 ) {
        fprintf(fp, "#%s\t%s\t%s\t%s\t%s\t%s\n",
                "NB_THREADS",
                "NB_COMMIT_PHASES",
                "TIME_TOTAL",
                "TIME_AFTER_TX",
                "TIME_WAIT",
                "TIME_SGL");
    }
    fprintf(fp, "%i\t%lu\t%lu\t%lu\t%lu\n", gs_appInfo->info.nbThreads,
      incCommitsPhases, incTimeTotal, incAfterTx, incWaiting, timeSGL_global);
}

static inline void fetch_log(int threadId)
{
  write_log_thread[writeLogEnd] = 0;
  write_log_thread[(writeLogEnd + 8) & (gs_appInfo->info.allocLogSize - 1)] = 0;
}

void on_before_sgl_commit_htmUndo(int threadId) {
    // clear log
    __atomic_store_n(&G_next[threadId].log_ptrs.write_log_next, 0, __ATOMIC_RELEASE);
    writeLogEnd = writeLogStart = G_next[threadId].log_ptrs.write_log_next;
    // TODO: Confirm if __atomic_store_n might already impose the required thread fence
    __atomic_thread_fence(__ATOMIC_RELEASE);
}

void on_after_sgl_begin_htmUndo(int threadId) {
    // create log
    writeLogEnd = writeLogStart = G_next[threadId].log_ptrs.write_log_next;
}

void on_before_htm_begin_htmUndo(int threadId)
{
  onBeforeWrite = on_before_htm_write;
  onBeforeHtmCommit = on_before_htm_commit;
  write_log_thread = &(P_write_log[threadId][0]);

  if (__atomic_load_n(HTM_SGL_var_addr, __ATOMIC_ACQUIRE) != -1) {
    // Write default value into write_log_next to indicate we're inside an unfinished sgl transaction
    __atomic_store_n(&G_next[threadId].log_ptrs.write_log_next, -1, __ATOMIC_RELEASE);
    writeLogEnd = writeLogStart = G_next[threadId].log_ptrs.write_log_next;
    fetch_log(threadId);
  }

  MEASURE_TS(timeTotalTS1);
}

void on_htm_abort_htmUndo(int threadId) { /* empty */ }

void on_before_htm_write_8B_htmUndo(int threadId, void *addr, uint64_t val)
{
    // Write undo log if in SGL
    if (threadId == __atomic_load_n(HTM_SGL_var_addr, __ATOMIC_ACQUIRE)) {
        write_log_thread[writeLogEnd] = (uint64_t)addr;
        writeLogEnd = (writeLogEnd + 1) & (gs_appInfo->info.allocLogSize - 1);
        write_log_thread[writeLogEnd] = *((uint64_t*)addr);
        writeLogEnd = (writeLogEnd + 1) & (gs_appInfo->info.allocLogSize - 1);
    }
}

void on_before_htm_commit_htmUndo(int threadId) { /* empty */ }

void on_after_htm_commit_htmUndo(int threadId)
{
    MEASURE_TS(timeTotalTS2);
    MEASURE_INC(countCommitPhases);
    INC_PERFORMANCE_COUNTER(timeTotalTS1, timeTotalTS2, timeTotal);
}

void wait_commit_htmUndo(int threadId) { /* empty */ }
