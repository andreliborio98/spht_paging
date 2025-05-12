source paths.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

SCRIPT_PATH=$(pwd)
SCRIPT_AVG=$SCRIPT_PATH/SCRIPT_compute_AVG_ERR.R

# SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput_v2.py
SCRIPT_LOG_PROF_WBINVD=$SCRIPT_PATH/plot_prof_wbinvd.py

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


for sol in usePhysicalClocks 
do
  # BACKWARD NORMAL  BUFFER-FLUSHES
  for log_sol in BUFFER-WBINVD RANGE
  do
    for log_sorter in SYNC-SORTER
    do
      $SCRIPT_AVG $(ls data/${sol}-${log_sol}-${log_sorter}_s*.tsv)
      mv avg.txt ${sol}-${log_sol}-${log_sorter}_avg.tsv
      mv stdev.txt ${sol}-${log_sol}-${log_sorter}_stdev.tsv
      $SCRIPT_AVG $(ls data/prof_${sol}-${log_sol}-${log_sorter}_s*.tsv)
      mv avg.txt prof_${sol}-${log_sol}-${log_sorter}_avg.tsv
      mv stdev.txt prof_${sol}-${log_sol}-${log_sorter}_stdev.tsv
      $SCRIPT_AVG $(ls data/log_${sol}-${log_sol}-${log_sorter}_s*.tsv)
      mv avg.txt log_${sol}-${log_sol}-${log_sorter}_avg.tsv
      mv stdev.txt log_${sol}-${log_sol}-${log_sorter}_stdev.tsv

      cp data/param_${sol}-${log_sol}-${log_sorter}_s1.tsv param_${sol}-${log_sol}-${log_sorter}_avg.tsv
    done
  done
done

tar -Jcf data.tar.xz ./data
rm -r ./data
echo "data is in $PWD"

$SCRIPT_LOG_PROF_WBINVD "test" $(ls use*)

cd $SCRIPT_PATH
