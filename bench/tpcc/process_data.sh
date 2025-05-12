source ../../paths.sh
source benches_args.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

SCRIPT_PATH=$(pwd)
SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput.py
SCRIPT_THROUGHPUT_MULTIPARAM_PLOT=$SCRIPT_PATH/plot_throughput_multiparams.py
SCRIPT_THROUGHPUT_MASHUP_PLOT=$SCRIPT_PATH/plot_throughput_mashup.py
SCRIPT_ABORT_PLOT=$SCRIPT_PATH/plot_aborts.py
SCRIPT_PROF_PLOT=$SCRIPT_PATH/plot_prof.py
SCRIPT_AVG=$SCRIPT_PATH/../../scripts/SCRIPT_compute_AVG_ERR.R

# cd $DATA_PATH
#mkdir -p tpcc #UNCOMMENT
for d in $(ls -d data*)
do
  EXPERIMENT_FOLDER=$d #"2022-11-01T08_11_55" #$(date +%Y-%m-%dT%H_%m_%S)
  #mkdir -p $EXPERIMENT_FOLDER
  echo $EXPERIMENT_FOLDER
  warehouses=$(echo "$EXPERIMENT_FOLDER" | grep -o -E '[0-9]+$')
  warehousesFolder="mashupStats${warehouses}"
  mkdir -p "$warehousesFolder"
  cd $EXPERIMENT_FOLDER
  #mkdir -p data #UNCOMMENT

  #scp $NODE:~/$DM/bench/tpcc/data/* data/

  # scp $NODE:$DM/bench/tpcc/data/* data/ #UNCOMMENT
  # cp /home/andre/Code/SPHT-results/tpcc/* data/ #UNCOMMENT

  #cp /home/vagrant/saved-data/* data/

  # mkdir /home/vagrant/saved-data/
  # scp node05:~/projs/test_nvhtm_wait_phase/bench/tpcc/data/* /home/vagrant/saved-data
  # cp /home/vagrant/saved-data/* data/

  ### TODO: this is copy from mini_bench
  # useEpochCommit2 --> is too bad
  # usePhysicalClocks useLogicalClocks useEpochCommit1
  # usePCWC useFastPCWC
  #usePCWC-F usePCWC-NF usePCWC-F-DA usePCWC-NF-DA usePCWC-F-DSN usePCWC-NF-DSN usePCWC-F-DTX usePCWC-NF-DTX usePCWC-F-DWC usePCWC-NF-DWC
  #for sol in usePCWMeADRT1 usePCWMeADRT2 usePCWMeADR usePCWM
  for sol in usePCWM useHTM #useSharedHTM usePCWMeADRT1 usePCWMeADR usePCWM
  do
    sol2=$(tr -dc '[:upper:]\n\r' <<< $sol)
    sol3=$(sed -e 's/CWM//g' <<< "${sol2}")
    sol4=$(sed -e 's/TM//g' <<< "${sol3}")
    for i in $(seq $NB_BENCHES)
    do
      $SCRIPT_AVG $(ls data/${test_name[$i]}_${sol}_s*.tsv)
      echo "data/${test_name[$i]}_${sol}_s*.tsv"

      mv avg.txt ${test_name[$i]}_${sol}_avg.tsv
      mv stdev.txt ${test_name[$i]}_${sol}_stdev.tsv

      echo "${test_name[$i]}_${sol}_avg.tsv"

      cp ${test_name[$i]}_${sol}_avg.tsv ../${warehousesFolder}
      cp ${test_name[$i]}_${sol}_stdev.tsv ../${warehousesFolder}
      mv ../${warehousesFolder}/${test_name[$i]}_${sol}_avg.tsv ../${warehousesFolder}/${test_name[$i]}${EXPERIMENT_FOLDER}_${sol4}_avg.tsv
      mv ../${warehousesFolder}/${test_name[$i]}_${sol}_stdev.tsv ../${warehousesFolder}/${test_name[$i]}${EXPERIMENT_FOLDER}_${sol4}_stdev.tsv
      # $SCRIPT_AVG $(ls data/prof${test_name[$i]}_${sol}_s*.tsv)
      # mv avg.txt prof${test_name[$i]}_${sol}_avg.tsv
      # mv stdev.txt prof${test_name[$i]}_${sol}_stdev.tsv
    done
  done

  # tar -Jcf data.tar.xz ./data
  # rm -r ./data
  echo "data is in $PWD"

  for i in $(seq $NB_BENCHES)
  do
    $SCRIPT_THROUGHPUT_PLOT "${test_name[$i]}" $(ls ${test_name[$i]}_*)
    $SCRIPT_ABORT_PLOT "${test_name[$i]}" $(ls ${test_name[$i]}_*)
    $SCRIPT_PROF_PLOT "${test_name[$i]}" $(ls prof${test_name[$i]}_*)
  done
  $SCRIPT_THROUGHPUT_MULTIPARAM_PLOT "${test_name[$i]}" $(ls "TPCC_"*)
  cd $SCRIPT_PATH
done

$SCRIPT_THROUGHPUT_MASHUP_PLOT 32 "mashup" $(ls "mashupStats32/TPCC"*)
$SCRIPT_THROUGHPUT_MASHUP_PLOT 64 "mashup" $(ls "mashupStats64/TPCC"*)
# mv mashupStats dataMashup
# plot_throughput_mashup.py "mashup" $(ls "mashupStats/data"*)