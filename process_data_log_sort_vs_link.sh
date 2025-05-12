source paths.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

SCRIPT_PATH=$(pwd)
SCRIPT_AVG=$SCRIPT_PATH/SCRIPT_compute_AVG_ERR.R

SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput.py
# SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput_v2.py
SCRIPT_ABORT_PLOT=$SCRIPT_PATH/plot_aborts.py
SCRIPT_PROF_PLOT=$SCRIPT_PATH/plot_prof.py
SCRIPT_LOG_PLOT=$SCRIPT_PATH/plot_log.py
SCRIPT_LOG_REP_PLOT=$SCRIPT_PATH/plot_log_replayers.py
SCRIPT_LOG_REP_PLOT_v2=$SCRIPT_PATH/plot_log_replayers_1M_512M.py
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
scp $NODE:~/$DM/mini_bench/* data/

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

$SCRIPT_COL "THROUGHPUT = ((a\$WRITTEN_ENTRIES - a\$TOTAL_NB_TXS) / 2) / (a\$TIME_TOTAL / 2300000000); a = cbind(a, THROUGHPUT); " \
  $(ls data/log_*PCWM-*.tsv)
$SCRIPT_COL "THROUGHPUT = ((a\$WRITTEN_ENTRIES - (2 * a\$TOTAL_NB_TXS)) / 2) / (a\$TIME_TOTAL / 2300000000); a = cbind(a, THROUGHPUT); " \
  $(ls data/log_*PCWM3*.tsv)
$SCRIPT_COL "THROUGHPUT = ((a\$WRITTEN_ENTRIES - (2 * a\$TOTAL_NB_TXS)) / 2) / (a\$TIME_TOTAL / 2300000000); a = cbind(a, THROUGHPUT); " \
  $(ls data/log_*PCWM2*.tsv)

# for sol in usePCWM
# do
#   # BACKWARD NORMAL  BUFFER-FLUSHES
#     for log_sol in BUFFER-FLUSHES BACKWARD ST-CLWB
#     do
#       $SCRIPT_AVG $(ls data/${sol}-${log_sol}-LOGS64_s*.tsv)
#       mv avg.txt ${sol}-${log_sol}-LOGS64_avg.tsv
#       mv stdev.txt ${sol}-${log_sol}-LOGS64_stdev.tsv
#       $SCRIPT_AVG $(ls data/prof_${sol}-${log_sol}-LOGS64_s*.tsv)
#       mv avg.txt prof_${sol}-${log_sol}-LOGS64_avg.tsv
#       mv stdev.txt prof_${sol}-${log_sol}-LOGS64_stdev.tsv
#       $SCRIPT_AVG $(ls data/log_${sol}-${log_sol}-LOGS64_s*.tsv.cols)
#       mv avg.txt log_${sol}-${log_sol}-LOGS64_avg.tsv
#       mv stdev.txt log_${sol}-${log_sol}-LOGS64_stdev.tsv
#     done
# done

for sol in usePCWM usePCWM2
do
  # BACKWARD NORMAL  BUFFER-FLUSHES
  for log_sol in BUFFER-WBINVD
  do
    for HEAPSIZE in 2097152 33554432 536870912
    do
      $SCRIPT_AVG $(ls data/${sol}-${log_sol}-LOGS64-HEAP${HEAPSIZE}B_s*.tsv)
      mv avg.txt ${sol}-${log_sol}-LOGS64-HEAP${HEAPSIZE}B_avg.tsv
      mv stdev.txt ${sol}-${log_sol}-LOGS64-HEAP${HEAPSIZE}B_stdev.tsv
      $SCRIPT_AVG $(ls data/prof_${sol}-${log_sol}-LOGS64-HEAP${HEAPSIZE}B_s*.tsv)
      mv avg.txt prof_${sol}-${log_sol}-LOGS64-HEAP${HEAPSIZE}B_avg.tsv
      mv stdev.txt prof_${sol}-${log_sol}-LOGS64-HEAP${HEAPSIZE}B_stdev.tsv
      $SCRIPT_AVG $(ls data/log_${sol}-${log_sol}-LOGS64-HEAP${HEAPSIZE}B_s*.tsv.cols)
      mv avg.txt log_${sol}-${log_sol}-LOGS64-HEAP${HEAPSIZE}B_avg.tsv
      mv stdev.txt log_${sol}-${log_sol}-LOGS64-HEAP${HEAPSIZE}B_stdev.tsv
    done
  done
done

# for sol in usePCWM3
# do
#   # BACKWARD NORMAL  BUFFER-FLUSHES
#     for log_sol in BACKWARD
#     do
#       $SCRIPT_AVG $(ls data/${sol}-${log_sol}-LOGS64_s*.tsv)
#       mv avg.txt ${sol}-${log_sol}-LOGS64_avg.tsv
#       mv stdev.txt ${sol}-${log_sol}-LOGS64_stdev.tsv
#       $SCRIPT_AVG $(ls data/prof_${sol}-${log_sol}-LOGS64_s*.tsv)
#       mv avg.txt prof_${sol}-${log_sol}-LOGS64_avg.tsv
#       mv stdev.txt prof_${sol}-${log_sol}-LOGS64_stdev.tsv
#       $SCRIPT_AVG $(ls data/log_${sol}-${log_sol}-LOGS64_s*.tsv.cols)
#       mv avg.txt log_${sol}-${log_sol}-LOGS64_avg.tsv
#       mv stdev.txt log_${sol}-${log_sol}-LOGS64_stdev.tsv
#     done
# done


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

# $SCRIPT_THROUGHPUT_PLOT "lat${lat}ns-PCWC-F" $(ls ${lat}_usePCWC-F*)
# $SCRIPT_ABORT_PLOT "lat${lat}ns-PCWC-F" $(ls ${lat}_usePCWC-F*)
# $SCRIPT_PROF_PLOT "lat${lat}ns-PCWC-F" $(ls prof_${lat}_usePCWC-F*)

# $SCRIPT_THROUGHPUT_PLOT "lat${lat}ns-PCWC-NF" $(ls ${lat}_usePCWC-NF*)
# $SCRIPT_ABORT_PLOT "lat${lat}ns-PCWC-NF" $(ls ${lat}_usePCWC-NF*)
# $SCRIPT_PROF_PLOT "lat${lat}ns-PCWC-NF" $(ls prof_${lat}_usePCWC-NF*)

# $SCRIPT_THROUGHPUT_PLOT "2W_4R_all_opts" $(ls usePCWC-*-ALL* usePCWM* useLogical* usePhysical*)
# $SCRIPT_THROUGHPUT_PLOT "2W_4R_PCWC-NF" $(ls usePCWC-NF-* usePCWM* useLogical* usePhysical*)
# $SCRIPT_THROUGHPUT_PLOT "2W_4R_PCWC-F" $(ls usePCWC-F-* usePCWM* useLogical* usePhysical*)

# $SCRIPT_ABORT_PLOT "2W_4R_all_opts" $(ls usePCWC-*-ALL* usePCWM* useLogical* usePhysical*)
# $SCRIPT_ABORT_PLOT "2W_4R_PCWC-NF" $(ls usePCWC-NF-*)
# $SCRIPT_ABORT_PLOT "2W_4R_PCWC-F" $(ls usePCWC-F-*)

# $SCRIPT_PROF_PLOT "2W_4R_all_opts" $(ls prof_usePCWC-*-ALL* prof_usePCWM* prof_useLogical* prof_usePhysical*)
# $SCRIPT_PROF_PLOT "2W_4R_PCWC-NF" $(ls prof_usePCWC-NF-*)
# $SCRIPT_PROF_PLOT "2W_4R_PCWC-F" $(ls prof_usePCWC-F-*)

# $SCRIPT_ABORT_PLOT "test_log" $(ls use*)
# $SCRIPT_PROF_PLOT "test_log" $(ls prof_use*)

$SCRIPT_THROUGHPUT_PLOT "test_log" $(ls use*)
$SCRIPT_LOG_REP_PLOT "all" $(ls use*)
$SCRIPT_LOG_REP_PLOT "WBINVD" $(ls use*WBINVD*)
$SCRIPT_LOG_PROF_PLOT "test_log" $(ls log_use*)
$SCRIPT_LOG_BYTES_PLOT "test_log" $(ls log_use*)

# $SCRIPT_LOG_PROF_2 "profiling" $(ls log_usePhysicalClocks-BUFFER-WBINVD-SYNC-SORTER*) \
#   $(ls log_usePhysicalClocks*-FLUSHES-* log_usePhysicalClocks*-BACKWARD-* log_usePhysicalClocks*-RANGE-* \
#   log_usePCWM2*-WBINVD-*)

# $SCRIPT_LOG_PROF_PLOT "all_stacked" $(ls log_usePhysicalClocks-BUFFER-WBINVD-SYNC-SORTER*) \
#   $(ls log_usePhysicalClocks*-FLUSHES-*SYNC* log_usePhysicalClocks*-BACKWARD-*SYNC*)

cd $SCRIPT_PATH
