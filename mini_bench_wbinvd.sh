#!/bin/bash

EXPERIMENT_TIME=10000000
FIX_NUMBER_OF_TXS=5000000
PINNING=1
SAMPLES=10

mkdir -p mini_bench

cd ./nvhtm/
make clean ; make -j40 OPTIMIZE=1 DISABLE_FLAG_IS_IN_TX=1 DISABLE_FLAG_SNAPSHOT=1
cd -

declare -A file_name=( ["LOG_REPLAY_BACKWARD"]="BACKWARD" ["LOG_REPLAY_NORMAL"]="NORMAL" \
  ["LOG_REPLAY_RANGE_FLUSHES"]="RANGE" ["LOG_REPLAY_BUFFER_WBINVD"]="BUFFER-WBINVD" \
  ["LOG_REPLAY_BUFFER_FLUSHES"]="BUFFER-FLUSHES" ["LOG_REPLAY_SYNC_SORTER"]="SYNC-SORTER" \
  ["LOG_REPLAY_ASYNC_SORTER"]="ASYNC-SORTER")


for s in $(seq $SAMPLES)
do
  # useLogicalClocks usePhysicalClocks usePCWM 
  for sol in usePhysicalClocks
  do
    # LOG_REPLAY_NORMAL LOG_REPLAY_BUFFER_WBINVD LOG_REPLAY_BUFFER_FLUSHES
    for log_sol in LOG_REPLAY_BUFFER_WBINVD LOG_REPLAY_RANGE_FLUSHES
    do
      #LOG_REPLAY_ASYNC_SORTER
      for log_sorter in LOG_REPLAY_SYNC_SORTER
      do
        echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY" \
          > mini_bench/${sol}-${file_name[$log_sol]}-${file_name[$log_sorter]}_s${s}.tsv
        echo -e "SIZE" \
          > mini_bench/param_${sol}-${file_name[$log_sol]}-${file_name[$log_sorter]}_s${s}.tsv
        t=8 # 8 threads 
        for size in 2097152 4194304 8388608 16777216 33554432 67108864 134217728 268435456 536870912
        do
          echo "sample=$s s=$size" 
          echo "$size" >> mini_bench/param_${sol}-${file_name[$log_sol]}-${file_name[$log_sorter]}_s${s}.tsv
          ./nvhtm/test_spins FIX_NUMBER_OF_TXS=$FIX_NUMBER_OF_TXS MALLOC_SIZE=$size SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 NB_READS=64 NB_WRITES=4 disableLogChecker=1 FORCE_LEARN=1 \
            tid0Slowdown=0 $sol=1 NB_THREADS=$t PINNING=$PINNING PROFILE_FILE="./mini_bench/prof_${sol}-${file_name[$log_sol]}-${file_name[$log_sorter]}_s${s}.tsv" \
            ERROR_FILE="./mini_bench/error_${sol}-${file_name[$log_sol]}-${file_name[$log_sorter]}_s${s}.tsv" $log_sol $log_sorter \
            LOG_REPLAY_STATS_FILE="./mini_bench/log_${sol}-${file_name[$log_sol]}-${file_name[$log_sorter]}_s${s}.tsv" \
            | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n | tail -n 1 >> mini_bench/${sol}-${file_name[$log_sol]}-${file_name[$log_sorter]}_s${s}.tsv
          sleep 0.05s
        done
      done
    done
  done
done



