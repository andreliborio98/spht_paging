#ifndef TM_IMPL_H_GUARD
#define TM_IMPL_H_GUARD

// TODO: these macros must be implemented by the TM lib
// a suggestion is to #undef #define in other header

#include "spins.h"
#include "impl.h"
#include "htm_impl.h"
#include "threading.h"
#include "input_handler.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// done outside the TX
#define EB_MALLOC(size) ({ void *_PTR = nvmalloc_local(HTM_SGL_tid, size); _PTR; })
#define EB_FREE(ptr)    nvfree(ptr)

/* TODO: HTM erases the seed on abort */
#define EB_TM_BEGIN()          NV_HTM_BEGIN(HTM_SGL_tid)
#define EB_TM_END()            NV_HTM_END(HTM_SGL_tid)
#define EB_TM_READ(addr)       *addr
#define EB_TM_WRITE(addr, val) ({ onBeforeWrite(HTM_SGL_tid, addr, val); *addr = val; val; })

#define EB_TM_INIT(nb_thrs)    HTM_init(nb_thrs); \
  input_parse_file((char*)"nvhtm_params.txt"); \
  learn_spin_nops(CPU_FREQ, FLUSH_LAT, /* FORCE_LEARN */1); \
  input_handler(); \
  global_structs_init( \
    nb_thrs, \
    1, \
    32768 /* NB_EPOCHS: use max of 1073741824 */, \
    134217728 /* LOG_SIZE: in nb of entries */, \
    5*1024*1024 /* MEM_REGION (5M) */, \
    /* SPINS_FLUSH */FLUSH_LAT, \
		(int*)G_PINNING_1, \
    (int*)G_NUMA_PINNING, \
    (char**)NVRAM_REGIONS \
  ) \
//

#define EB_TM_DESTROY()        printf("stats_nbSuccess: %li (%f\%)\nstats_nbAbort: %i (%f\%)\n\tconfl: %li (%f\%)\n\tcapac: %li (%f\%)\nstats_nbFallback: %li (%f\%)\n", \
    stats_nbSuccess, (float)(1+stats_nbSuccess) * 100.0f / (stats_nbSuccess+stats_nbAbort+stats_nbFallback+1), \
    stats_nbAbort, (float)(1+stats_nbAbort) * 100.0f / (stats_nbSuccess+stats_nbAbort+stats_nbFallback+1), \
    stats_nbConfl, (float)(1+stats_nbConfl) * 100.0f / (stats_nbAbort+1), \
    stats_nbCapac, (float)(1+stats_nbCapac) * 100.0f / (stats_nbAbort+1), \
    stats_nbFallback, (float)(1+stats_nbFallback) * 100.0f / (stats_nbSuccess+stats_nbAbort+stats_nbFallback+1)); \
  printf("THREADS\tTHROUGHPUT\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\n"); \
  printf("%i\t%lf\t%lu\t%lu\t%lu\t%lu\t%lu\n", HTM_get_nb_threads(), (stats_nbSuccess + stats_nbFallback) / eb_run_duration, \
    stats_nbSuccess, stats_nbFallback, stats_nbAbort, stats_nbConfl, stats_nbCapac); \
  if (state_print_profile != NULL) { \
    state_print_profile(PROFILE_FILE); \
  } \
  global_structs_destroy(); \
//

#define EB_THREAD_ENTER(tid)            HTM_thr_init(tid); \
  /* threading_pinThisThread(HTM_SGL_tid); */ /* Done in thread.h */ \
  HTM_set_is_record(1); \
//

#define EB_THREAD_EXIT(tid)             __atomic_fetch_add(&stats_nbSuccess,  HTM_get_status_count(HTM_SUCCESS, NULL),  __ATOMIC_RELAXED); \
  __atomic_fetch_add(&stats_nbAbort,    HTM_get_status_count(HTM_ABORT, NULL),    __ATOMIC_RELAXED); \
  __atomic_fetch_add(&stats_nbConfl,    HTM_get_status_count(HTM_CONFLICT, NULL), __ATOMIC_RELAXED); \
  __atomic_fetch_add(&stats_nbCapac,    HTM_get_status_count(HTM_CAPACITY, NULL), __ATOMIC_RELAXED); \
  __atomic_fetch_add(&stats_nbFallback, HTM_get_status_count(HTM_FALLBACK, NULL), __ATOMIC_RELAXED); \
  HTM_thr_exit(); \
//

void(*state_profile)(int);
void(*state_print_profile)(char*);
char PROFILE_FILE[1024];
char ERROR_FILE[1024];
int FLUSH_LAT;

long stats_nbSuccess;
long stats_nbFallback;
long stats_nbAbort;
long stats_nbConfl;
long stats_nbCapac;
double stats_benchTime;

static void input_handler()
{
  int usePhysicalClocks = 1;
  install_bindings_pc(); // may be overrided
  state_profile = state_gather_profiling_info_pc;
  state_print_profile = state_fprintf_profiling_info_pc;
  spin_fn = spin_cycles;

  if (input_exists((char*)"FLUSH_LAT")) {
    FLUSH_LAT = input_getLong((char*)"FLUSH_LAT");
  }
  printf("\nFLUSH_LAT = %i\n", FLUSH_LAT);

  if (input_exists((char*)"PROFILE_FILE")) {
    if (input_getString((char*)"PROFILE_FILE", PROFILE_FILE) >= 1024) {
      fprintf(stderr, "string copy exceeded the capacity of the buffer\n");
    }
  }
  printf("PROFILE_FILE = \"%s\"\n", PROFILE_FILE);

  if (input_exists((char*)"ERROR_FILE")) {
    extern FILE *error_fp;
    if (input_getString((char*)"ERROR_FILE", ERROR_FILE) >= 1024) {
      fprintf(stderr, "string copy exceeded the capacity of the buffer\n");
    }
    error_fp = fopen(ERROR_FILE, "a+");
  }
  printf("ERROR_FILE = \"%s\"\n", ERROR_FILE);

  extern int PCWC_haltSnoopAfterAborts;
  extern int PCWC2_haltSnoopAfterAborts;
  if (input_exists((char*)"ABORTS_BEFORE_STOP_SNOOP")) {
    PCWC_haltSnoopAfterAborts = input_getLong((char*)"ABORTS_BEFORE_STOP_SNOOP");
    PCWC2_haltSnoopAfterAborts = input_getLong((char*)"ABORTS_BEFORE_STOP_SNOOP");
  }
  printf("ABORTS_BEFORE_STOP_SNOOP is set to %i\n", PCWC_haltSnoopAfterAborts);

  if (input_exists((char*)"useLogicalClocks")) {
    printf("useLogicalClocks is set\n");
    usePhysicalClocks = 0;
    install_bindings_lc();
    wait_commit_fn = wait_commit_lc;
    state_profile = state_gather_profiling_info_lc;
    state_print_profile = state_fprintf_profiling_info_lc;
} else if (input_exists((char*)"usePCWMeADRT1")) {
    printf("usePCWMeADRT1 is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm_eadr_t1();
    wait_commit_fn = wait_commit_pcwm_eadr_t1;
    state_profile = state_gather_profiling_info_pcwm_eadr_t1;
    state_print_profile = state_fprintf_profiling_info_pcwm_eadr_t1;
} else if (input_exists((char*)"usePCWMeADRT2")) {
    printf("usePCWMeADRT2 is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm_eadr_t2();
    wait_commit_fn = wait_commit_pcwm_eadr_t2;
    state_profile = state_gather_profiling_info_pcwm_eadr_t2;
    state_print_profile = state_fprintf_profiling_info_pcwm_eadr_t2;
} else if (input_exists((char*)"usePCWMeADRT3")) {
    printf("usePCWMeADRT3 is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm_eadr_t3();
    wait_commit_fn = wait_commit_pcwm_eadr_t3;
    state_profile = state_gather_profiling_info_pcwm_eadr_t3;
    state_print_profile = state_fprintf_profiling_info_pcwm_eadr_t3;
} else if (input_exists((char*)"usePCWMeADRT4")) {
    printf("usePCWMeADRT4 is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm_eadr_t4();
    wait_commit_fn = wait_commit_pcwm_eadr_t4;
    state_profile = state_gather_profiling_info_pcwm_eadr_t4;
    state_print_profile = state_fprintf_profiling_info_pcwm_eadr_t4;
} else if (input_exists((char*)"usePCWMeADRT5")) {
    printf("usePCWMeADRT5 is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm_eadr_t5();
    wait_commit_fn = wait_commit_pcwm_eadr_t5;
    state_profile = state_gather_profiling_info_pcwm_eadr_t5;
    state_print_profile = state_fprintf_profiling_info_pcwm_eadr_t5;
  } else if (input_exists((char*)"usePCWMeADR")) {
    printf("usePCWMeADR is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm_eadr();
    wait_commit_fn = wait_commit_pcwm_eadr;
    state_profile = state_gather_profiling_info_pcwm_eadr;
    state_print_profile = state_fprintf_profiling_info_pcwm_eadr;
    } else if (input_exists((char*)"usePCWM")) {
    printf("usePCWM is set\n");
    usePhysicalClocks = 0;
    install_bindings_pcwm();
    wait_commit_fn = wait_commit_pcwm;
    state_profile = state_gather_profiling_info_pcwm;
    state_print_profile = state_fprintf_profiling_info_pcwm;
  } else if (input_exists((char*)"useUpperBound")) {
    printf("useUpperBound is set\n");
    usePhysicalClocks = 0;
    install_bindings_ub();
    wait_commit_fn = wait_commit_ub;
    state_profile = state_gather_profiling_info_ub;
    state_print_profile = state_fprintf_profiling_info_ub;
  } else if (input_exists((char*)"useEpochCommit1")) {
    printf("useEpochCommit1 is set /* patient version */\n");
    usePhysicalClocks = 0;
    install_bindings_epoch_sync();
    wait_commit_fn = wait_commit_epoch_sync;
    state_profile = state_gather_profiling_info_epoch_sync;
    state_print_profile = state_fprintf_profiling_info_epoch_sync;
  } else if (input_exists((char*)"useEpochCommit2")) {
    printf("useEpochCommit2 is set /* impatient version */\n");
    usePhysicalClocks = 0;
    install_bindings_epoch_impa();
    wait_commit_fn = wait_commit_epoch_impa;
    state_profile = state_gather_profiling_info_epoch_impa;
    state_print_profile = state_fprintf_profiling_info_epoch_impa;
  } else if (input_exists((char*)"usePCWC-F")) {
    printf("usePCWC-F is set");
    usePhysicalClocks = 0;
    install_bindings_pcwc();
    wait_commit_fn = wait_commit_pcwc;
    state_profile = state_gather_profiling_info_pcwc;
    state_print_profile = state_fprintf_profiling_info_pcwc;
  } else if (input_exists((char*)"usePCWC-NF")) {
    printf("usePCWC-NF is set");
    usePhysicalClocks = 0;
    install_bindings_pcwc2();
    wait_commit_fn = wait_commit_pcwc2;
    state_profile = state_gather_profiling_info_pcwc2;
    state_print_profile = state_fprintf_profiling_info_pcwc2;
    printf("\n");
} else if (input_exists("useHTMUndo")) {
    printf("useHTMUndo is set\n");
    usePhysicalClocks = 0;
    isSharedHTM = 0;
    install_bindings_htmUndo();
    wait_commit_fn = wait_commit_htmUndo;
    state_profile = state_gather_profiling_info_htmUndo;
    state_print_profile = state_fprintf_profiling_info_htmUndo;
    log_replay_flags = 0;
} else if (input_exists("useSharedHTMUndo")) {
    printf("useSharedHTMUndo is set\n");
    usePhysicalClocks = 0;
    isSharedHTM = 1;
    install_bindings_htmUndo();
    wait_commit_fn = wait_commit_htmUndo;
    state_profile = state_gather_profiling_info_htmUndo;
    state_print_profile = state_fprintf_profiling_info_htmUndo;
    log_replay_flags = 0;
  } else if (input_exists((char*)"useHTM")) {
    printf("useHTM is set\n");
    usePhysicalClocks = 0;
    isSharedHTM = 0;
    install_bindings_htmOnly();
    wait_commit_fn = wait_commit_htmOnly;
    state_profile = state_gather_profiling_info_htmOnly;
    state_print_profile = state_fprintf_profiling_info_htmOnly;
} else if (input_exists("useSharedHTM")) {
    printf("useSharedHTM is set\n");
    usePhysicalClocks = 0;
    isSharedHTM = 1;
    install_bindings_htmOnly();
    wait_commit_fn = wait_commit_htmOnly;
    state_profile = state_gather_profiling_info_htmOnly;
    state_print_profile = state_fprintf_profiling_info_htmOnly;
  } else if (input_exists((char*)"useCcHTM")) {
    printf("useCcHTM is set\n");
    usePhysicalClocks = 0;
    install_bindings_ccHTM();
    wait_commit_fn = wait_commit_ccHTM;
    state_profile = state_gather_profiling_info_ccHTM;
    state_print_profile = state_fprintf_profiling_info_ccHTM;
  } else if (input_exists((char*)"useEpochCommit3")) {
    printf("useEpochCommit3 is set /* deadline version */\n");
    usePhysicalClocks = 0;
    install_bindings_epoch_static_deadline();
    wait_commit_fn = wait_commit_epoch_static_deadline;
  }
  printf(" --- \n");
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TM_IMPL_H_GUARD */
