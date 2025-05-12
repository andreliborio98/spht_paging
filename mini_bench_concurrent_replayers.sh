#!/bin/bash

EXPERIMENT_TIME=4000000
PINNING=1
SAMPLES=10
# MALLOC_SIZE=1048576
# MALLOC_SIZE=33554432
# MALLOC_SIZE=8388608
# MALLOC_SIZE=8589934592
MALLOC_SIZE=4294967296
MALLOC_SIZE=16777216

FOLDER=data_mini_bench_concurrent_replayers
mkdir -p $FOLDER

cd ./nvhtm/
make clean ; make -j40 OPTIMIZE=1 NPROFILE=1 DISABLE_FLAG_SNAPSHOT=1
cd -

# declare -A workload_name=( ["2W4R-PRI"]="NB_READS=4 NB_WRITES=2" \
#   ["2W4R-SHA"]="NB_READS=4 NB_WRITES=2 SAME_MEM_POOL" \
#   ["2W32R-PRI"]="NB_READS=32 NB_WRITES=2" \
#   ["2W32R-SHA"]="NB_READS=32 NB_WRITES=2 SAME_MEM_POOL")
declare -A workload_name=(\
  ["usePCWM-5W0R-LARGE-1R"]="usePCWM NB_READS=0 NB_WRITES=5 MALLOC_SIZE=4294967296 NB_REPLAYERS=1" \
  ["usePCWM2-5W0R-LARGE-1R"]="usePCWM2 NB_READS=0 NB_WRITES=5 MALLOC_SIZE=4294967296 NB_REPLAYERS=1" \
  ["5W0R-LARGE-8R"]="NB_READS=0 NB_WRITES=5 MALLOC_SIZE=4294967296 NB_REPLAYERS=8" \
  ["usePCWM-5W0R-SMALL-1R"]="usePCWM NB_READS=0 NB_WRITES=5 MALLOC_SIZE=16777216 NB_REPLAYERS=1" \
  ["usePCWM2-5W0R-SMALL-1R"]="usePCWM2 NB_READS=0 NB_WRITES=5 MALLOC_SIZE=16777216 NB_REPLAYERS=1" \
  ["5W0R-SMALL-8R"]="NB_READS=0 NB_WRITES=5 MALLOC_SIZE=16777216 NB_REPLAYERS=8" \
  )

MALLOC_SIZE=4294967296
MALLOC_SIZE=16777216

LOG_SIZE=1024
NB_THREADS=16

### Flushes in-place
for s in $(seq $SAMPLES)
do
  # for sol in useCrafty usePSTM usePCWM2 usePCWM3 useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM
  # for sol in usePCWM
  # do
    for workload in usePCWM-5W0R-LARGE-1R usePCWM-5W0R-SMALL-1R
    do
      echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY\tHTM_EXPLICIT\tHTM_OTHER" \
        > $FOLDER/${workload}_s${s}.tsv
      echo -e "LOG_SIZE\tFIX_NUMBER_OF_TXS" \
        > $FOLDER/param-${workload}_s${s}.tsv
      for log_size in 16 32 64 128 256
      do
        FIX_NUMBER_OF_TXS=$(($NB_THREADS*8192*$log_size)) 
        ./nvhtm/test_spins \
          FIX_NUMBER_OF_TXS=$FIX_NUMBER_OF_TXS \
          ${workload_name[$workload]} \
          NB_THREADS=$NB_THREADS \
          PINNING=$PINNING \
          LOG_REPLAY_PARALLEL \
          LOG_REPLAY_CONCURRENT \
          NB_LOG_ENTRIES=$(($LOG_SIZE*$log_size)) \
          PROFILE_FILE="./$FOLDER/prof_${workload}_s${s}.tsv" \
          ERROR_FILE="./$FOLDER/error_${workload}_s${s}.tsv" \
          LOG_REPLAY_STATS_FILE="./$FOLDER/log_${workload}_s${s}.tsv" \
          | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n \
          | tail -n 1 >> $FOLDER/${workload}_s${s}.tsv
        echo -e "$(($LOG_SIZE*$log_size))\t$FIX_NUMBER_OF_TXS" \
          >> $FOLDER/param-${workload}_s${s}.tsv
        sleep 0.05s
        echo "$workload --> $log_size"
      done
    done
  # done
done

### Flushes in-place
for s in $(seq $SAMPLES)
do
  # for sol in useCrafty usePSTM usePCWM2 usePCWM3 useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM
  # for sol in usePCWM
  # do
    for workload in usePCWM-5W0R-LARGE-1R usePCWM-5W0R-SMALL-1R
    do
      echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY\tHTM_EXPLICIT\tHTM_OTHER" \
        > $FOLDER/${workload}-DIS_s${s}.tsv
      echo -e "LOG_SIZE\tFIX_NUMBER_OF_TXS" \
        > $FOLDER/param-${workload}-DIS_s${s}.tsv
      for log_size in 1 2 4 8 16 32 64 128 256
      do
        FIX_NUMBER_OF_TXS=$(($NB_THREADS*8192*$log_size)) 
        ./nvhtm/test_spins \
          FIX_NUMBER_OF_TXS=$FIX_NUMBER_OF_TXS \
          ${workload_name[$workload]} \
          NB_THREADS=$NB_THREADS \
          PINNING=$PINNING \
          DISABLE_LOG_REPLAY \
          NB_LOG_ENTRIES=$(($LOG_SIZE*$log_size)) \
          PROFILE_FILE="./$FOLDER/prof_${workload}-DIS_s${s}.tsv" \
          ERROR_FILE="./$FOLDER/error_${workload}-DIS_s${s}.tsv" \
          LOG_REPLAY_STATS_FILE="./$FOLDER/log_${workload}-DIS_s${s}.tsv" \
          | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n \
          | tail -n 1 >> $FOLDER/${workload}-DIS_s${s}.tsv
        echo -e "$(($LOG_SIZE*$log_size))\t$FIX_NUMBER_OF_TXS" \
          >> $FOLDER/param-${workload}-DIS_s${s}.tsv
        sleep 0.05s
        echo "$workload --> $log_size"
      done
    done
  # done
done


# cd bench/stamp
# ./bench.sh

