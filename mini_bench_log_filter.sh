#!/bin/bash

EXPERIMENT_TIME=10000000
# FIX_NUMBER_OF_TXS=800000
# FIX_NUMBER_OF_TXS=7000000
# FIX_NUMBER_OF_TXS=112000000
# FIX_NUMBER_OF_TXS=100000000
# FIX_NUMBER_OF_TXS=25000000
PINNING=1
SAMPLES=5

FOLDER=data_mini_bench_filter
mkdir -p $FOLDER

cd ./nvhtm/
make clean ; make -j40 OPTIMIZE=1 DISABLE_FLAG_IS_IN_TX=1 DISABLE_FLAG_SNAPSHOT=1 NPROFILE=1
cd -

declare -A file_name=( ["LOG_REPLAY_BACKWARD"]="BACKWARD" ["LOG_REPLAY_ST_CLWB"]="ST-CLWB" \
  ["LOG_REPLAY_RANGE_FLUSHES"]="RANGE" ["LOG_REPLAY_BUFFER_WBINVD"]="BUFFER-WBINVD" \
  ["LOG_REPLAY_BUFFER_FLUSHES"]="BUFFER-FLUSHES" ["LOG_REPLAY_SYNC_SORTER"]="SYNC" \
  ["LOG_REPLAY_ASYNC_SORTER"]="ASYNC")

# MALLOC_SIZE=536870912
# MALLOC_SIZE=268435456
# MALLOC_SIZE=33554432
# MALLOC_SIZE=1048576
MALLOC_SIZE=8589934592

for s in $(seq $SAMPLES)
do
  # useLogicalClocks usePhysicalClocks usePCWM 
  # 
  for sol in usePCWM2 usePCWM3
  # for sol in usePCWM
  do
    # for log_sol in LOG_REPLAY_BUFFER_WBINVD LOG_REPLAY_BACKWARD
    for log_sol in LOG_REPLAY_BUFFER_WBINVD
    do
      # log_sol=LOG_REPLAY_BUFFER_WBINVD
      t=64
      # for 
      # for FIX_NUMBER_OF_TXS in $(($MALLOC_SIZE / (16 * 5))) $(($MALLOC_SIZE / (8 * 5) * 5)) $(($MALLOC_SIZE / (8 * 5) * 10)) 
      for FIX_NUMBER_OF_TXS in $(($MALLOC_SIZE / (16 * 5))) $(($MALLOC_SIZE / (8 * 5) * 5)) $(($MALLOC_SIZE / (8 * 5) * 10)) 
      do
        # LOG_REPLAY_ASYNC_SORTER
        echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY\tHTM_EXPLICIT\tHTM_OTHER" \
          > $FOLDER/${sol}-${file_name[$log_sol]}-LOGS${t}-L${FIX_NUMBER_OF_TXS}_s${s}.tsv
        for NB_REPLAYERS in 1 2 4 8 16 32 64
        do
          echo "t=$t" 
          ./nvhtm/test_spins  SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 tid0Slowdown=0  \
            FIX_NUMBER_OF_TXS=$FIX_NUMBER_OF_TXS NB_READS=0 NB_WRITES=5 disableLogChecker=1 FORCE_LEARN=1 \
            $log_sol LOG_REPLAY_PARALLEL NB_REPLAYERS=${NB_REPLAYERS} \
            MALLOC_SIZE=$MALLOC_SIZE $sol=1 NB_THREADS=$t PINNING=$PINNING \
            PROFILE_FILE="./$FOLDER/prof_${sol}-${file_name[$log_sol]}-LOGS${t}-L${FIX_NUMBER_OF_TXS}_s${s}.tsv" \
            ERROR_FILE="./$FOLDER/error_${sol}-${file_name[$log_sol]}-LOGS${t}-L${FIX_NUMBER_OF_TXS}_s${s}.tsv" \
            LOG_REPLAY_STATS_FILE="./$FOLDER/log_${sol}-${file_name[$log_sol]}-LOGS${t}-L${FIX_NUMBER_OF_TXS}_s${s}.tsv" \
            | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n \
            | tail -n 1 >> $FOLDER/${sol}-${file_name[$log_sol]}-LOGS${t}-L${FIX_NUMBER_OF_TXS}_s${s}.tsv
          sleep 0.05s
        done
      done
    done
  done
done
