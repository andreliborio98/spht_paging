#!/bin/bash

EXPERIMENT_TIME=5000000
PINNING=1
SAMPLES=5

#TOTAL_MEMORY=171798691840
#TOTAL_MEMORY=16777216

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
  ["0W1R-PRI"]="NB_READS=1   NB_WRITES=0" \
  ["0W5R-PRI"]="NB_READS=5   NB_WRITES=0" \
  ["1W1R-PRI"]="NB_READS=1   NB_WRITES=1" \

  )

for zipf_alpha in 200 #70 80 90 #true value = zipf_alpha/100
do
	zipf_alpha_true=`echo "scale=1; $zipf_alpha/100" | bc -l`
	for tot_mem in 1048576 #16777216 33554432 67108864 134217728
	do
		for perc_work_set in 85 90 95 99 #50 85 90 95 99 #works with percentage of tot_mem
		do
		  mkdir -p "$FOLDER/${tot_mem}_${perc_work_set}_${zipf_alpha}/data"
		  FOLDER="${FOLDER}/${tot_mem}_${perc_work_set}_${zipf_alpha}/data"

		  for s in $(seq $SAMPLES)
		  do
		    # for sol in useCrafty usePSTM usePCWM2 usePCWM3 useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM
		    # for sol in usePCWMeADR useSharedHTMUndo useSharedHTM useHTM useHTMShared useCrafty usePSTM usePCWM2 usePCWM3 useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM
		    # for sol in usePCWMeADR usePCWM useSharedHTMUndo useSharedHTM useHTM
		    for sol in usePCWM #useHTMUndo usePCWMeADRT1 usePCWMeADR  #HTMonly PCWMeADRT5 #plot lines - usePCWM = SPHT
		    do
		      for workload in 0W1R-PRI 0W5R-PRI 1W1R-PRI #5W5R-PRI 25W25R-PRI 1W9R-PRI 9W1R-PRI 5W45R-PRI #4W4R-PRI 5W50R-PRI 50W5R-PRI
		      do
		        echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY\tHTM_EXPLICIT\tHTM_OTHER" \
		          > $FOLDER/${sol}-${workload}_s${s}.tsv
		        # for t in 1 2 4 6 8 12 16 20 24 28 32 36 40 48 56 64
		        for t in 1 2 4 8 16 24 #32 48 #6 8 10 12 16 20 24 32 40 48
		        do
		          # tot_work_set=`echo "($work_set * 0.01 * $tot_mem)" | bc`
		          # tot_work_set=`printf "%.0f" $tot_work_set`

		          echo "zipf_alpha=$zipf_alpha_true t=$t tot_mem=$tot_mem perc_work_set=$perc_work_set samples=$s workload=$workload"

	            ../nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 \
	            ${workload_name[$workload]} disableLogChecker=1 FORCE_LEARN=1 tid0Slowdown=0 \
	            $sol=1 NB_THREADS=$t PINNING=$PINNING TOTAL_MEMORY=${tot_mem} PERC_SIZE_WORKING_SET=${perc_work_set} \
	            DISABLE_LOG_REPLAY $log_sol ZIPF_ALPHA=$zipf_alpha ADD_PAGE_THRESHOLD=90 RM_PAGE_THRESHOLD=85\
	            PROFILE_FILE="./$FOLDER/prof_${sol}-${workload}_s${s}.tsv" \
	            ERROR_FILE="./$FOLDER/error_${sol}-${workload}_s${s}.tsv" \
	            LOG_REPLAY_STATS_FILE="./$FOLDER/log_${sol}-${workload}_s${s}.tsv" \
	            | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n \
	            | tail -n 1 >> $FOLDER/${sol}-${workload}_s${s}.tsv #&& echo ${PIPESTATUS[0]

		    #       if [ "${PIPESTATUS[0]}" != "0" ]; then
	    	# 	    echo '\tExecution Failed'
				  # fi
		          sleep 0.05s
		          pkill test_spins
		        done
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
