source paths.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

SCRIPT_PATH=$(pwd)
SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput.py
# SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput_v2.py
SCRIPT_ABORT_PLOT=$SCRIPT_PATH/plot_aborts.py
SCRIPT_PROF_PLOT=$SCRIPT_PATH/plot_prof.py
SCRIPT_AVG=$SCRIPT_PATH/SCRIPT_compute_AVG_ERR.R
SCRIPT_LOG_PLOT=$SCRIPT_PATH/plot_log.py
SCRIPT_LOG_PROF_PLOT=$SCRIPT_PATH/plot_prof_log.py
SCRIPT_LOG_BYTES_PLOT=$SCRIPT_PATH/plot_bytes_log.py

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

#usePCWM2-DOPT usePCWM2-WOPT usePCWM-DOPT usePCWM-WOPT
for sol in usePCWM2-PIN1 usePCWM-PIN1 usePCWM2-PIN0 usePCWM-PIN0
do
  $SCRIPT_AVG $(ls data/${sol}_s*.tsv)
  mv avg.txt ${sol}_avg.tsv
  mv stdev.txt ${sol}_stdev.tsv
  $SCRIPT_AVG $(ls data/prof_${sol}_s*.tsv)
  mv avg.txt prof_${sol}_avg.tsv
  mv stdev.txt prof_${sol}_stdev.tsv
done


# for sol in usePCWC-NF usePCWM
# do
#   for log_sol in NORMAL BUFFER-WBINVD BUFFER-FLUSHES
#   do
#     $SCRIPT_AVG $(ls data/${sol}-${log_sol}_s*.tsv)
#     mv avg.txt ${sol}-${log_sol}_avg.tsv
#     mv stdev.txt ${sol}-${log_sol}_stdev.tsv
#     $SCRIPT_AVG $(ls data/prof_${sol}-${log_sol}_s*.tsv)
#     mv avg.txt prof_${sol}-${log_sol}_avg.tsv
#     mv stdev.txt prof_${sol}-${log_sol}_stdev.tsv
#     $SCRIPT_AVG $(ls data/log_${sol}-${log_sol}_s*.tsv)
#     mv avg.txt log_${sol}-${log_sol}_avg.tsv
#     mv stdev.txt log_${sol}-${log_sol}_stdev.tsv
#   done
# done


# for sol in useLogicalClocks usePhysicalClocks usePCWC-F usePCWC-NF usePCWM
# do
#   $SCRIPT_AVG $(ls data/${sol}_s*.tsv)
#   mv avg.txt ${sol}_avg.tsv
#   mv stdev.txt ${sol}_stdev.tsv
#   $SCRIPT_AVG $(ls data/params_${sol}_s*.tsv)
#   mv avg.txt params_${sol}_avg.tsv
#   mv stdev.txt params_${sol}_stdev.tsv
#   $SCRIPT_AVG $(ls data/prof_${sol}_s*.tsv)
#   mv avg.txt prof_${sol}_avg.tsv
#   mv stdev.txt prof_${sol}_stdev.tsv
# done


tar -Jcf data.tar.xz ./data
rm -r ./data
echo "data is in $PWD"

# for lat in 500
# do
#   $SCRIPT_THROUGHPUT_PLOT "lat${lat}ns" $(ls ${lat}_*)
#   $SCRIPT_ABORT_PLOT "lat${lat}ns" $(ls ${lat}_*)
#   $SCRIPT_PROF_PLOT "lat${lat}ns" $(ls prof_${lat}_*)
# done

#################

$SCRIPT_THROUGHPUT_PLOT "all" $(ls use*)
$SCRIPT_ABORT_PLOT "all" $(ls use*)
$SCRIPT_PROF_PLOT "all" $(ls prof_use*)


cd $SCRIPT_PATH
