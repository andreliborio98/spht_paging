#!/bin/bash

EXPERIMENT_TIME=4000000
PINNING=1
SAMPLES=5

mkdir -p mini_bench

cd ./nvhtm/
make clean ; make -j40 OPTIMIZE=1 DISABLE_FLAG_IS_IN_TX=1 DISABLE_FLAG_SNAPSHOT=1
cd -

#OPTIMIZE=1 USE_VECT=1

# for s in $(seq $SAMPLES)
# do
#   # usePCWC usePCWC2 useFastPCWC useUpperBound
#   # useLogicalClocks usePhysicalClocks usePCWC-F usePCWM   usePCWC-NF
#   for sol in useLogicalClocks usePhysicalClocks usePCWM
#   do
#     echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY" \
#       > mini_bench/${sol}_s${s}.tsv
#     #  40 48 56 64
#     for i in 1 2 4 6 8 10 12 14 16
#     do
#       echo "./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 NB_READS=4 NB_WRITES=2 FLUSH_LAT=0 spinInCycles=1 disableLogChecker=1 \
#         FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$i PINNING=$PINNING PROFILE_FILE=./mini_bench/prof_${sol}_s${s}.tsv \
#         ERROR_FILE=./mini_bench/error_${sol}_s${s}.tsv"
#       ./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 NB_READS=4 NB_WRITES=2 FLUSH_LAT=0 spinInCycles=1 disableLogChecker=1 \
#         FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$i PINNING=$PINNING PROFILE_FILE="./mini_bench/prof_${sol}_s${s}.tsv" \
#         ERROR_FILE="./mini_bench/error_${sol}_s${s}.tsv" | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  | tail -n 1 \
#           >> mini_bench/${sol}_s${s}.tsv
#       sleep 0.05s
#     done
#   done
# done

########################################
### READ/WRITES   ######################
########################################


# for s in $(seq $SAMPLES)
# do
#   # useEpochCommit2 it is too bad
#   # usePhysicalClocks useLogicalClocks useEpochCommit1
#   # usePCWC usePCWC2 useFastPCWC useUpperBound
#   # useLogicalClocks useEpochCommit1 usePCWC-F usePCWC-NF usePCWM
#   for sol in useLogicalClocks usePhysicalClocks usePCWC-F usePCWC-NF usePCWM
#   do
#     echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY" \
#       > mini_bench/${sol}_s${s}.tsv
#     echo -e "NB_READS\tNB_WRITES" \
#       > mini_bench/params_${sol}_s${s}.tsv
#     #  40 48 56 64
#     i=2 # nb_threads
#     for r in 30 40 50 60 70 80 90 100 110
#     do
#       echo -e "${r}\t2" \
#         >> mini_bench/params_${sol}_s${s}.tsv
#       echo "./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 NB_READS=$r NB_WRITES=2 FLUSH_LAT=0 spinInCycles=1 disableLogChecker=1 \
#         FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$i PINNING=$PINNING PROFILE_FILE=./mini_bench/prof_${sol}_s${s}.tsv \
#         ERROR_FILE=./mini_bench/error_${sol}_s${s}.tsv"
#       ./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 NB_READS=$r NB_WRITES=2 FLUSH_LAT=0 spinInCycles=1 disableLogChecker=1 \
#         FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$i PINNING=$PINNING PROFILE_FILE="./mini_bench/prof_${sol}_s${s}.tsv" \
#         ERROR_FILE="./mini_bench/error_${sol}_s${s}.tsv" | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  | tail -n 1 \
#           >> mini_bench/${sol}_s${s}.tsv
#       sleep 0.05s
#     done
#   done
# done


########################################
### OPTIMIZATIONS ######################
########################################

# for s in $(seq $SAMPLES)
# do
#   for sol in useLogicalClocks usePhysicalClocks
#   do
#     echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY" \
#       > mini_bench/${sol}_s${s}.tsv
#     for t in 1 2 4 8 12 16 24 32 48 64
#     do
#       echo "./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=1 FLUSH_LAT=$lat spinInCycles=1 disableLogChecker=1 \
#         FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$t PINNING=$PINNING PROFILE_FILE=\"./mini_bench/prof_${sol}_s${s}.tsv\" \
#         ERROR_FILE=\"./mini_bench/error_${sol}_s${s}.tsv\""
#       ./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 disableLogChecker=1 DISABLE_LOG_REPLAY \
#         FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$t NB_WRITES=2 NB_READS=4 PINNING=$PINNING PROFILE_FILE="./mini_bench/prof_${sol}_s${s}.tsv" \
#         ERROR_FILE="./mini_bench/error_${sol}_s${s}.tsv" | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n \
#         | tail -n 1 >> mini_bench/${sol}_s${s}.tsv
#       sleep 0.05s
#     done
#   done
# done

# make_opt[1]="DISABLE_FLAG_IS_IN_TX=1 DISABLE_FLAG_SNAPSHOT=1"
# make_opt[2]="DISABLE_FLAG_IS_IN_TX=1"
# make_opt[3]="DISABLE_FLAG_SNAPSHOT=1"
# make_opt[4]=""

# make_opt_name[1]="DA"
# make_opt_name[2]="DTX"
# make_opt_name[3]="DSN"
# make_opt_name[4]="ALL"

# NB_OPTS=4

# make_opt[1]="DISABLE_FLAG_IS_IN_TX=1"
# make_opt[2]=""

# make_opt_name[1]="DTX"
# make_opt_name[2]="ALL"

# NB_OPTS=1

# for i in $(seq 1 $NB_OPTS)
# do
#   ### NORMAL
#   cd ./nvhtm/
#   make clean ; make -j40 OPTIMIZE=1 ${make_opt[$i]}
#   cd -

#   for s in $(seq $SAMPLES)
#   do
#     for sol in usePCWC-F
#     do
#       echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY" \
#         > mini_bench/${sol}-${make_opt_name[$i]}_s${s}.tsv
#       for t in 1 2 4 6 8 10 12 16 24 32 48 64
#       do
#         echo "./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=1 FLUSH_LAT=$lat spinInCycles=1 disableLogChecker=1 \
#           FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$t PINNING=$PINNING PROFILE_FILE=\"./mini_bench/prof_${lat}_${sol}_s${s}.tsv\" \
#           ERROR_FILE=\"./mini_bench/error_${sol}-${make_opt_name[$i]}_s${s}.tsv\""
#         ./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 disableLogChecker=1 DISABLE_LOG_REPLAY \
#           FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$t NB_WRITES=2 NB_READS=4 PINNING=$PINNING PROFILE_FILE="./mini_bench/prof_${sol}-${make_opt_name[$i]}_s${s}.tsv" \
#           ERROR_FILE="./mini_bench/error_${sol}-${make_opt_name[$i]}_s${s}.tsv"  | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  \
#           | tail -n 1 >> mini_bench/${sol}-${make_opt_name[$i]}_s${s}.tsv
#         sleep 0.05s
#       done
#     done
#   done
# done

# make_opt[1]="DISABLE_FLAG_IS_IN_TX=1"
# make_opt[2]=""

# make_opt_name[1]="DTX"
# make_opt_name[2]="ALL"

# NB_OPTS=1

# for i in $(seq $NB_OPTS)
# do
#   ### NORMAL
#   cd ./nvhtm/
#   make clean ; make -j40 OPTIMIZE=1 ${make_opt[$i]}
#   cd -

#   for s in $(seq $SAMPLES)
#   do
#     for sol in usePCWC-NF
#     do
#       echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY" \
#         > mini_bench/${sol}-${make_opt_name[$i]}_s${s}.tsv
#       for t in 1 2 4 6 8 10 12 16 24 32 48 64
#       do
#         echo "./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=1 FLUSH_LAT=$lat spinInCycles=1 disableLogChecker=1 \
#           FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$t PINNING=$PINNING PROFILE_FILE=\"./mini_bench/prof_${lat}_${sol}_s${s}.tsv\" \
#           ERROR_FILE=\"./mini_bench/error_${sol}-${make_opt_name[$i]}_s${s}.tsv\""
#         ./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 disableLogChecker=1 DISABLE_LOG_REPLAY \
#           FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$t NB_WRITES=2 NB_READS=4 PINNING=$PINNING PROFILE_FILE="./mini_bench/prof_${sol}-${make_opt_name[$i]}_s${s}.tsv" \
#           ERROR_FILE="./mini_bench/error_${sol}-${make_opt_name[$i]}_s${s}.tsv" | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  \
#            | tail -n 1 >> mini_bench/${sol}-${make_opt_name[$i]}_s${s}.tsv
#         sleep 0.05s
#       done
#     done
#   done
# done


# make_opt[1]="DISABLE_PCWM_OPT=1"
# make_opt[2]=""

# make_opt_name[1]="DOPT"
# make_opt_name[2]="WOPT"

# NB_OPTS=2

# for i in $(seq $NB_OPTS)
# do
#   ### NORMAL
#   cd ./nvhtm/
#   make clean ; make -j40 OPTIMIZE=1 ${make_opt[$i]}
#   cd -

#   for s in $(seq $SAMPLES)
#   do
#     for sol in usePCWM usePCWM2
#     do
#       echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY" \
#         > mini_bench/${sol}-${make_opt_name[$i]}_s${s}.tsv
#       for t in 1 2 4 8 12 16 24 32 48 64
#       do
#         echo "./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=1 FLUSH_LAT=$lat spinInCycles=1 disableLogChecker=1 \
#           FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$t PINNING=$PINNING PROFILE_FILE=\"./mini_bench/prof_${lat}_${sol}_s${s}.tsv\" \
#           ERROR_FILE=\"./mini_bench/error_${sol}-${make_opt_name[$i]}_s${s}.tsv\""
#         ./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 disableLogChecker=1 DISABLE_LOG_REPLAY \
#           FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$t NB_WRITES=4 NB_READS=64 PINNING=$PINNING PROFILE_FILE="./mini_bench/prof_${sol}-${make_opt_name[$i]}_s${s}.tsv" \
#           ERROR_FILE="./mini_bench/error_${sol}-${make_opt_name[$i]}_s${s}.tsv" | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  \
#            | tail -n 1 >> mini_bench/${sol}-${make_opt_name[$i]}_s${s}.tsv
#         sleep 0.05s
#       done
#     done
#   done
# done




### NORMAL
cd ./nvhtm/
make clean ; make -j40 OPTIMIZE=1
cd -

for s in $(seq $SAMPLES)
do
  for sol in usePCWM usePCWM2
  do
    for PINNING in 0 1
    do
      echo -e "THREADS\tTHROUGHPUT\tHTM_COMMITS\tSGL_COMMITS\tHTM_ABORTS\tHTM_CONFLICT\tHTM_CAPACITY\tHTM_EXPLICIT\tHTM_OTHER" \
        > mini_bench/${sol}-PIN${PINNING}_s${s}.tsv
      for t in 1 2 4 8 12 16 20 24 28 32
      do
        echo "t $t"
        ./nvhtm/test_spins EXPERIMENT_TIME=$EXPERIMENT_TIME SPINS_EXEC=0 FLUSH_LAT=0 spinInCycles=1 disableLogChecker=1 DISABLE_LOG_REPLAY \
          FORCE_LEARN=1 tid0Slowdown=0 $sol=1 NB_THREADS=$t NB_WRITES=4 NB_READS=64 PINNING=$PINNING PROFILE_FILE="./mini_bench/prof_${sol}-PIN${PINNING}_s${s}.tsv" \
          ERROR_FILE="./mini_bench/error_${sol}-PIN${PINNING}_s${s}.tsv" | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  \
            | tail -n 1 >> mini_bench/${sol}-PIN${PINNING}_s${s}.tsv
        sleep 0.05s
      done
    done
  done
done



