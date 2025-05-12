source ../paths.sh

SCRIPT_PATH=$(pwd)
SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput_workers.py
SCRIPT_ABORT_PLOT=$SCRIPT_PATH/plot_aborts_workers.py
SCRIPT_ABORT_TYPE_PLOT=$SCRIPT_PATH/plot_abort_types_workers.py
SCRIPT_PROF_PLOT=$SCRIPT_PATH/plot_prof_workers.py
SCRIPT_AVG=$SCRIPT_PATH/SCRIPT_compute_AVG_ERR.R
SCRIPT_LOG_PLOT=$SCRIPT_PATH/plot_log.py
SCRIPT_LOG_PROF_PLOT=$SCRIPT_PATH/plot_prof_log.py
SCRIPT_LOG_BYTES_PLOT=$SCRIPT_PATH/plot_bytes_log.py
SCRIPT_COL=$SCRIPT_PATH/SCRIPT_put_column.R

if [[ $# -gt 0 ]] 
then
	NODE=$1
fi

SCRIPT_PATH=$(pwd)

FOLDER=data_mini_bench_workers

# if [ -d "$FOLDER" ];
# then
#     mv "$FOLDER" "dmbw_$(date +%Y-%m-%dT%H_%m_%S)"
# fi
#scp -r $NODE:$DM/scripts/$FOLDER $FOLDER

yourfilenames=`ls $FOLDER`
for eachfile in $yourfilenames
do
	cd $FOLDER
	cd $eachfile
	for sol in usePCWM #useHTMUndo usePCWMeADRT1 usePCWMeADR usePCWM
	do
	  for workload in 0W1R-PRI 0W5R-PRI 1W1R-PRI 2W5R-PRI #0W5R-PRI 0W25R-PRI 0W9R-PRI 0W1R-PRI 0W45R-PRI #5W5R-PRI 25W25R-PRI 1W9R-PRI 9W1R-PRI 5W45R-PRI
	  do
	    $SCRIPT_AVG $(ls data/${sol}-${workload}_s*.tsv)
	    mv avg.txt ${sol}-${workload}_avg.tsv
	    mv stdev.txt ${sol}-${workload}_stdev.tsv
	    # $SCRIPT_AVG $(ls data/log_${sol}-${workload}_s*.tsv)
	    # mv avg.txt log_${sol}-${workload}_avg.tsv
	    # mv stdev.txt log_${sol}-${workload}_stdev.tsv
	  done
	done
	# $SCRIPT_THROUGHPUT_PLOT "0W5R"  $(ls useHTMUndo-0W5R*  usePCWMeADRT1-0W5R*  usePCWMeADR-0W5R*  usePCWM-0W5R*)
	# $SCRIPT_THROUGHPUT_PLOT "0W25R" $(ls useHTMUndo-0W25R*  usePCWMeADRT1-0W25R*  usePCWMeADR-0W25R*  usePCWM-0W25R*)
	# $SCRIPT_THROUGHPUT_PLOT "0W9R" $(ls useHTMUndo-0W9R*  usePCWMeADRT1-0W9R*  usePCWMeADR-0W9R*  usePCWM-0W9R*)
	# $SCRIPT_THROUGHPUT_PLOT "0W1R" $(ls useHTMUndo-0W1R*  usePCWMeADRT1-0W1R*  usePCWMeADR-0W1R*  usePCWM-0W1R*)
	# $SCRIPT_THROUGHPUT_PLOT "0W45R" $(ls useHTMUndo-0W45R*  usePCWMeADRT1-045R*  usePCWMeADR-0W45R*  usePCWM-0W45R*)

	$SCRIPT_THROUGHPUT_PLOT "0W1R"  $(ls usePCWM-0W1R*)
	$SCRIPT_THROUGHPUT_PLOT "0W5R" $(ls usePCWM-0W5R*)
	$SCRIPT_THROUGHPUT_PLOT "1W1R" $(ls usePCWM-1W1R*)
	$SCRIPT_THROUGHPUT_PLOT "2W5R" $(ls usePCWM-2W5R*)

	# $SCRIPT_THROUGHPUT_PLOT "45W5R"  $(ls useHTMUndo-5W5R*  usePCWMeADRT1-5W5R*  usePCWMeADR-5W5R*  usePCWM-5W5R*)
	# $SCRIPT_THROUGHPUT_PLOT "25W25R" $(ls useHTMUndo-25W25R*  usePCWMeADRT1-25W25R*  usePCWMeADR-25W25R*  usePCWM-25W25R*)
	# $SCRIPT_THROUGHPUT_PLOT "1W9R" $(ls useHTMUndo-1W9R*  usePCWMeADRT1-1W9R*  usePCWMeADR-1W9R*  usePCWM-1W9R*)
	# $SCRIPT_THROUGHPUT_PLOT "9W1R" $(ls useHTMUndo-9W1R*  usePCWMeADRT1-9W1R*  usePCWMeADR-9W1R*  usePCWM-9W1R*)
	# $SCRIPT_THROUGHPUT_PLOT "5W45R" $(ls useHTMUndo-5W45R*  usePCWMeADRT1-5W45R*  usePCWMeADR-5W45R*  usePCWM-5W45R*)

	# $SCRIPT_ABORT_PLOT "45W5R"  $(ls useHTMUndo-5W5R*  usePCWMeADRT1-5W5R*  usePCWMeADR-5W5R*  usePCWM-5W5R*)
	# $SCRIPT_ABORT_PLOT "25W25R" $(ls useHTMUndo-25W25R*  usePCWMeADRT1-25W25R*  usePCWMeADR-25W25R*  usePCWM-25W25R*)
	# $SCRIPT_ABORT_PLOT "1W9R" $(ls useHTMUndo-1W9R*  usePCWMeADRT1-1W9R*  usePCWMeADR-1W9R*  usePCWM-1W9R*)
	# $SCRIPT_ABORT_PLOT "9W1R" $(ls useHTMUndo-9W1R*  usePCWMeADRT1-9W1R*  usePCWMeADR-9W1R*  usePCWM-9W1R*)
	# $SCRIPT_ABORT_PLOT "5W45R" $(ls useHTMUndo-5W45R*  usePCWMeADRT1-5W45R*  usePCWMeADR-5W45R*  usePCWM-5W45R*)

	# $SCRIPT_ABORT_TYPE_PLOT "45W5R"  $(ls useHTMUndo-5W5R*  usePCWMeADRT1-5W5R*  usePCWMeADR-5W5R*  usePCWM-5W5R*)
	# $SCRIPT_ABORT_TYPE_PLOT "25W25R" $(ls useHTMUndo-25W25R*  usePCWMeADRT1-25W25R*  usePCWMeADR-25W25R*  usePCWM-25W25R*)
	# $SCRIPT_ABORT_TYPE_PLOT "1W9R" $(ls useHTMUndo-1W9R*  usePCWMeADRT1-1W9R*  usePCWMeADR-1W9R*  usePCWM-1W9R*)
	# $SCRIPT_ABORT_TYPE_PLOT "9W1R" $(ls useHTMUndo-9W1R*  usePCWMeADRT1-9W1R*  usePCWMeADR-9W1R*  usePCWM-9W1R*)
	# $SCRIPT_ABORT_TYPE_PLOT "5W45R" $(ls useHTMUndo-5W45R*  usePCWMeADRT1-5W45R*  usePCWMeADR-5W45R*  usePCWM-5W45R*)

	echo "data is in $PWD"
	cd $SCRIPT_PATH
done
