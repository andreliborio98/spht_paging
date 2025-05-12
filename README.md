# FAST'21 paper28 SPHT

The focus is to stress the capabilities of a given strategy to commit a PTM transaction (HTM+PM). In this version, 
with a paging mechanism add-on.

## Prerequisites

An Intel machine with TSX enabled. Follow the info in the rsync.sh script.

In order to compile, run ```makeall.sh``` following one of the presets:
  - [1] Replayer enabled, Paging and Hashmap (Paging component) disabled (FAST'21 config with some tweaks)
  - [2] Paging enabled, Replayer and Hashmap disabled
  - [3] Replayer and Paging enabled, Hashmap disabled (Used to check main Paging logic overhead)
  - [4] Replayer, Paging and Hashmap enabled (Basic Paging functionality)*
  - [5] Replayer, Paging and Swap enabled (Most complete Paging implementation)*
  - [6] Replayer, Paging and Hashmap disabled (Most barebones version, for debug purposes)

There is a ```deps/``` folder with all the dependencies, if not using ```makeall.sh```, compile each first.

File paths should be altered at ```src/global_structs.c```

## What the code do

Currently there are 5 different implementations for the commit phase:  
  -  [Logical Clock](nvhtm/src/impl_lc.c) - uses a logical clock to serialize transactions in the log (our prototype of DudeTM)  
  -  [Physical Clock](nvhtm/src/impl_pc.c) - logged transactions are sorted via monotonic, non-contiguos counter (our prototype of NV-HTM)  
  -  [ccHTM](nvhtm/src/impl_ccHTM.c) - The background process flushes the most recent queued transaction upon commit (our prototype of ccHTM)  
  -  [Crafty](nvhtm/src/impl_crafty.c) - our prototype of Crafty  
  -  [SPHT-NL](nvhtm/src/impl_pcwm.c) - our prototype of SPHT without links  
  -  [SPHT-FL](nvhtm/src/impl_pcwm2.c) - our prototype of SPHT with forward links  
  -  [SPHT-BL](nvhtm/src/impl_pcwm3.c) - our prototype of SPHT with backward links  

### Solutions that we are not using:

  -  [Epoch "Patient"](nvhtm/src/impl_epoch_sync.c) - transactions are logged in "batches" of N threads, if 1 thread is slow, then the other N-1 will have to wait  
  -  [Epoch "Impatient"](nvhtm/src/impl_epoch_impa.c) - solves the previous problem by imposing more synchronization (allows to "steal" a log slot from another thread if it is too slow)  
  -  [Physical+Logical](nvhtm/src/impl_pcwc.c) - WIP, the idea is to devise the logical clock from existing information, without having it actually mantaining it  

## Benchmark usage:
### test_spins
  -   Located at /nvhtm
  -   Simplistic benchmark for module testing
  -   Can be run by running a command line like the one below:
  ```./test_spins EXPERIMENT_TIME=5000000 SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 NB_READS=1 NB_WRITES=1 disableLogChecker=1 FORCE_LEARN=1 tid0Slowdown=0 usePCWM=1 NB_THREADS=1 PINNING=1 PROFILE_FILE="prof_file" ERROR_FILE="error_file" LOG_REPLAY_STATS_FILE="lala3" TOTAL_MEMORY=1048576 PERC_SIZE_WORKING_SET=50 ZIPF_ALPHA=70```

### STAMP
  -   Located at /bench/stamp
  -   Original source: "https://github.com/kozyraki/stamp"
  -   It can be executed by running the "bench.sh" script, followed by the configuration code (same as makeall.sh)
    -   "benches_args.sh" has the suggested presets for each benchmark

### TPCC
  -  Located at /bench/tpcc
  -  It can be executed by running the "bench.sh" script 
    -   "benches_args.sh" has the suggested presets for each benchmark
