#!/bin/bash

EXPERIMENT_TIME=10000000
# FIX_NUMBER_OF_TXS=800000
# FIX_NUMBER_OF_TXS=7000000
# FIX_NUMBER_OF_TXS=112000000
FIX_NUMBER_OF_TXS=100000000
# FIX_NUMBER_OF_TXS=25000000
PINNING=1
SAMPLES=10

mkdir -p mini_bench

cd ./nvhtm/
make clean ; make -j40 OPTIMIZE=1 DISABLE_FLAG_IS_IN_TX=1 DISABLE_FLAG_SNAPSHOT=1 NPROFILE=1
cd -

declare -A file_name=( ["LOG_REPLAY_BACKWARD"]="BACKWARD" ["LOG_REPLAY_ST_CLWB"]="ST-CLWB" \
  ["LOG_REPLAY_RANGE_FLUSHES"]="RANGE" ["LOG_REPLAY_BUFFER_WBINVD"]="BUFFER-WBINVD" \
  ["LOG_REPLAY_BUFFER_FLUSHES"]="BUFFER-FLUSHES" ["LOG_REPLAY_SYNC_SORTER"]="SYNC" \
  ["LOG_REPLAY_ASYNC_SORTER"]="ASYNC")

# MALLOC_SIZE=536870912
# MALLOC_SIZE=268435456
MALLOC_SIZE=33554432
# MALLOC_SIZE=2097152

for s in $(seq $SAMPLES)
do
  # useLogicalClocks usePhysicalClocks usePCWM 
  for sol in usePCWM usePCWM2
  do
    log_sol=LOG_REPLAY_BUFFER_WBINVD
    t=64
    for MALLOC_SIZE in 2097152 33554432 536870912
    do
      # LOG_REPLAY_ASYNC_SORTER
      echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY\tHTM_EXPLICIT\tHTM_OTHER" \
        > mini_bench/${sol}-${file_name[$log_sol]}-LOGS${t}-HEAP${MALLOC_SIZE}B_s${s}.tsv
      for NB_REPLAYERS in 1 2 4 8 16 32 64
      do
        echo "t=$t" 
        ./nvhtm/test_spins FIX_NUMBER_OF_TXS=$FIX_NUMBER_OF_TXS SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 NB_READS=0 NB_WRITES=5 disableLogChecker=1 FORCE_LEARN=1 \
          MALLOC_SIZE=$MALLOC_SIZE tid0Slowdown=0 $sol=1 NB_THREADS=$t PINNING=$PINNING PROFILE_FILE="./mini_bench/prof_${sol}-${file_name[$log_sol]}-LOGS${t}-HEAP${MALLOC_SIZE}B_s${s}.tsv" \
          LOG_REPLAY_PARALLEL NB_REPLAYERS=${NB_REPLAYERS} ERROR_FILE="./mini_bench/error_${sol}-${file_name[$log_sol]}-LOGS${t}-HEAP${MALLOC_SIZE}B_s${s}.tsv" $log_sol \
          LOG_REPLAY_STATS_FILE="./mini_bench/log_${sol}-${file_name[$log_sol]}-LOGS${t}-HEAP${MALLOC_SIZE}B_s${s}.tsv" \
          | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n \
          | tail -n 1 >> mini_bench/${sol}-${file_name[$log_sol]}-LOGS${t}_s${s}.tsv
        sleep 0.05s
      done
    done
  done
done
