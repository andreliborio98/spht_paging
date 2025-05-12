source paths.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

SCRIPT_PATH=$(pwd)
SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput_workers.py
# SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput_v2.py
SCRIPT_ABORT_PLOT=$SCRIPT_PATH/plot_aborts_workers.py
SCRIPT_ABORT_TYPE_PLOT=$SCRIPT_PATH/plot_abort_types_workers.py
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
# scp $NODE:~/$DM/$FOLDER/* data/
# cp /home/vagrant/saved-data/* data/

# mkdir /home/vagrant/saved-data/
# scp node05:~/projs/test_nvhtm_wait_phase/data_mini_bench_workers/* /home/vagrant/saved-data
# cp /home/vagrant/saved-data/* data/

mkdir /home/vagrant/node05-160gb-pm-swap/
scp node05:~/projs/test_nvhtm_wait_phase/data_mini_bench_workers/* /home/vagrant/node05-160gb-pm-swap
cp /home/vagrant/node05-160gb-pm-swap/* data/

### TODO: this is copy from mini_bench
# useEpochCommit2 --> is too bad
# usePhysicalClocks useLogicalClocks useEpochCommit1
# usePCWC useFastPCWC
#usePCWC-F usePCWC-NF usePCWC-F-DA usePCWC-NF-DA usePCWC-F-DSN usePCWC-NF-DSN usePCWC-F-DTX usePCWC-NF-DTX usePCWC-F-DWC usePCWC-NF-DWC

# for sol in useSharedHTMUndo useSharedHTM useHTM useHTMShared useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM usePCWM2 usePCWM3 usePSTM useCrafty
# for sol in usePCWMeADR usePCWM useSharedHTMUndo useHTM
# for sol in usePCWMeADR usePCWM useSharedHTMUndo
for sol in usePCWMeADRT1 usePCWMeADRT2 usePCWMeADR usePCWM
do
  for workload in 4W4R-PRI 5W50R-PRI 50W5R-PRI
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
# $SCRIPT_THROUGHPUT_PLOT "4W4R" $(ls usePCWM* useLog* usePhy* useCc* useCrafty* usePSTM*)
# $SCRIPT_THROUGHPUT_PLOT "4W4R" $(ls usePCWMeADR-4W4R* usePCWM-4W4R* useSharedHTMUndo-4W4R*)
# $SCRIPT_THROUGHPUT_PLOT "5W50R" $(ls usePCWMeADR-5W50R* usePCWM-5W50R* useSharedHTMUndo-5W50R*)
# $SCRIPT_THROUGHPUT_PLOT "50W5R" $(ls usePCWMeADR-50W5R* usePCWM-50W5R* useSharedHTMUndo-50W5R*)
# $SCRIPT_THROUGHPUT_PLOT "4W4R" $(ls usePCWMeADRT1-4W4R* usePCWMeADR-4W4R* usePCWM-4W4R*)
# $SCRIPT_THROUGHPUT_PLOT "5W50R" $(ls usePCWMeADRT1-5W50R* usePCWMeADR-5W50R* usePCWM-5W50R*)
# $SCRIPT_THROUGHPUT_PLOT "50W5R" $(ls usePCWMeADRT1-50W5R* usePCWMeADR-50W5R* usePCWM-50W5R*)
# $SCRIPT_THROUGHPUT_PLOT "4W4R" $(ls usePCWMeADRT1-4W4R* usePCWMeADRT2-4W4R* usePCWMeADRT3-4W4R* usePCWMeADRT4-4W4R* usePCWMeADRT5-4W4R* usePCWMeADR-4W4R* usePCWM-4W4R*)
# $SCRIPT_THROUGHPUT_PLOT "5W50R" $(ls usePCWMeADRT1-5W50R* usePCWMeADRT2-5W50R* usePCWMeADRT3-5W50R* usePCWMeADRT4-5W50R* usePCWMeADRT5-5W50R* usePCWMeADR-5W50R* usePCWM-5W50R*)
# $SCRIPT_THROUGHPUT_PLOT "50W5R" $(ls usePCWMeADRT1-50W5R* usePCWMeADRT2-50W5R* usePCWMeADRT3-50W5R* usePCWMeADRT4-50W5R* usePCWMeADRT5-50W5R* usePCWMeADR-50W5R* usePCWM-50W5R*)

$SCRIPT_THROUGHPUT_PLOT "4W4R"  $(ls usePCWMeADRT1-4W4R*  usePCWMeADRT2-4W4R*  usePCWMeADR-4W4R*  usePCWM-4W4R*)
$SCRIPT_THROUGHPUT_PLOT "5W50R" $(ls usePCWMeADRT1-5W50R* usePCWMeADRT2-5W50R* usePCWMeADR-5W50R* usePCWM-5W50R*)
$SCRIPT_THROUGHPUT_PLOT "50W5R" $(ls usePCWMeADRT1-50W5R* usePCWMeADRT2-50W5R* usePCWMeADR-50W5R* usePCWM-50W5R*)

# $SCRIPT_ABORT_PLOT "2W_4R_(SHARED)" $(ls use*2W4R-SHA*)
# $SCRIPT_ABORT_PLOT "2W_64R_(SHARED)" $(ls use*2W64R-SHA*)
# $SCRIPT_ABORT_PLOT "4W_4R_(PRIVATE)" $(ls use*4W4R-PRI*)
# $SCRIPT_ABORT_PLOT "4W4R" $(ls usePCWM-* useLog* useCrafty*)
# $SCRIPT_ABORT_PLOT "4W4R" $(ls usePCWMeADR-4W4R* usePCWM-4W4R* useSharedHTMUndo-4W4R*)
# $SCRIPT_ABORT_PLOT "5W50R" $(ls usePCWMeADR-5W50R* usePCWM-5W50R* useSharedHTMUndo-5W50R*)
# $SCRIPT_ABORT_PLOT "50W5R" $(ls usePCWMeADR-50W5R* usePCWM-50W5R* useSharedHTMUndo-50W5R*)
# $SCRIPT_ABORT_PLOT "4W4R" $(ls usePCWMeADRT1-4W4R* usePCWMeADR-4W4R* usePCWM-4W4R*)
# $SCRIPT_ABORT_PLOT "5W50R" $(ls usePCWMeADRT1-5W50R* usePCWMeADR-5W50R* usePCWM-5W50R*)
# $SCRIPT_ABORT_PLOT "50W5R" $(ls usePCWMeADRT1-50W5R* usePCWMeADR-50W5R* usePCWM-50W5R*)
# $SCRIPT_ABORT_PLOT "4W4R" $(ls usePCWMeADRT1-4W4R* usePCWMeADRT2-4W4R* usePCWMeADRT3-4W4R* usePCWMeADRT4-4W4R* usePCWMeADRT5-4W4R* usePCWMeADR-4W4R* usePCWM-4W4R*)
# $SCRIPT_ABORT_PLOT "5W50R" $(ls usePCWMeADRT1-5W50R* usePCWMeADRT2-5W50R* usePCWMeADRT3-5W50R* usePCWMeADRT4-5W50R* usePCWMeADRT5-5W50R* usePCWMeADR-5W50R* usePCWM-5W50R*)
# $SCRIPT_ABORT_PLOT "50W5R" $(ls usePCWMeADRT1-50W5R* usePCWMeADRT2-50W5R* usePCWMeADRT3-50W5R* usePCWMeADRT4-50W5R* usePCWMeADRT5-50W5R* usePCWMeADR-50W5R* usePCWM-50W5R*)

$SCRIPT_ABORT_PLOT "4W4R"  $(ls usePCWMeADRT1-4W4R*  usePCWMeADRT2-4W4R*  usePCWMeADR-4W4R*  usePCWM-4W4R*)
$SCRIPT_ABORT_PLOT "5W50R" $(ls usePCWMeADRT1-5W50R* usePCWMeADRT2-5W50R* usePCWMeADR-5W50R* usePCWM-5W50R*)
$SCRIPT_ABORT_PLOT "50W5R" $(ls usePCWMeADRT1-50W5R* usePCWMeADRT2-50W5R* usePCWMeADR-50W5R* usePCWM-50W5R*)

$SCRIPT_ABORT_TYPE_PLOT "4W4R"  $(ls usePCWMeADRT1-4W4R*  usePCWMeADRT2-4W4R* usePCWMeADR-4W4R*  usePCWM-4W4R*)
$SCRIPT_ABORT_TYPE_PLOT "5W50R" $(ls usePCWMeADRT1-5W50R* usePCWMeADRT2-5W50R* usePCWMeADR-5W50R* usePCWM-5W50R*)
$SCRIPT_ABORT_TYPE_PLOT "50W5R" $(ls usePCWMeADRT1-50W5R* usePCWMeADRT2-50W5R* usePCWMeADR-50W5R* usePCWM-50W5R*)

# $SCRIPT_PROF_PLOT "2W_4R_(SHARED)" $(ls prof_use*2W4R-SHA*)
# $SCRIPT_PROF_PLOT "2W_64R_(SHARED)" $(ls prof_use*2W64R-SHA*)
# $SCRIPT_PROF_PLOT "4W_4R_(PRIVATE)" $(ls prof_use*4W4R-PRI*)
# $SCRIPT_PROF_PLOT "4W4R-PCWM" $(ls prof_usePCWM*)
# $SCRIPT_PROF_PLOT "4W4R-OTHER" $(ls prof_usePCWM-* prof_useCc* prof_useLog* prof_usePhy* prof_useCrafty*)

cd $SCRIPT_PATH
