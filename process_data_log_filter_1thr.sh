source paths.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

FOLDER=data_mini_bench_log_filter_1thr

SCRIPT_PATH=$(pwd)
SCRIPT_AVG=$SCRIPT_PATH/SCRIPT_compute_AVG_ERR.R

SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput.py
# SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput_v2.py
SCRIPT_ABORT_PLOT=$SCRIPT_PATH/plot_aborts.py
SCRIPT_PROF_PLOT=$SCRIPT_PATH/plot_prof.py
SCRIPT_LOG_PLOT=$SCRIPT_PATH/plot_log.py
SCRIPT_LOG_REP_PLOT=$SCRIPT_PATH/plot_log_replayers.py
SCRIPT_LOG_REP_1THR_PLOT=$SCRIPT_PATH/plot_log_filter_1thr.py
SCRIPT_LOG_PROF_PLOT=$SCRIPT_PATH/plot_prof_log.py
SCRIPT_LOG_PROF_2=$SCRIPT_PATH/plot_prof_log_2.py
SCRIPT_LOG_BYTES_PLOT=$SCRIPT_PATH/plot_bytes_log.py
SCRIPT_COL=$SCRIPT_PATH/SCRIPT_put_column.R

cd $DATA_PATH
mkdir -p mini_bench
cd mini_bench
EXPERIMENT_FOLDER=$(date +%Y-%m-%dT%H_%m_%S)
mkdir -p $EXPERIMENT_FOLDER
cd $EXPERIMENT_FOLDER
mkdir -p data
scp $NODE:~/$DM/$FOLDER/* data/

MALLOC_SIZE=536870912

$SCRIPT_COL "\
  HEAP_WRITES_RATIO = (((a\$WRITTEN_ENTRIES - (2 * a\$TOTAL_NB_TXS)) / 2) / $MALLOC_SIZE);\
  THROUGHPUT = ((a\$WRITTEN_ENTRIES - (2 * a\$TOTAL_NB_TXS)) / 2) / (a\$TIME_TOTAL / 2300000000);\
  a = cbind(a, HEAP_WRITES_RATIO, THROUGHPUT); " \
  $(ls data/log_*PCWM*.tsv)

for sol in usePCWM2 usePCWM3
do
  for MALLOC_SIZE in 536870912 33554432 1048576
  do
    NB_REPLAYERS=1
  # BACKWARD NORMAL  BUFFER-FLUSHES
    for log_sol in BUFFER-WBINVD
    do
      # for FIX_NUMBER_OF_TXS in $(($MALLOC_SIZE / (16 * 5))) $(($MALLOC_SIZE / (8 * 5) * 5)) 
      $SCRIPT_AVG $(ls data/${sol}-${log_sol}-H${MALLOC_SIZE}B-LOGS64-REP${NB_REPLAYERS}_s*.tsv)
      mv avg.txt ${sol}-${log_sol}-H${MALLOC_SIZE}B-LOGS64-REP${NB_REPLAYERS}_avg.tsv
      mv stdev.txt ${sol}-${log_sol}-H${MALLOC_SIZE}B-LOGS64-REP${NB_REPLAYERS}_stdev.tsv
      $SCRIPT_AVG $(ls data/prof_${sol}-${log_sol}-H${MALLOC_SIZE}B-LOGS64-REP${NB_REPLAYERS}_s*.tsv)
      mv avg.txt prof_${sol}-${log_sol}-H${MALLOC_SIZE}B-LOGS64-REP${NB_REPLAYERS}_avg.tsv
      mv stdev.txt prof_${sol}-${log_sol}-H${MALLOC_SIZE}B-LOGS64-REP${NB_REPLAYERS}_stdev.tsv
      $SCRIPT_AVG $(ls data/log_${sol}-${log_sol}-H${MALLOC_SIZE}B-LOGS64-REP${NB_REPLAYERS}_s*.tsv.cols)
      mv avg.txt log_${sol}-${log_sol}-H${MALLOC_SIZE}B-LOGS64-REP${NB_REPLAYERS}_avg.tsv
      mv stdev.txt log_${sol}-${log_sol}-H${MALLOC_SIZE}B-LOGS64-REP${NB_REPLAYERS}_stdev.tsv
    done
  done
done


# tar -Jcf data.tar.xz ./data
# rm -r ./data
echo " >>>> data is in $PWD"

# for lat in 500
# do
#   $SCRIPT_THROUGHPUT_PLOT "lat${lat}ns" $(ls ${lat}_*)
#   $SCRIPT_ABORT_PLOT "lat${lat}ns" $(ls ${lat}_*)
#   $SCRIPT_PROF_PLOT "lat${lat}ns" $(ls prof_${lat}_*)
# done

#################


$SCRIPT_LOG_REP_1THR_PLOT "all" $(ls use*)

# $SCRIPT_LOG_PROF_2 "profiling" $(ls log_usePhysicalClocks-BUFFER-WBINVD-SYNC-SORTER*) \
#   $(ls log_usePhysicalClocks*-FLUSHES-* log_usePhysicalClocks*-BACKWARD-* log_usePhysicalClocks*-RANGE-* \
#   log_usePCWM2*-WBINVD-*)

# $SCRIPT_LOG_PROF_PLOT "all_stacked" $(ls log_usePhysicalClocks-BUFFER-WBINVD-SYNC-SORTER*) \
#   $(ls log_usePhysicalClocks*-FLUSHES-*SYNC* log_usePhysicalClocks*-BACKWARD-*SYNC*)

cd $SCRIPT_PATH
