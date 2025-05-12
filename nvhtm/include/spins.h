#ifndef SPINS_H_GUARD
#define SPINS_H_GUARD

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef DEF_CPU_FREQ
#error "Please pass the CPU frequency (kHz) via -DDEF_CPU_FREQ=... "
#endif /* DEF_CPU_FREQ */

typedef void(*spin_fn_t)(unsigned long cyclesOrSpins);

static const double LATENCY_NS = 500.0;
static const double CPU_FREQ = 3000000.00; //DEF_CPU_FREQ; TEMPORARY (BUT ACCURATE), BUG IN MAKEFILE WHEN USING HASHMAP
static const unsigned long long LEARN_SPINS_TEST_SIZE = 99999999;

// extern unsigned long learnedSpins;
extern spin_fn_t spin_fn; // defaults to wait_commit_pc_simple
// extern volatile __thread unsigned long spin_flush_ts, spin_flush_ts2;
// extern volatile __thread unsigned long spin_nb_flushes;

void spin_nops(unsigned long nbSpins);
void spin_cycles(unsigned long nbCycles);
void learn_spin_nops(double latencyInNanoSec, double cpuFreqInKiloHertz, int isForceTest);

#define FLUSH_X86_INST "clwb"
// #define FLUSH_X86_INST "clflushopt"
// #define FLUSH_X86_INST "clflush"

#define FENCE_X86_INST "sfence"

// TODO: removed emulation
// if (spin_nb_flushes == 0) spin_flush_ts = rdtsc();
// spin_nb_flushes++;
#define FLUSH_CL(_addr) \
  __asm__ volatile(FLUSH_X86_INST " (%0)" : : "r"((void*)((uint64_t)(_addr) & -ARCH_CACHE_LINE_SIZE)) : "memory") \
//

// while (rdtsc() - spin_flush_ts < learnedSpins*spin_nb_flushes);
// spin_nb_flushes = 0;
#define FENCE_PREV_FLUSHES() \
  __asm__ volatile(FENCE_X86_INST : : : "memory"); \
//

// allow circular buffer
// if (spin_nb_flushes == 0) spin_flush_ts = rdtsc();
// spin_nb_flushes++;
#define FLUSH_RANGE(addr1, addr2, beginAddr, endAddr) \
  if (addr2 < addr1) { \
    for (uint64_t _addr = ((uint64_t)(addr1) & (uint64_t)-ARCH_CACHE_LINE_SIZE); \
                  _addr < (uint64_t)(endAddr); \
                  _addr += ARCH_CACHE_LINE_SIZE) { \
      __asm__ volatile(FLUSH_X86_INST " (%0)" : : "r"(((void*)_addr)) : "memory"); \
    } \
    for (uint64_t _addr = ((uint64_t)(beginAddr) & (uint64_t)-ARCH_CACHE_LINE_SIZE); \
                  _addr < (uint64_t)(addr2); \
                  _addr += ARCH_CACHE_LINE_SIZE) { \
      __asm__ volatile(FLUSH_X86_INST " (%0)" : : "r"(((void*)_addr)) : "memory"); \
    } \
  } else { \
    for (uint64_t _addr = ((uint64_t)(addr1) & (uint64_t)-ARCH_CACHE_LINE_SIZE); \
                  _addr < (uint64_t)(addr2); \
                  _addr += ARCH_CACHE_LINE_SIZE) { \
      __asm__ volatile(FLUSH_X86_INST " (%0)" : : "r"(((void*)_addr)) : "memory"); \
    } \
  } \
//

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* SPINS_H_GUARD */
