source paths.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

SCRIPT_PATH=$(pwd)
SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput_workers.py
# SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput_v2.py
SCRIPT_ABORT_PLOT=$SCRIPT_PATH/plot_aborts_workers.py
SCRIPT_PROF_PLOT=$SCRIPT_PATH/plot_prof_workers.py
SCRIPT_AVG=$SCRIPT_PATH/SCRIPT_compute_AVG_ERR.R
SCRIPT_LOG_PLOT=$SCRIPT_PATH/plot_log.py
SCRIPT_LOG_PROF_PLOT=$SCRIPT_PATH/plot_prof_log.py
SCRIPT_LOG_BYTES_PLOT=$SCRIPT_PATH/plot_bytes_log.py
SCRIPT_COL=$SCRIPT_PATH/SCRIPT_put_column.R

FOLDER=data_mini_bench_workers

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

# for sol in useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM usePCWM2 usePCWM3 usePSTM useCrafty
for sol in usePSTM useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM usePCWM2 usePCWM3 usePSTM useCrafty
do
  for workload in 1W2R-PRI
  do
    $SCRIPT_AVG $(ls data/${sol}-${workload}_s*.tsv)
    mv avg.txt ${sol}-${workload}_avg.tsv
    mv stdev.txt ${sol}-${workload}_stdev.tsv
    $SCRIPT_AVG $(ls data/prof_${sol}-${workload}_s*.tsv)
    mv avg.txt prof_${sol}-${workload}_avg.tsv
    mv stdev.txt prof_${sol}-${workload}_stdev.tsv
    $SCRIPT_AVG $(ls data/log_${sol}-${workload}_s*.tsv)
    mv avg.txt log_${sol}-${workload}_avg.tsv
    mv stdev.txt log_${sol}-${workload}_stdev.tsv
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

# $SCRIPT_THROUGHPUT_PLOT "2W_4R_(SHARED)" $(ls use*2W4R-SHA*)
# $SCRIPT_THROUGHPUT_PLOT "2W_64R_(SHARED)" $(ls use*2W64R-SHA*)
# $SCRIPT_THROUGHPUT_PLOT "4W_4R_(PRIVATE)" $(ls use*4W4R-PRI*)
$SCRIPT_THROUGHPUT_PLOT "1W2R" $(ls usePCWM* useLog* usePhy* useCc* useCrafty* usePSTM*)

# $SCRIPT_ABORT_PLOT "2W_4R_(SHARED)" $(ls use*2W4R-SHA*)
# $SCRIPT_ABORT_PLOT "2W_64R_(SHARED)" $(ls use*2W64R-SHA*)
# $SCRIPT_ABORT_PLOT "4W_4R_(PRIVATE)" $(ls use*4W4R-PRI*)
$SCRIPT_ABORT_PLOT "1W2R" $(ls usePCWM-* useLog* useCrafty*)

# $SCRIPT_PROF_PLOT "2W_4R_(SHARED)" $(ls prof_use*2W4R-SHA*)
# $SCRIPT_PROF_PLOT "2W_64R_(SHARED)" $(ls prof_use*2W64R-SHA*)
# $SCRIPT_PROF_PLOT "4W_4R_(PRIVATE)" $(ls prof_use*4W4R-PRI*)
$SCRIPT_PROF_PLOT "1W2R-PCWM" $(ls prof_usePCWM*)
$SCRIPT_PROF_PLOT "1W2R-OTHER" $(ls prof_usePCWM-* prof_useCc* prof_useLog* prof_usePhy* prof_useCrafty*)

cd $SCRIPT_PATH
