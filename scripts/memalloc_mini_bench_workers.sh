#!/bin/bash

EXPERIMENT_TIME=500000
PINNING=1
SAMPLES=5

#TOTAL_MEMORY=171798691840
TOTAL_MEMORY=16777216
ADDR_QTT=50000000

FOLDER=data_mini_bench_workers
mkdir -p $FOLDER

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
  ["2W5R-PRI"]="NB_READS=5   NB_WRITES=2" \
  ["1W1R-PRI"]="NB_READS=1   NB_WRITES=1" \
  # with reads-only
  ["0W5R-PRI"]="NB_READS=5   NB_WRITES=0" \
  ["0W9R-PRI"]="NB_READS=9   NB_WRITES=0" \
  ["0W1R-PRI"]="NB_READS=1   NB_WRITES=0" \
  ["0W25R-PRI"]="NB_READS=25   NB_WRITES=0" \
  ["0W45R-PRI"]="NB_READS=45   NB_WRITES=0" \
  )

if [ ! -f /memalloc.x ]
then 
	gcc -O3 memalloc.c -o memalloc.x
fi

for perc_pagefault in 0 20 40 60 80 100
do
  ###for tot_mem in 1048576 16777216 33554432 67108864
  for tot_mem in 1048576
  do
    for work_set in 25 50 75 #works with percentage of tot_mem
    do
      mkdir -p "$FOLDER/${perc_pagefault}_${tot_mem}_${work_set}/data"
      FOLDER="${FOLDER}/${perc_pagefault}_${tot_mem}_${work_set}/data"

      # for sol in useCrafty usePSTM usePCWM2 usePCWM3 useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM
      # for sol in usePCWMeADR useSharedHTMUndo useSharedHTM useHTM useHTMShared useCrafty usePSTM usePCWM2 usePCWM3 useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM
      # for sol in usePCWMeADR usePCWM useSharedHTMUndo useSharedHTM useHTM
      for sol in usePCWM #useHTMUndo usePCWMeADRT1 usePCWMeADR HTMonly PCWMeADRT5 #plot lines - usePCWM = SPHT
      do
        fileWorkloadFlag=0
        fileSamplesFlag=0
        for workload in 0W1R-PRI 0W5R-PRI 1W1R-PRI 2W5R-PRI #1W2R-PRI #0W9R-PRI 0W1R-PRI 0W25R-PRI 0W45R-PRI 5W5R-PRI 25W25R-PRI 1W9R-PRI 9W1R-PRI 5W45R-PRI 4W4R-PRI 5W50R-PRI 50W5R-PRI
        do
          # echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY\tHTM_EXPLICIT\tHTM_OTHER" \
          #   > $FOLDER/${sol}-${workload}_s${s}.tsv
          # for t in 1 2 4 6 8 12 16 20 24 28 32 36 40 48 56 64
          #for t in 1 2 4 6 8 10 12 16 20 24 32 40 48
          for t in 1 2 4 8 12 16 20 24
          do
            #adjusting tot_mem and work_set so mem_alloc.c can work properly with multiple threads
            tot_mem_threads=$((tot_mem / t))
            tot_work_set=`echo "($work_set * 0.01 * $tot_mem)" | bc`
            tot_work_set=`printf "%.0f" $tot_work_set`
            work_set_threads=$((tot_work_set / t))
       
            filename="mem_array_${tot_mem_threads}_${work_set_threads}_${ADDR_QTT}_${perc_pagefault}.c"
            mem_addr_dir="./../nvhtm/bench/${filename}"
            if [ ! -f $mem_addr_dir ]
            then
              echo "Generating memory file $tot_mem_threads $work_set_threads 4096 $ADDR_QTT $perc_pagefault"
              ./memalloc.x $tot_mem_threads $work_set_threads 4096 $ADDR_QTT $perc_pagefault
            fi
            mv "${mem_addr_dir}" "./../nvhtm/bench/mem_addr_array.c"

            #has to compile everytime to incluide the proper mem_addr_array.c
            cd ../nvhtm/
            make clean ; make -j40 OPTIMIZE=1
            cd -  
            for s in $(seq $SAMPLES) #doesnt need to add a header to each thread variation
            do
              if ([ $s -ne $fileSamplesFlag ] || [ $workload != $fileWorkloadFlag ]) && ([ ! -f $FOLDER/${sol}-${workload}_s${s}.tsv ])
              then
                echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY\tHTM_EXPLICIT\tHTM_OTHER" \
                > $FOLDER/${sol}-${workload}_s${s}.tsv
                fileWorkloadFlag=$workload
                fileSamplesFlag=$s
              fi
              echo "perc_pagefault=$perc_pagefault tot_mem=$tot_mem tot_work_set=$tot_work_set samples=$s workload=$workload t=$t"
              ../nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 \
              ${workload_name[$workload]} disableLogChecker=1 FORCE_LEARN=1 tid0Slowdown=0 \
              $sol=1 NB_THREADS=$t PINNING=$PINNING TOTAL_MEMORY=${tot_mem} SIZE_WORKING_SET=${tot_work_set} PERCENT_PAGE_FAULTS=${perc_pagefault} \
              DISABLE_LOG_REPLAY $log_sol \
              PROFILE_FILE="./$FOLDER/prof_${sol}-${workload}_s${s}.tsv" \
              ERROR_FILE="./$FOLDER/error_${sol}-${workload}_s${s}.tsv" \
              LOG_REPLAY_STATS_FILE="./$FOLDER/log_${sol}-${workload}_s${s}.tsv" \
              | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n \
              | tail -n 1 >> $FOLDER/${sol}-${workload}_s${s}.tsv
              sleep 0.05s
              pkill test_spins
            done
            mv "./../nvhtm/bench/mem_addr_array.c" "${mem_addr_dir}"
          done
        done
      done
      FOLDER=data_mini_bench_workers
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
