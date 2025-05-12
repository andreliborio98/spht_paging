source paths.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

MALLOC_1GB=1073741824
MALLOC_2GB=2147483648
MALLOC_4GB=4294967296
MALLOC_8GB=8589934592
MALLOC_16GB=17179869184
MALLOC_24GB=25769803776
MALLOC_32GB=34359738368
MALLOC_48GB=51539607552
MALLOC_64GB=68719476736
MALLOC_96GB=103079215104
MALLOC_128GB=137438953472
MALLOC_160GB=171798691840
MALLOC_256GB=274877906944

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

# mkdir /home/vagrant/node05-heap-sizes-disk-swap/
# scp node05:~/projs/test_nvhtm_wait_phase/data_mini_bench_workers/* /home/vagrant/node05-heap-sizes-disk-swap
# cp /home/vagrant/node05-heap-sizes-disk-swap/* data/

# mkdir /home/vagrant/nvram-heap-sizes-disk-swap/
# scp nvram:~/projs/test_nvhtm_wait_phase/data_mini_bench_workers/* /home/vagrant/nvram-heap-sizes-disk-swap
cp /home/vagrant/nvram-heap-sizes-disk-swap/* data/

# mkdir /home/vagrant/node05-heap-sizes-pm-swap/
# scp node05:~/projs/test_nvhtm_wait_phase/data_mini_bench_workers/* /home/vagrant/node05-heap-sizes-pm-swap
# cp /home/vagrant/node05-heap-sizes-pm-swap/* data/

# mkdir /home/vagrant/nvram-heap-sizes-pm-swap/
# scp nvram:~/projs/test_nvhtm_wait_phase/data_mini_bench_workers/* /home/vagrant/nvram-heap-sizes-pm-swap
# cp /home/vagrant/nvram-heap-sizes-pm-swap/* data/

# mkdir /home/vagrant/node05-heap-sizes-pm-swap-seq/
# scp node05:~/projs/test_nvhtm_wait_phase/data_mini_bench_workers/* /home/vagrant/node05-heap-sizes-pm-swap-seq
# cp /home/vagrant/node05-heap-sizes-pm-swap-seq/* data/

# mkdir /home/vagrant/node05-heap-sizes-disk-swap-seq/
# scp node05:~/projs/test_nvhtm_wait_phase/data_mini_bench_workers/* /home/vagrant/node05-heap-sizes-disk-swap-seq
# cp /home/vagrant/node05-heap-sizes-disk-swap-seq/* data/

cd data
rename 's/274877906944/256GB/' *.tsv; rename 's/171798691840/160GB/' *.tsv; rename 's/137438953472/128GB/' *.tsv; rename 's/103079215104/96GB/' *.tsv; rename 's/68719476736/64GB/' *.tsv; rename 's/51539607552/48GB/' *.tsv; rename 's/34359738368/32GB/' *.tsv; rename 's/25769803776/24GB/' *.tsv; rename 's/17179869184/16GB/' *.tsv; rename 's/8589934592/8GB/' *.tsv; rename 's/4294967296/4GB/' *.tsv; rename 's/2147483648/2GB/' *.tsv; rename 's/1073741824/1GB/' *.tsv
cd ..

# for heap in $MALLOC_1GB $MALLOC_2GB $MALLOC_4GB $MALLOC_8GB $MALLOC_16GB $MALLOC_32GB $MALLOC_64GB $MALLOC_128GB $MALLOC_160GB
for heap in 1GB 2GB 4GB 8GB 16GB 24GB 32GB 48GB 64GB 96GB 128GB 160GB 256GB
do
  for sol in usePCWMeADRT1 usePCWMeADR usePCWM useSharedHTMUndo
  do
    for workload in 50W5R-PRI 5W50R-PRI 4W4R-PRI
    do
      $SCRIPT_AVG $(ls data/${sol}-${workload}-${heap}_s*.tsv)
      mv avg.txt ${sol}-${workload}-${heap}_avg.tsv
      mv stdev.txt ${sol}-${workload}-${heap}_stdev.tsv
      $SCRIPT_AVG $(ls data/prof_${sol}-${workload}-${heap}_s*.tsv)
      mv avg.txt prof_${sol}-${workload}-${heap}_avg.tsv
      mv stdev.txt prof_${sol}-${workload}-${heap}_stdev.tsv
      $SCRIPT_AVG $(ls data/log_${sol}-${workload}-${heap}_s*.tsv)
      mv avg.txt log_${sol}-${workload}-${heap}_avg.tsv
      mv stdev.txt log_${sol}-${workload}-${heap}_stdev.tsv
    done
  done
done


# tar -Jcf data.tar.xz ./data
# rm -r ./data
echo "data is in $PWD"

$SCRIPT_THROUGHPUT_PLOT "50W5R SPHT-eADR" $(ls usePCWMeADR-50W5R-PRI-*)
$SCRIPT_THROUGHPUT_PLOT "50W5R Smart Close" $(ls usePCWMeADRT1-50W5R-PRI-*)
$SCRIPT_THROUGHPUT_PLOT "50W5R SPHT" $(ls usePCWM-50W5R-PRI-*)
$SCRIPT_THROUGHPUT_PLOT "50W5R HTM+Undo (Shared)" $(ls useSharedHTMUndo-50W5R-PRI-*)

$SCRIPT_THROUGHPUT_PLOT "5W50R SPHT-eADR" $(ls usePCWMeADR-5W50R-PRI-*)
$SCRIPT_THROUGHPUT_PLOT "5W50R Smart Close" $(ls usePCWMeADRT1-5W50R-PRI-*)
$SCRIPT_THROUGHPUT_PLOT "5W50R SPHT" $(ls usePCWM-5W50R-PRI-*)
$SCRIPT_THROUGHPUT_PLOT "5W50R HTM+Undo (Shared)" $(ls useSharedHTMUndo-5W50R-PRI-*)

$SCRIPT_THROUGHPUT_PLOT "4W4R SPHT-eADR" $(ls usePCWMeADR-4W4R-PRI-*)
$SCRIPT_THROUGHPUT_PLOT "4W4R Smart Close" $(ls usePCWMeADRT1-4W4R-PRI-*)
$SCRIPT_THROUGHPUT_PLOT "4W4R SPHT" $(ls usePCWM-4W4R-PRI-*)
$SCRIPT_THROUGHPUT_PLOT "4W4R HTM+Undo (Shared)" $(ls useSharedHTMUndo-4W4R-PRI-*)

# $SCRIPT_THROUGHPUT_PLOT "1GB" $(ls usePCWMeADRT1-50W5R-PRI-1GB* usePCWMeADR-50W5R-PRI-1GB* usePCWM-50W5R-PRI-1GB* useSharedHTMUndo-50W5R-PRI-1GB*)
# $SCRIPT_THROUGHPUT_PLOT "2GB" $(ls usePCWMeADRT1-50W5R-PRI-2GB* usePCWMeADR-50W5R-PRI-2GB* usePCWM-50W5R-PRI-2GB* useSharedHTMUndo-50W5R-PRI-2GB*)
# $SCRIPT_THROUGHPUT_PLOT "4GB" $(ls usePCWMeADRT1-50W5R-PRI-4GB* usePCWMeADR-50W5R-PRI-4GB* usePCWM-50W5R-PRI-4GB* useSharedHTMUndo-50W5R-PRI-4GB*)
# $SCRIPT_THROUGHPUT_PLOT "8GB" $(ls usePCWMeADRT1-50W5R-PRI-8GB* usePCWMeADR-50W5R-PRI-8GB* usePCWM-50W5R-PRI-8GB* useSharedHTMUndo-50W5R-PRI-8GB*)
# $SCRIPT_THROUGHPUT_PLOT "16GB" $(ls usePCWMeADRT1-50W5R-PRI-16GB* usePCWMeADR-50W5R-PRI-16GB* usePCWM-50W5R-PRI-16GB* useSharedHTMUndo-50W5R-PRI-16GB*)
# $SCRIPT_THROUGHPUT_PLOT "24GB" $(ls usePCWMeADRT1-50W5R-PRI-24GB* usePCWMeADR-50W5R-PRI-24GB* usePCWM-50W5R-PRI-24GB* useSharedHTMUndo-50W5R-PRI-24GB*)
# $SCRIPT_THROUGHPUT_PLOT "32GB" $(ls usePCWMeADRT1-50W5R-PRI-32GB* usePCWMeADR-50W5R-PRI-32GB* usePCWM-50W5R-PRI-32GB* useSharedHTMUndo-50W5R-PRI-32GB*)
# $SCRIPT_THROUGHPUT_PLOT "48GB" $(ls usePCWMeADRT1-50W5R-PRI-48GB* usePCWMeADR-50W5R-PRI-48GB* usePCWM-50W5R-PRI-48GB* useSharedHTMUndo-50W5R-PRI-48GB*)
# $SCRIPT_THROUGHPUT_PLOT "64GB" $(ls usePCWMeADRT1-50W5R-PRI-64GB* usePCWMeADR-50W5R-PRI-64GB* usePCWM-50W5R-PRI-64GB* useSharedHTMUndo-50W5R-PRI-64GB*)
# $SCRIPT_THROUGHPUT_PLOT "96GB" $(ls usePCWMeADRT1-50W5R-PRI-96GB* usePCWMeADR-50W5R-PRI-96GB* usePCWM-50W5R-PRI-96GB* useSharedHTMUndo-50W5R-PRI-96GB*)
# $SCRIPT_THROUGHPUT_PLOT "128GB" $(ls usePCWMeADRT1-50W5R-PRI-128GB* usePCWMeADR-50W5R-PRI-128GB* usePCWM-50W5R-PRI-128GB* useSharedHTMUndo-50W5R-PRI-128GB*)
# $SCRIPT_THROUGHPUT_PLOT "160GB" $(ls usePCWMeADRT1-50W5R-PRI-160GB* usePCWMeADR-50W5R-PRI-160GB* usePCWM-50W5R-PRI-160GB* useSharedHTMUndo-50W5R-PRI-160GB*)

# $SCRIPT_ABORT_PLOT "1GB" $(ls usePCWMeADRT1-50W5R-PRI-1GB* usePCWMeADR-50W5R-PRI-1GB* usePCWM-50W5R-PRI-1GB* useSharedHTMUndo-50W5R-PRI-1GB*)
# $SCRIPT_ABORT_PLOT "2GB" $(ls usePCWMeADRT1-50W5R-PRI-2GB* usePCWMeADR-50W5R-PRI-2GB* usePCWM-50W5R-PRI-2GB* useSharedHTMUndo-50W5R-PRI-2GB*)
# $SCRIPT_ABORT_PLOT "4GB" $(ls usePCWMeADRT1-50W5R-PRI-4GB* usePCWMeADR-50W5R-PRI-4GB* usePCWM-50W5R-PRI-4GB* useSharedHTMUndo-50W5R-PRI-4GB*)
# $SCRIPT_ABORT_PLOT "8GB" $(ls usePCWMeADRT1-50W5R-PRI-8GB* usePCWMeADR-50W5R-PRI-8GB* usePCWM-50W5R-PRI-8GB* useSharedHTMUndo-50W5R-PRI-8GB*)
# $SCRIPT_ABORT_PLOT "16GB" $(ls usePCWMeADRT1-50W5R-PRI-16GB* usePCWMeADR-50W5R-PRI-16GB* usePCWM-50W5R-PRI-16GB* useSharedHTMUndo-50W5R-PRI-16GB*)
# $SCRIPT_ABORT_PLOT "24GB" $(ls usePCWMeADRT1-50W5R-PRI-24GB* usePCWMeADR-50W5R-PRI-24GB* usePCWM-50W5R-PRI-24GB* useSharedHTMUndo-50W5R-PRI-24GB*)
# $SCRIPT_ABORT_PLOT "32GB" $(ls usePCWMeADRT1-50W5R-PRI-32GB* usePCWMeADR-50W5R-PRI-32GB* usePCWM-50W5R-PRI-32GB* useSharedHTMUndo-50W5R-PRI-32GB*)
# $SCRIPT_ABORT_PLOT "48GB" $(ls usePCWMeADRT1-50W5R-PRI-48GB* usePCWMeADR-50W5R-PRI-48GB* usePCWM-50W5R-PRI-48GB* useSharedHTMUndo-50W5R-PRI-48GB*)
# $SCRIPT_ABORT_PLOT "64GB" $(ls usePCWMeADRT1-50W5R-PRI-64GB* usePCWMeADR-50W5R-PRI-64GB* usePCWM-50W5R-PRI-64GB* useSharedHTMUndo-50W5R-PRI-64GB*)
# $SCRIPT_ABORT_PLOT "96GB" $(ls usePCWMeADRT1-50W5R-PRI-96GB* usePCWMeADR-50W5R-PRI-96GB* usePCWM-50W5R-PRI-96GB* useSharedHTMUndo-50W5R-PRI-96GB*)
# $SCRIPT_ABORT_PLOT "128GB" $(ls usePCWMeADRT1-50W5R-PRI-128GB* usePCWMeADR-50W5R-PRI-128GB* usePCWM-50W5R-PRI-128GB* useSharedHTMUndo-50W5R-PRI-128GB*)
# $SCRIPT_ABORT_PLOT "160GB" $(ls usePCWMeADRT1-50W5R-PRI-160GB* usePCWMeADR-50W5R-PRI-160GB* usePCWM-50W5R-PRI-160GB* useSharedHTMUndo-50W5R-PRI-160GB*)

# $SCRIPT_ABORT_TYPE_PLOT "1GB" $(ls usePCWMeADRT1-50W5R-PRI-1GB* usePCWMeADR-50W5R-PRI-1GB* usePCWM-50W5R-PRI-1GB* useSharedHTMUndo-50W5R-PRI-1GB*)
# $SCRIPT_ABORT_TYPE_PLOT "2GB" $(ls usePCWMeADRT1-50W5R-PRI-2GB* usePCWMeADR-50W5R-PRI-2GB* usePCWM-50W5R-PRI-2GB* useSharedHTMUndo-50W5R-PRI-2GB*)
# $SCRIPT_ABORT_TYPE_PLOT "4GB" $(ls usePCWMeADRT1-50W5R-PRI-4GB* usePCWMeADR-50W5R-PRI-4GB* usePCWM-50W5R-PRI-4GB* useSharedHTMUndo-50W5R-PRI-4GB*)
# $SCRIPT_ABORT_TYPE_PLOT "8GB" $(ls usePCWMeADRT1-50W5R-PRI-8GB* usePCWMeADR-50W5R-PRI-8GB* usePCWM-50W5R-PRI-8GB* useSharedHTMUndo-50W5R-PRI-8GB*)
# $SCRIPT_ABORT_TYPE_PLOT "16GB" $(ls usePCWMeADRT1-50W5R-PRI-16GB* usePCWMeADR-50W5R-PRI-16GB* usePCWM-50W5R-PRI-16GB* useSharedHTMUndo-50W5R-PRI-16GB*)
# $SCRIPT_ABORT_TYPE_PLOT "24GB" $(ls usePCWMeADRT1-50W5R-PRI-24GB* usePCWMeADR-50W5R-PRI-24GB* usePCWM-50W5R-PRI-24GB* useSharedHTMUndo-50W5R-PRI-24GB*)
# $SCRIPT_ABORT_TYPE_PLOT "32GB" $(ls usePCWMeADRT1-50W5R-PRI-32GB* usePCWMeADR-50W5R-PRI-32GB* usePCWM-50W5R-PRI-32GB* useSharedHTMUndo-50W5R-PRI-32GB*)
# $SCRIPT_ABORT_TYPE_PLOT "48GB" $(ls usePCWMeADRT1-50W5R-PRI-48GB* usePCWMeADR-50W5R-PRI-48GB* usePCWM-50W5R-PRI-48GB* useSharedHTMUndo-50W5R-PRI-48GB*)
# $SCRIPT_ABORT_TYPE_PLOT "64GB" $(ls usePCWMeADRT1-50W5R-PRI-64GB* usePCWMeADR-50W5R-PRI-64GB* usePCWM-50W5R-PRI-64GB* useSharedHTMUndo-50W5R-PRI-64GB*)
# $SCRIPT_ABORT_TYPE_PLOT "96GB" $(ls usePCWMeADRT1-50W5R-PRI-96GB* usePCWMeADR-50W5R-PRI-96GB* usePCWM-50W5R-PRI-96GB* useSharedHTMUndo-50W5R-PRI-96GB*)
# $SCRIPT_ABORT_TYPE_PLOT "128GB" $(ls usePCWMeADRT1-50W5R-PRI-128GB* usePCWMeADR-50W5R-PRI-128GB* usePCWM-50W5R-PRI-128GB* useSharedHTMUndo-50W5R-PRI-128GB*)
# $SCRIPT_ABORT_TYPE_PLOT "160GB" $(ls usePCWMeADRT1-50W5R-PRI-160GB* usePCWMeADR-50W5R-PRI-160GB* usePCWM-50W5R-PRI-160GB* useSharedHTMUndo-50W5R-PRI-160GB*)

cd $SCRIPT_PATH