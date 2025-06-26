# CCPE25 SPHT_paging

The focus is to stress the capabilities of a given strategy to commit a PTM transaction (HTM+PM). In this version, 
with a paging mechanism add-on, capable of working under memory constrained situations.

### User guide can be found at ```UserGuide.rtf```

## Prerequisites

An Intel machine with TSX enabled. Follow the info in the ```rsync.sh``` script.

In order to compile, run ```makeall.sh``` following one of the PRESETS:
  - [1] Replayer enabled, Paging and Hashmap (Paging component) disabled (FAST'21 config with some tweaks)
  - [2] Paging enabled, Replayer and Hashmap (ImpHash) enabled*
  - [3] Replayer and Paging enabled, Hashmap disabled (Used to check main Paging logic overhead)
  - [4] Paging enabled, Replayer and Hashmap (OpenHash) enabled*
  - [5] Replayer, Paging and Swap enabled*
  - [6] Replayer, Paging and Hashmap disabled (Most barebones version, for debug purposes)

* Settings presented in the paper

There is a ```deps/``` folder with all the dependencies, if not using ```makeall.sh```, compile each first.

File paths should be altered at ```src/global_structs.c```

## What the code can do

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
  - File paths should be altered accordingly at ```src/global_structs.c```
  - We also encourage to verify the size of "vm.max_map_count", in our experiments we used it equal to 5000000
  - STAMP and TPCC have default setting in their respective ```nvhtm_params.txt```

### Data post process
  -  If using “bench.sh”, running “process_data.sh” will check the structure given by the OUTPUT FORMAT and:
    -  Calculate average “avg” from the number of executions made, and the standard deviation “stdev” and save them on the OUTPUT FORMAT folder
    -  Will also create plots for each workload, and if there are more than one, “plot_throughput_multiparams.py” will create plots will all together for ease of comparison
      -  TPCC EXCLUSIVE: Inside “TPCC” folders, “plot_throughput_mashup.py” will gather all the “data*” folders with the data from a single “warehouse” value and create a folder “mashup$NB_WAREHOUSES”, copy the data required with its naming scheme and create the plots inside
  -  Note that the post processing scripts require perfect data files, if not, no output is produced for that configuration
    
### 1) test_spins
  -   Located at ```/nvhtm```
  -   Simplistic benchmark for module testing (very granular controls)
  -   As an example, here are some suggested parameters to start running it via cli:
	  ```./test_spins EXPERIMENT_TIME=5000000 SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 NB_READS=1 NB_WRITES=1 disableLogChecker=1 FORCE_LEARN=1 tid0Slowdown=0 usePCWM=1 NB_THREADS=1 PINNING=1 PROFILE_FILE="prof_file" ERROR_FILE="error_file" LOG_REPLAY_STATS_FILE="lala3" 	TOTAL_MEMORY=1048576 PERC_SIZE_WORKING_SET=50 ZIPF_ALPHA=70```

### 2) STAMP
  -   Located at ```/bench/stamp```
  -   Original source: "https://github.com/kozyraki/stamp"
  -   It can be executed by running the ```bench.sh``` script, followed by the configuration code (same as makeall.sh)
    -   ```benches_args.sh``` has the suggested presets for each benchmark

### 3) TPCC
  -  Located at ```/bench/tpcc```
  -  It can be executed by running the ```bench.sh``` script 
    -   ```benches_args.sh``` has the suggested presets for each benchmark  
  -  "finalscript.sh" was a late automation, allowing the whole test suite to run with a single command
    -  "finalscript_benches_argsB1.sh" and "finalscript_benches_argsB2.sh" have the arguments for each experiment batch, those being, light-focus and heavy-focus workloads, respectively
    -  The results can be processed by running "process_finalscript.sh"

#### Output format
  - Output files will be created in folders following the naming scheme, inside a “data” folder (this structure is required for post processing (see “PROCESS_DATA.SH”): “data” + PRESET + SOL + NUM_WAREHOUSES
  - PRESET = bench.sh preset int value (see "Pre-requisites")
  - SOLUTIONS = e.g.: (see respective bench.sh for more) - "SOLUTIONS" in bench.sh and "sol" in post_process.sh should match
	usePCWM=P
	useHTM=H
	usePCWMeADRT1=PADRT
  - NUM_WAREHOUSES = int value set in "benches_args.sh"
    
