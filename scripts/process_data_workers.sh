source ../paths.sh


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

cd $SCRIPT_PATH
EXPERIMENT_FOLDER="data_mini_bench_workers"


# mkdir /home/vagrant/saved-data/
# scp node05:~/projs/test_nvhtm_wait_phase/data_mini_bench_workers/* /home/vagrant/saved-data
# cp /home/vagrant/saved-data/* data/

### TODO: this is copy from mini_bench
# useEpochCommit2 --> is too bad
# usePhysicalClocks useLogicalClocks useEpochCommit1
# usePCWC useFastPCWC
#usePCWC-F usePCWC-NF usePCWC-F-DA usePCWC-NF-DA usePCWC-F-DSN usePCWC-NF-DSN usePCWC-F-DTX usePCWC-NF-DTX usePCWC-F-DWC usePCWC-NF-DWC

yourfilenames=`ls $EXPERIMENT_FOLDER`
echo $yourfilenames
for eachfile in $yourfilenames
do
  cd $EXPERIMENT_FOLDER
  cd $eachfile
# for sol in useSharedHTMUndo useSharedHTM useHTM useHTMShared useCcHTMbest useLogicalClocks usePhysicalClocks usePCWM usePCWM2 usePCWM3 usePSTM useCrafty
# for sol in usePCWMeADR usePCWM useSharedHTMUndo useHTM
# for sol in usePCWMeADR usePCWM useSharedHTMUndo
  for sol in usePCWM #useHTMUndo usePCWMeADRT1 usePCWMeADR 
  do
    for workload in 0W5R-PRI 0W1R-PRI 1W1R-PRI #2W5R-PRI #5W5R-PRI 25W25R-PRI 1W9R-PRI 9W1R-PRI 5W45R-PRI
    do
      $SCRIPT_AVG $(ls data/${sol}-${workload}_s*.tsv)
      mv avg.txt ${sol}-${workload}_avg.tsv
      mv stdev.txt ${sol}-${workload}_stdev.tsv
  #    $SCRIPT_AVG $(ls data/prof_${sol}-${workload}_s*.tsv)
  #    mv avg.txt prof_${sol}-${workload}_avg.tsv
  #    mv stdev.txt prof_${sol}-${workload}_stdev.tsv
      # $SCRIPT_AVG $(ls data/log_${sol}-${workload}_s*.tsv)
      # mv avg.txt log_${sol}-${workload}_avg.tsv
      # mv stdev.txt log_${sol}-${workload}_stdev.tsv
    done
  done


  # tar -Jcf data.tar.xz ./data
  # rm -r ./data

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

  $SCRIPT_THROUGHPUT_PLOT "0W5R"  $(ls usePCWM-0W5R*)
  $SCRIPT_THROUGHPUT_PLOT "0W1R" $(ls usePCWM-0W1R*)
  $SCRIPT_THROUGHPUT_PLOT "1W1R" $(ls usePCWM-1W1R*)
  #$SCRIPT_THROUGHPUT_PLOT "2W5R" $(ls usePCWM-2W5R*)

  # $SCRIPT_THROUGHPUT_PLOT "45W5R"  $(ls useHTMUndo-5W5R*  usePCWMeADRT1-5W5R*  usePCWMeADR-5W5R*  usePCWM-5W5R*)
  # $SCRIPT_THROUGHPUT_PLOT "25W25R" $(ls useHTMUndo-25W25R*  usePCWMeADRT1-25W25R*  usePCWMeADR-25W25R*  usePCWM-25W25R*)
  # $SCRIPT_THROUGHPUT_PLOT "1W9R" $(ls useHTMUndo-1W9R*  usePCWMeADRT1-1W9R*  usePCWMeADR-1W9R*  usePCWM-1W9R*)
  # $SCRIPT_THROUGHPUT_PLOT "9W1R" $(ls useHTMUndo-9W1R*  usePCWMeADRT1-9W1R*  usePCWMeADR-9W1R*  usePCWM-9W1R*)
  # $SCRIPT_THROUGHPUT_PLOT "5W45R" $(ls useHTMUndo-5W45R*  usePCWMeADRT1-5W45R*  usePCWMeADR-5W45R*  usePCWM-5W45R*)

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

  # $SCRIPT_ABORT_PLOT "4W4R"  $(ls usePCWMeADRT1-4W4R*  usePCWMeADRT2-4W4R*  usePCWMeADR-4W4R*  usePCWM-4W4R*)
  # $SCRIPT_ABORT_PLOT "5W50R" $(ls usePCWMeADRT1-5W50R* usePCWMeADRT2-5W50R* usePCWMeADR-5W50R* usePCWM-5W50R*)
  # $SCRIPT_ABORT_PLOT "50W5R" $(ls usePCWMeADRT1-50W5R* usePCWMeADRT2-50W5R* usePCWMeADR-50W5R* usePCWM-50W5R*)

  $SCRIPT_ABORT_PLOT "0W5R"  $(ls usePCWM-0W5R*)
  $SCRIPT_ABORT_PLOT "0W1R" $(ls usePCWM-0W1R*)
  $SCRIPT_ABORT_PLOT "1W1R" $(ls usePCWM-1W1R*)
  # $SCRIPT_ABORT_PLOT "2W5R" $(ls usePCWM-2W5R*)

  # $SCRIPT_ABORT_PLOT "45W5R"  $(ls useHTMUndo-5W5R*  usePCWMeADRT1-5W5R*  usePCWMeADR-5W5R*  usePCWM-5W5R*)
  # $SCRIPT_ABORT_PLOT "25W25R" $(ls useHTMUndo-25W25R*  usePCWMeADRT1-25W25R*  usePCWMeADR-25W25R*  usePCWM-25W25R*)
  # $SCRIPT_ABORT_PLOT "1W9R" $(ls useHTMUndo-1W9R*  usePCWMeADRT1-1W9R*  usePCWMeADR-1W9R*  usePCWM-1W9R*)
  # $SCRIPT_ABORT_PLOT "9W1R" $(ls useHTMUndo-9W1R*  usePCWMeADRT1-9W1R*  usePCWMeADR-9W1R*  usePCWM-9W1R*)
  # $SCRIPT_ABORT_PLOT "5W45R" $(ls useHTMUndo-5W45R*  usePCWMeADRT1-5W45R*  usePCWMeADR-5W45R*  usePCWM-5W45R*)

  # $SCRIPT_ABORT_TYPE_PLOT "4W4R"  $(ls usePCWMeADRT1-4W4R*  usePCWMeADRT2-4W4R* usePCWMeADR-4W4R*  usePCWM-4W4R*)
  # $SCRIPT_ABORT_TYPE_PLOT "5W50R" $(ls usePCWMeADRT1-5W50R* usePCWMeADRT2-5W50R* usePCWMeADR-5W50R* usePCWM-5W50R*)
  # $SCRIPT_ABORT_TYPE_PLOT "50W5R" $(ls usePCWMeADRT1-50W5R* usePCWMeADRT2-50W5R* usePCWMeADR-50W5R* usePCWM-50W5R*)

  $SCRIPT_ABORT_TYPE_PLOT "0W5R"  $(ls usePCWM-0W5R*)
  $SCRIPT_ABORT_TYPE_PLOT "0W1R" $(ls usePCWM-0W1R*)
  $SCRIPT_ABORT_TYPE_PLOT "1W1R" $(ls usePCWM-1W1R*)
  #$SCRIPT_ABORT_TYPE_PLOT "2W5R" $(ls usePCWM-2W5R*)

  # $SCRIPT_ABORT_TYPE_PLOT "45W5R"  $(ls useHTMUndo-5W5R*  usePCWMeADRT1-5W5R*  usePCWMeADR-5W5R*  usePCWM-5W5R*)
  # $SCRIPT_ABORT_TYPE_PLOT "25W25R" $(ls useHTMUndo-25W25R*  usePCWMeADRT1-25W25R*  usePCWMeADR-25W25R*  usePCWM-25W25R*)
  # $SCRIPT_ABORT_TYPE_PLOT "1W9R" $(ls useHTMUndo-1W9R*  usePCWMeADRT1-1W9R*  usePCWMeADR-1W9R*  usePCWM-1W9R*)
  # $SCRIPT_ABORT_TYPE_PLOT "9W1R" $(ls useHTMUndo-9W1R*  usePCWMeADRT1-9W1R*  usePCWMeADR-9W1R*  usePCWM-9W1R*)
  # $SCRIPT_ABORT_TYPE_PLOT "5W45R" $(ls useHTMUndo-5W45R*  usePCWMeADRT1-5W45R*  usePCWMeADR-5W45R*  usePCWM-5W45R*)



  # $SCRIPT_PROF_PLOT "2W_4R_(SHARED)" $(ls prof_use*2W4R-SHA*)
  # $SCRIPT_PROF_PLOT "2W_64R_(SHARED)" $(ls prof_use*2W64R-SHA*)
  # $SCRIPT_PROF_PLOT "4W_4R_(PRIVATE)" $(ls prof_use*4W4R-PRI*)
  # $SCRIPT_PROF_PLOT "4W4R-PCWM" $(ls prof_usePCWM*)
  # $SCRIPT_PROF_PLOT "4W4R-OTHER" $(ls prof_usePCWM-* prof_useCc* prof_useLog* prof_usePhy* prof_useCrafty*)

  cd $SCRIPT_PATH
done
