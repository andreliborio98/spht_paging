#!/bin/bash

EXPERIMENT_TIME=10000000
# FIX_NUMBER_OF_TXS=800000
# FIX_NUMBER_OF_TXS=7000000
# FIX_NUMBER_OF_TXS=112000000
FIX_NUMBER_OF_TXS=20000000
# FIX_NUMBER_OF_TXS=25000000
PINNING=1
SAMPLES=10

FOLDER=data_mini_bench_log_sort_vs_link_1thr
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
# MALLOC_SIZE=2097152

for s in $(seq $SAMPLES)
do
  # usePCWM 
  for sol in usePCWM2
  do
    log_sol=LOG_REPLAY_BUFFER_WBINVD
    t=64
    for NB_REPLAYERS in 2 4
    do
      for NB_WRITES in 1 5
      do
        echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY\tHTM_EXPLICIT\tHTM_OTHER" \
          > $FOLDER/${sol}-${file_name[$log_sol]}-LOGS${t}-REP${NB_REPLAYERS}-W${NB_WRITES}_s${s}.tsv
        echo -e "MALLOC_SIZE" \
          > $FOLDER/params_${sol}-${file_name[$log_sol]}-LOGS${t}-REP${NB_REPLAYERS}-W${NB_WRITES}_s${s}.tsv
        for MALLOC_SIZE in 1048576 2097152 4194304 8388608 16777216 33554432 67108864 134217728 268435456 536870912 1073741824 2147483648 4294967296
        do
        # LOG_REPLAY_ASYNC_SORTER
          echo -e "$MALLOC_SIZE" \
            >> $FOLDER/params_${sol}-${file_name[$log_sol]}-LOGS${t}-REP${NB_REPLAYERS}-W${NB_WRITES}_s${s}.tsv
          echo "t=$t" 
          ./nvhtm/test_spins \
            FIX_NUMBER_OF_TXS=$FIX_NUMBER_OF_TXS \
            NB_READS=0 \
            NB_WRITES=${NB_WRITES} \
            MALLOC_SIZE=$MALLOC_SIZE \
            $sol=1 \
            NB_THREADS=$t \
            PINNING=$PINNING \
            LOG_REPLAY_PARALLEL \
            NB_REPLAYERS=${NB_REPLAYERS} \
            ERROR_FILE="./$FOLDER/error_${sol}-${file_name[$log_sol]}-LOGS${t}-REP${NB_REPLAYERS}-W${NB_WRITES}_s${s}.tsv" $log_sol \
            PROFILE_FILE="./$FOLDER/prof_${sol}-${file_name[$log_sol]}-LOGS${t}-REP${NB_REPLAYERS}-W${NB_WRITES}_s${s}.tsv" \
            LOG_REPLAY_STATS_FILE="./$FOLDER/log_${sol}-${file_name[$log_sol]}-LOGS${t}-REP${NB_REPLAYERS}-W${NB_WRITES}_s${s}.tsv" \
            | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n \
            | tail -n 1 >> $FOLDER/${sol}-${file_name[$log_sol]}-LOGS${t}-REP${NB_REPLAYERS}-W${NB_WRITES}_s${s}.tsv
          sleep 0.05s
        done
      done
    done
  done
done
