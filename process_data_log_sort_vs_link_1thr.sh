source paths.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

FOLDER=data_mini_bench_log_sort_vs_link_1thr
mkdir -p $FOLDER

SCRIPT_PATH=$(pwd)
SCRIPT_AVG=$SCRIPT_PATH/SCRIPT_compute_AVG_ERR.R

SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput.py
# SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput_v2.py
SCRIPT_LOG_REP_PLOT=$SCRIPT_PATH/plot_log_sort_vs_link_1thr.py
SCRIPT_LOG_REP_ABS_PLOT=$SCRIPT_PATH/plot_log_sort_vs_link_1thr_absVal.py
SCRIPT_COL=$SCRIPT_PATH/SCRIPT_put_column.R

cd $DATA_PATH
mkdir -p mini_bench
cd mini_bench
EXPERIMENT_FOLDER=$(date +%Y-%m-%dT%H_%m_%S)
mkdir -p $EXPERIMENT_FOLDER
cd $EXPERIMENT_FOLDER
mkdir -p data
scp $NODE:~/$DM/$FOLDER/* data/

### TODO: this is copy from mini_bench
# useEpochCommit2 --> is too bad
# usePhysicalClocks useLogicalClocks useEpochCommit1
# usePCWC useFastPCWC 
#usePCWC-F usePCWC-NF usePCWC-F-DA usePCWC-NF-DA usePCWC-F-DSN usePCWC-NF-DSN usePCWC-F-DTX usePCWC-NF-DTX usePCWC-F-DWC usePCWC-NF-DWC

# for sol in useLogicalClocks usePhysicalClocks usePCWC-F-ALL usePCWC-F-DTX usePCWC-F-DSN usePCWC-F-DA usePCWC-NF-ALL usePCWC-NF-DTX usePCWM 
# do
#   $SCRIPT_AVG $(ls data/${sol}_s*.tsv)
#   mv avg.txt ${sol}_avg.tsv
#   mv stdev.txt ${sol}_stdev.tsv
#   $SCRIPT_AVG $(ls data/prof_${sol}_s*.tsv)
#   mv avg.txt prof_${sol}_avg.tsv
#   mv stdev.txt prof_${sol}_stdev.tsv
# done


# for sol in usePhysicalClocks 
# do
#   # BACKWARD NORMAL  BUFFER-FLUSHES
#   for log_sol in BUFFER-FLUSHES BACKWARD RANGE
#   do
#     for log_sorter in SYNC-SORTER
#     do
#       $SCRIPT_AVG $(ls data/${sol}-${log_sol}-${log_sorter}_s*.tsv)
#       mv avg.txt ${sol}-${log_sol}-${log_sorter}_avg.tsv
#       mv stdev.txt ${sol}-${log_sol}-${log_sorter}_stdev.tsv
#       $SCRIPT_AVG $(ls data/prof_${sol}-${log_sol}-${log_sorter}_s*.tsv)
#       mv avg.txt prof_${sol}-${log_sol}-${log_sorter}_avg.tsv
#       mv stdev.txt prof_${sol}-${log_sol}-${log_sorter}_stdev.tsv
#       $SCRIPT_AVG $(ls data/log_${sol}-${log_sol}-${log_sorter}_s*.tsv)
#       mv avg.txt log_${sol}-${log_sol}-${log_sorter}_avg.tsv
#       mv stdev.txt log_${sol}-${log_sol}-${log_sorter}_stdev.tsv
#     done
#   done
# done

$SCRIPT_COL "\
  THROUGHPUT = ((a\$WRITTEN_ENTRIES - a\$TOTAL_NB_TXS) / 2) / (a\$TIME_TOTAL / 2300000000); \
  a = cbind(a, THROUGHPUT); " \
  $(ls data/log_*PCWM-*.tsv)
$SCRIPT_COL "\
  THROUGHPUT = ((a\$WRITTEN_ENTRIES - (2 * a\$TOTAL_NB_TXS)) / 2) / (a\$TIME_TOTAL / 2300000000); \
  a = cbind(a, THROUGHPUT); " \
  $(ls data/log_*PCWM3*.tsv)
$SCRIPT_COL "\
  THROUGHPUT = ((a\$WRITTEN_ENTRIES - (2 * a\$TOTAL_NB_TXS)) / 2) / (a\$TIME_TOTAL / 2300000000); \
  a = cbind(a, THROUGHPUT); " \
  $(ls data/log_*PCWM2*.tsv)


for sol in usePCWM usePCWM2
do
  # BACKWARD NORMAL  BUFFER-FLUSHES
  for log_sol in BUFFER-WBINVD
  do
    for NB_REPLAYERS in 1 2 4 8
    do
      for NB_WRITES in 1 5
      do
        cp data/params_${sol}-${log_sol}-LOGS64-REP${NB_REPLAYERS}-W${NB_WRITES}_s1.tsv params_${sol}-${log_sol}-LOGS64-REP${NB_REPLAYERS}-W${NB_WRITES}_avg.tsv
        $SCRIPT_AVG $(ls data/${sol}-${log_sol}-LOGS64-REP${NB_REPLAYERS}-W${NB_WRITES}_s*.tsv)
        mv avg.txt ${sol}-${log_sol}-LOGS64-REP${NB_REPLAYERS}-W${NB_WRITES}_avg.tsv
        mv stdev.txt ${sol}-${log_sol}-LOGS64-REP${NB_REPLAYERS}-W${NB_WRITES}_stdev.tsv
        $SCRIPT_AVG $(ls data/prof_${sol}-${log_sol}-LOGS64-REP${NB_REPLAYERS}-W${NB_WRITES}_s*.tsv)
        mv avg.txt prof_${sol}-${log_sol}-LOGS64-REP${NB_REPLAYERS}-W${NB_WRITES}_avg.tsv
        mv stdev.txt prof_${sol}-${log_sol}-LOGS64-REP${NB_REPLAYERS}-W${NB_WRITES}_stdev.tsv
        $SCRIPT_AVG $(ls data/log_${sol}-${log_sol}-LOGS64-REP${NB_REPLAYERS}-W${NB_WRITES}_s*.tsv.cols)
        mv avg.txt log_${sol}-${log_sol}-LOGS64-REP${NB_REPLAYERS}-W${NB_WRITES}_avg.tsv
        mv stdev.txt log_${sol}-${log_sol}-LOGS64-REP${NB_REPLAYERS}-W${NB_WRITES}_stdev.tsv
      done
    done
  done
done

# tar -Jcf data.tar.xz ./data
# rm -r ./data
echo "data is in $PWD"

# for lat in 500
# do
#   $SCRIPT_THROUGHPUT_PLOT "lat${lat}ns" $(ls ${lat}_*)
#   $SCRIPT_ABORT_PLOT "lat${lat}ns" $(ls ${lat}_*)
#   $SCRIPT_PROF_PLOT "lat${lat}ns" $(ls prof_${lat}_*)
# done

#################

$SCRIPT_LOG_REP_PLOT "all" $(ls use*)
$SCRIPT_LOG_REP_ABS_PLOT "all" $(ls use*)

cd $SCRIPT_PATH
