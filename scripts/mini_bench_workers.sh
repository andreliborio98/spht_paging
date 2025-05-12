#!/bin/bash

EXPERIMENT_TIME=5000000
PINNING=1
SAMPLES=1
# MALLOC_SIZE=1073741824
# MALLOC_SIZE=2147483648
# MALLOC_SIZE=4294967296
# MALLOC_SIZE=8589934592
# MALLOC_SIZE=17179869184
# MALLOC_SIZE=34359738368
# MALLOC_SIZE=68719476736
# MALLOC_SIZE=137438953472
# MALLOC_SIZE=274877906944
MALLOC_SIZE=171798691840

FOLDER=data_mini_bench_workers
mkdir -p $FOLDER

cd ../nvhtm/
make clean ; make -j40 OPTIMIZE=1
cd -

# declare -A workload_name=( ["2W4R-PRI"]="NB_READS=4 NB_WRITES=2" \
#   ["2W4R-SHA"]="NB_READS=4 NB_WRITES=2 SAME_MEM_POOL" \
#   ["2W32R-PRI"]="NB_READS=32 NB_WRITES=2" \
#   ["2W32R-SHA"]="NB_READS=32 NB_WRITES=2 SAME_MEM_POOL")
declare -A workload_name=(\
  ["2W64R-PRI"]="NB_READS=64 NB_WRITES=2" \
  ["2W64R-SHA"]="NB_READS=64 NB_WRITES=2 SAME_MEM_POOL" \
  ["2W4R-PRI"]="NB_READS=4   NB_WRITES=2" \
  ["2W8R-PRI"]="NB_READS=8   NB_WRITES=2" \
  ["2W16R-PRI"]="NB_READS=16 NB_WRITES=2" \
  ["8W0R-PRI"]="NB_READS=8   NB_WRITES=0" \
  ["4W64R-PRI"]="NB_READS=64 NB_WRITES=4" \
  ["5W64R-PRI"]="NB_READS=64 NB_WRITES=5" \
  ["4W4R-PRI"]="NB_READS=4   NB_WRITES=4" \
  ["5W0R-PRI"]="NB_READS=0   NB_WRITES=5" \
  ["5W5R-PRI"]="NB_READS=5   NB_WRITES=5" \
  ["1W16R-PRI"]="NB_READS=16 NB_WRITES=1" \
  ["1W2R-PRI"]="NB_READS=2   NB_WRITES=1" \
  ["2W2R-PRI"]="NB_READS=2   NB_WRITES=2" \
  ["10W5R-PRI"]="NB_READS=5   NB_WRITES=10" \
  ["25W5R-PRI"]="NB_READS=5   NB_WRITES=25" \
  ["50W5R-PRI"]="NB_READS=5   NB_WRITES=50" \
  ["5W50R-PRI"]="NB_READS=50   NB_WRITES=5" \
  # extras
  ["5W5R-PRI"]="NB_READS=5   NB_WRITES=5" \
  ["1W9R-PRI"]="NB_READS=9   NB_WRITES=1" \
  ["9W1R-PRI"]="NB_READS=1   NB_WRITES=9" \
  ["25W25R-PRI"]="NB_READS=25   NB_WRITES=25" \
  ["5W45R-PRI"]="NB_READS=45   NB_WRITES=5" \
  )

for s in $(seq $SAMPLES)
do
  # for sol in useCrafty usePSTM usePCWM2 usePCWM3 useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM
  # for sol in usePCWMeADR useSharedHTMUndo useSharedHTM useHTM useHTMShared useCrafty usePSTM usePCWM2 usePCWM3 useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM
  # for sol in usePCWMeADR usePCWM useSharedHTMUndo useSharedHTM useHTM
  for sol in useHTMUndo usePCWMeADRT1 usePCWMeADR usePCWM #HTMonly PCWMeADRT5 #plot lines - usePCWM = SPHT
  do
    for workload in 5W5R-PRI 25W25R-PRI 1W9R-PRI 9W1R-PRI 5W45R-PRI #4W4R-PRI 5W50R-PRI 50W5R-PRI
    do
      echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY\tHTM_EXPLICIT\tHTM_OTHER" \
        > $FOLDER/${sol}-${workload}_s${s}.tsv
      # for t in 1 2 4 6 8 12 16 20 24 28 32 36 40 48 56 64
      for t in 1 2 4 6 8 10 12 16 20 24 32 40 48
      do
        echo "t=$t"
        # HEAP_SIZE=$(($MALLOC_SIZE * $t))
        HEAP_SIZE=$MALLOC_SIZE
          ../nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 \
          ${workload_name[$workload]} disableLogChecker=1 FORCE_LEARN=1 MALLOC_SIZE=${HEAP_SIZE} \
          tid0Slowdown=0 $sol=1 NB_THREADS=$t PINNING=$PINNING \
          DISABLE_LOG_REPLAY $log_sol \
          PROFILE_FILE="./$FOLDER/prof_${sol}-${workload}_s${s}.tsv" \
          ERROR_FILE="./$FOLDER/error_${sol}-${workload}_s${s}.tsv" \
          LOG_REPLAY_STATS_FILE="./$FOLDER/log_${sol}-${workload}_s${s}.tsv" \
          | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n \
          | tail -n 1 >> $FOLDER/${sol}-${workload}_s${s}.tsv
        sleep 0.05s
        pkill test_spins
      done
    done
  done
done


# for s in $(seq $SAMPLES)
# do
# #usePCWC-NF usePCWC-F usePCWM useLogicalClocks usePhysicalClocks
# #usePHTM useLogicalClocks usePhysicalClocks usePCWM usePCWM2
# # useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM
#   for sol in usePSTM
#   do
#     for workload in 4W4R-PRI
#     do
#       echo -e "THREADS\tTHROUGHPUT\tTIME\tCOMMITS\tABORTS" \
#         > $FOLDER/${sol}-${workload}_s${s}.tsv
#       for t in  1 2 4 6 8 12 16 24 32 33 40 48 56 64
#       do
#         echo "t=$t"
#         # HEAP_SIZE=$(($MALLOC_SIZE * $t))
#         HEAP_SIZE=$MALLOC_SIZE
#         ./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 \
#           ${workload_name[$workload]} disableLogChecker=1 FORCE_LEARN=1 MALLOC_SIZE=${HEAP_SIZE} \
#           tid0Slowdown=0 $sol=1 NB_THREADS=$t PINNING=$PINNING \
#           DISABLE_LOG_REPLAY $log_sol \
#           PROFILE_FILE="./$FOLDER/prof_${sol}-${workload}_s${s}.tsv" \
#           ERROR_FILE="./$FOLDER/error_${sol}-${workload}_s${s}.tsv" \
#           LOG_REPLAY_STATS_FILE="./$FOLDER/log_${sol}-${workload}_s${s}.tsv" \
#           | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*$/p' -n \
#           | tail -n 1 >> $FOLDER/${sol}-${workload}_s${s}.tsv
#         sleep 0.05s
#       done
#     done
#   done
# done

# cd bench/stamp
# ./bench.sh
