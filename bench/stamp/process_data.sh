source ../../paths.sh
source benches_args.sh


#USEFUL FOR CUSTOM RUNS (macos):
#sed -i '' 's/ HSCONSULTTIME   HSADDTS HSADDCOUNT//g' *.tsv (removes selected headers from .tsv)
#sed -i '' ' 1 s/.*/&      TIMERPLWAIT     PGOUTPOSTINIT/' *.tsv (add columns to .tsv header)
#data % sed -i '' ' 1 s/.*/&  COUNT_SWAP/' *.tsv (add columns (with swap))
#for \t in macos, use Ctrl+V and then press Tab.
REPLAYER=1 #1=ON, 0=OFF
EXPERIMENT_FOLDER='data_swap_c4_50' #$(date +%Y-%m-%dT%H_%m_%S)

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

SCRIPT_PATH=$(pwd)
SCRIPT_PAGING_PLOT=$SCRIPT_PATH/plot_paging.py
SCRIPT_REPLAY_ACTI_PLOT=$SCRIPT_PATH/plot_replayer_acti.py
SCRIPT_REPLAY_PAGES_PLOT=$SCRIPT_PATH/plot_replayer_pages.py
SCRIPT_THROUGHPUT_PLOT=$SCRIPT_PATH/plot_throughput.py
SCRIPT_THROUGHPUT_MULTIPLOT=$SCRIPT_PATH/plot_throughput_multi.py
SCRIPT_THROUGHPUT_MULTIFOLDER=$SCRIPT_PATH/plot_throughput_multifolder.py
SCRIPT_ABORT_MULTIPLOT=$SCRIPT_PATH/plot_aborts_multi.py
SCRIPT_ABORT_PLOT=$SCRIPT_PATH/plot_aborts.py
SCRIPT_PROF_PLOT=$SCRIPT_PATH/plot_prof.py
SCRIPT_AVG=$SCRIPT_PATH/../../scripts/SCRIPT_compute_AVG_ERR.R
SCRIPT_COL=$SCRIPT_PATH/../../scripts/SCRIPT_put_column.R

echo $REPLAYER
# cd $DATA_PATH
# mkdir -p stamp
# cd stamp
# mkdir -p $EXPERIMENT_FOLDER

cd $EXPERIMENT_FOLDER 
# if [ $REPLAYER -eq 1 ]; then
#   perl -i -pe 's/ ([^ ]*)$/\t\1/' *tsv
# fi

#mkdir -p data
#scp $NODE:$DM/bench/stamp/data/* data/ #use
#cp /home/andre/Code/SPHT-results/stamp/* data/ #use

#cp /home/vagrant/saved-data/* data/

# mkdir /home/vagrant/saved-data/
# scp node05:~/projs/test_nvhtm_wait_phase/bench/stamp/data/* /home/vagrant/saved-data
# cp /home/vagrant/saved-data/* data/

# for sol in useCcHTMbest usePhysicalClocks useLogicalClocks usePCWM usePCWM2 usePCWM3
#for sol in usePCWMeADRT1 usePCWMeADRT2 usePCWMeADRT3 usePCWMeADRT4 usePCWMeADR usePCWM useSharedHTMUndo
for sol in usePCWM #useSharedHTM usePCWMeADRT1 usePCWMeADR usePCWM
do
  for i in $(seq $NB_BENCHES)
  do
    $SCRIPT_COL "THROUGHPUT = (a\$NB_HTM_SUCCESS + a\$NB_FALLBACK) / a\$TIME; a = cbind(a, THROUGHPUT); " \
      $(ls data/${test_name[$i]}_${sol}_s*.tsv)
  done
done

# for sol in useLogicalClocks useCrafty useCcHTMbest usePhysicalClocks useLogicalClocks usePCWM usePCWM2 usePCWM3
# do
#   for i in 1
#   do
#     $SCRIPT_COL "THROUGHPUT = (a\$NB_HTM_SUCCESS + a\$NB_FALLBACK) / a\$TIME; a = cbind(a, THROUGHPUT); " \
#       $(ls data/${test_name[$i]}_${sol}_s*.tsv)
#   done
# done


# for sol in usePSTM
# do
#   # for i in $(seq $NB_BENCHES)
#   for i in 1
#   do
#     $SCRIPT_COL "THROUGHPUT = (a\$COMMITS) / a\$TIME; a = cbind(a, THROUGHPUT); " \
#       $(ls data/${test_name[$i]}_${sol}_s*.tsv)
#   done
# done

# for sol in useCrafty
# do
#   for i in "KMEANS_VLOW" "KMEANS_LOW" "INTRUDER"
#   do
#     $SCRIPT_COL "THROUGHPUT = (a\$NB_HTM_SUCCESS + a\$NB_FALLBACK) / a\$TIME; a = cbind(a, THROUGHPUT); " \
#       $(ls data/${i}_${sol}_s*.tsv)
#   done
# done

### TODO: this is copy from mini_bench
# useEpochCommit2 --> is too bad
# usePhysicalClocks useLogicalClocks useEpochCommit1
# usePCWC useFastPCWC
#usePCWC-F usePCWC-NF usePCWC-F-DA usePCWC-NF-DA usePCWC-F-DSN usePCWC-NF-DSN usePCWC-F-DTX usePCWC-NF-DTX usePCWC-F-DWC usePCWC-NF-DWC

# for sol in useCcHTMbest usePhysicalClocks useLogicalClocks usePCWM usePCWM2 usePCWM3 usePSTM
#for sol in usePCWMeADRT1 usePCWMeADRT2 usePCWMeADRT3 usePCWMeADRT4 usePCWMeADR usePCWM useSharedHTMUndo
for sol in usePCWM #useSharedHTM usePCWMeADRT1 usePCWMeADR usePCWM
do
  for i in $(seq $NB_BENCHES)
  do
    $SCRIPT_AVG $(ls data/${test_name[$i]}_${sol}_s*.tsv.cols)
    echo "${test_name[$i]}_${sol}"
    mv avg.txt ${test_name[$i]}_${sol}_avg.tsv
    mv stdev.txt ${test_name[$i]}_${sol}_stdev.tsv
    # $SCRIPT_AVG $(ls data/prof${test_name[$i]}_${sol}_s*.tsv)
    # mv avg.txt prof${test_name[$i]}_${sol}_avg.tsv
    # mv stdev.txt prof${test_name[$i]}_${sol}_stdev.tsv
    
    # cd ../$AUX_EXP_FOLDER #IF AUX FOLDER EXISTS!!!!!
    # $SCRIPT_AVG $(ls data/${test_name[$i]}_${sol}_s*.tsv.cols)
    # echo "${test_name[$i]}_${sol}"
    # mv avg.txt ${test_name[$i]}_${sol}_avg.tsv
    # mv stdev.txt ${test_name[$i]}_${sol}_stdev.tsv
    # cd ../$EXPERIMENT_FOLDER
  done
done


# for sol in usePCWM
# do
#   for i in 7
#   do
#     $SCRIPT_AVG $(ls data/${test_name[$i]}_${sol}_s*.tsv.cols)
#     echo "${test_name[$i]}_${sol}"
#     mv avg.txt ${test_name[$i]}_${sol}_avg.tsv
#     mv stdev.txt ${test_name[$i]}_${sol}_stdev.tsv
#     # $SCRIPT_AVG $(ls data/prof${test_name[$i]}_${sol}_s*.tsv)
#     # mv avg.txt prof${test_name[$i]}_${sol}_avg.tsv
#     # mv stdev.txt prof${test_name[$i]}_${sol}_stdev.tsv
#   done
# done

# for sol in useCrafty
# do
#   for i in $(seq $NB_BENCHES)
#   do
#     $SCRIPT_AVG $(ls data/${test_name[$i]}_${sol}_s*.tsv.out.cols)
#     echo "${test_name[$i]}_${sol}"
#     mv avg.txt ${test_name[$i]}_${sol}_avg.tsv
#     mv stdev.txt ${test_name[$i]}_${sol}_stdev.tsv
#     $SCRIPT_AVG $(ls data/${test_name[$i]}_${sol}_s*.tsv.cols)
#     echo "${test_name[$i]}_${sol}"
#     mv avg.txt ${test_name[$i]}_${sol}_avg.tsv
#     mv stdev.txt ${test_name[$i]}_${sol}_stdev.tsv
#     # $SCRIPT_AVG $(ls data/prof${test_name[$i]}_${sol}_s*.tsv)
#     # mv avg.txt prof${test_name[$i]}_${sol}_avg.tsv
#     # mv stdev.txt prof${test_name[$i]}_${sol}_stdev.tsv
#   done
# done


# for sol in useCrafty
# do
#   for i in $(seq $NB_BENCHES)
#   do
#     $SCRIPT_AVG $(ls data/${test_name[$i]}_${sol}_s*.tsv.out.cols)
#     echo "${test_name[$i]}_${sol}"
#     mv avg.txt ${test_name[$i]}_${sol}_avg.tsv
#     mv stdev.txt ${test_name[$i]}_${sol}_stdev.tsv
#     # $SCRIPT_AVG $(ls data/prof${test_name[$i]}_${sol}_s*.tsv)
#     # mv avg.txt prof${test_name[$i]}_${sol}_avg.tsv
#     # mv stdev.txt prof${test_name[$i]}_${sol}_stdev.tsv
#   done
# done

# tar -Jcf data.tar.xz ./data
# rm -r ./data
echo " >>>>> data is in $PWD >>>>>"

for i in $(seq $NB_BENCHES)
do
  # $SCRIPT_THROUGHPUT_PLOT "${test_name[$i]}-PIN0" $(ls ${test_name[$i]}_*-PIN0*)
  # $SCRIPT_ABORT_PLOT "${test_name[$i]}-PIN0" $(ls ${test_name[$i]}_*-PIN0*)
  # $SCRIPT_THROUGHPUT_PLOT "${test_name[$i]}-PIN1" $(ls ${test_name[$i]}_*-PIN1*)
  # $SCRIPT_ABORT_PLOT "${test_name[$i]}-PIN1" $(ls ${test_name[$i]}_*-PIN1*)
  
  $SCRIPT_THROUGHPUT_PLOT "${test_name[$i]}" $(ls ${test_name[$i]}_*)
  $SCRIPT_ABORT_PLOT "${test_name[$i]}" $(ls ${test_name[$i]}_*)
  $SCRIPT_PAGING_PLOT "${test_name[$i]}" $(ls ${test_name[$i]}_*)
  # if [ $REPLAYER -eq 1 ]; then
    $SCRIPT_REPLAY_PAGES_PLOT "${test_name[$i]}" $(ls ${test_name[$i]}_*)
    $SCRIPT_REPLAY_ACTI_PLOT "${test_name[$i]}" $(ls ${test_name[$i]}_*)
  # fi
done

### TODO
NB_SOLUTIONS=8 #8 STAMP BENCHMARKS

$SCRIPT_ABORT_MULTIPLOT $NB_BENCHES \
  $NB_SOLUTIONS $(ls VACATION_LOW_use*) \
  $NB_SOLUTIONS $(ls SSCA2_use*) \
  $NB_SOLUTIONS $(ls KMEANS_VLOW_use*) \
  $NB_SOLUTIONS $(ls INTRUDER_use*) \
  $NB_SOLUTIONS $(ls KMEANS_LOW_use*) \
  $NB_SOLUTIONS $(ls GENOME_use*) \
  $NB_SOLUTIONS $(ls YADA_use*) \
  $NB_SOLUTIONS $(ls LABYRINTH_use*)

$SCRIPT_THROUGHPUT_PLOT $NB_BENCHES \
  $NB_SOLUTIONS $(ls VACATION_LOW_use*) \
  $NB_SOLUTIONS $(ls SSCA2_use*) \
  $NB_SOLUTIONS $(ls KMEANS_VLOW_use*) \
  $NB_SOLUTIONS $(ls INTRUDER_use*) \
  $NB_SOLUTIONS $(ls KMEANS_LOW_use*) \
  $NB_SOLUTIONS $(ls GENOME_use*) \
  $NB_SOLUTIONS $(ls YADA_use*) \
  $NB_SOLUTIONS $(ls LABYRINTH_use*)

$SCRIPT_PAGING_PLOT $NB_BENCHES \
  $NB_SOLUTIONS $(ls VACATION_LOW_use*) \
  $NB_SOLUTIONS $(ls SSCA2_use*) \
  $NB_SOLUTIONS $(ls KMEANS_VLOW_use*) \
  $NB_SOLUTIONS $(ls INTRUDER_use*) \
  $NB_SOLUTIONS $(ls KMEANS_LOW_use*) \
  $NB_SOLUTIONS $(ls GENOME_use*) \
  $NB_SOLUTIONS $(ls YADA_use*) \
  $NB_SOLUTIONS $(ls LABYRINTH_use*)

$SCRIPT_REPLAY_ACTI_PLOT $NB_BENCHES \
  $NB_SOLUTIONS $(ls VACATION_LOW_use*) \
  $NB_SOLUTIONS $(ls SSCA2_use*) \
  $NB_SOLUTIONS $(ls KMEANS_VLOW_use*) \
  $NB_SOLUTIONS $(ls INTRUDER_use*) \
  $NB_SOLUTIONS $(ls KMEANS_LOW_use*) \
  $NB_SOLUTIONS $(ls GENOME_use*) \
  $NB_SOLUTIONS $(ls YADA_use*) \
  $NB_SOLUTIONS $(ls LABYRINTH_use*)

  $SCRIPT_REPLAY_PAGES_PLOT $NB_BENCHES \
  $NB_SOLUTIONS $(ls VACATION_LOW_use*) \
  $NB_SOLUTIONS $(ls SSCA2_use*) \
  $NB_SOLUTIONS $(ls KMEANS_VLOW_use*) \
  $NB_SOLUTIONS $(ls INTRUDER_use*) \
  $NB_SOLUTIONS $(ls KMEANS_LOW_use*) \
  $NB_SOLUTIONS $(ls GENOME_use*) \
  $NB_SOLUTIONS $(ls YADA_use*) \
  $NB_SOLUTIONS $(ls LABYRINTH_use*)

# $SCRIPT_THROUGHPUT_MULTIFOLDER $NB_BENCHES \
#   $NB_SOLUTIONS $(ls VACATION_LOW_use*) $(ls ../$AUX_EXP_FOLDER/VACATION_LOW_use*) \
#   $NB_SOLUTIONS $(ls SSCA2_use*) $(ls ../$AUX_EXP_FOLDER/SSCA2_use*) \
#   $NB_SOLUTIONS $(ls KMEANS_VLOW_use*) $(ls ../$AUX_EXP_FOLDER/KMEANS_VLOW_use*)\
#   $NB_SOLUTIONS $(ls INTRUDER_use*) $(ls ../$AUX_EXP_FOLDER/INTRUDER_use*) \
#   $NB_SOLUTIONS $(ls KMEANS_LOW_use*) $(ls ../$AUX_EXP_FOLDER/KMEANS_LOW_use*) \
#   $NB_SOLUTIONS $(ls GENOME_use*) $(ls ../$AUX_EXP_FOLDER/GENOME_use*) \
#   $NB_SOLUTIONS $(ls YADA_use*) $(ls ../$AUX_EXP_FOLDER/YADA_use*) \
#   $NB_SOLUTIONS $(ls LABYRINTH_use*) $(ls ../$AUX_EXP_FOLDER/LABYRINTH_use*)

# NB_SOLUTIONS=3
# $SCRIPT_THROUGHPUT_MULTIPLOT 5 \
#   $NB_SOLUTIONS $(ls VACATION_LOW_use*) \
#   $NB_SOLUTIONS $(ls KMEANS_VLOW_use*) \
#   $NB_SOLUTIONS $(ls INTRUDER_use*) \
#   $NB_SOLUTIONS $(ls KMEANS_LOW_use*) \
#   $NB_SOLUTIONS $(ls GENOME_use*)

# $SCRIPT_ABORT_MULTIPLOT 3 \
#   $NB_SOLUTIONS $(ls VACATION_LOW_use*) \
#   $NB_SOLUTIONS $(ls INTRUDER_use*) \
#   $NB_SOLUTIONS $(ls YADA_use*)


# cd $SCRIPT_PATH
