#ifndef EIGENBENCH_H_GUARD
#define EIGENBENCH_H_GUARD

#include <stdint.h>
#include <stdlib.h>

#define EB_RAND_R_FNC(seed) ({ \
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

// TODO: check cross-platform
#define EB_NOP_INST asm volatile ("NOP" ::: "memory")

typedef struct eb_params_ {
  int tid;        // set by the benchemark (value {0..N-1})
  long loops;     // Benchmark duration, number of TXs per thread
  int persist;    // Restore
  float lct;      // Probability of address repetition
  long R1;        // Reads/TX on hot array
  long W1;        // Write/TX on not array
  long R2;        // Reads/TX on mild array
  long W2;        // Write/TX on mild array
  long R3_i;      // Reads of Cold array inside TX
  long W3_i;      // Writes of Cold array inside TX
  long Nop_i;     // No-ops between TM accesses
  long k_i;       // Scaler for in-TX local ops
  long R3_o;      // Reads of Cold array outside TX
  long W3_o;      // Writes of Cold array outside TX
  long Nop_o;     // No-ops outside TXs
  long k_o;       // Scaler for out-TX local ops
  long A1;        // Size of Array1 (Hot array)
  long A2;        // Size of Array2 (Mild array)
  long A3;        // Size of Array3 (Cold array)
  long N;         // Number of threads
  long *Array1;   // Hot Array
  long *Array2;   // Mild Array
  long *Array3;   // Cold Array
  uint64_t seed;
} eb_params_s;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern double eb_run_duration;

void eb_run(eb_params_s params);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EIGENBENCH_H_GUARD */
