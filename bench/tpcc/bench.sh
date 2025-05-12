#!/bin/bash

source benches_args.sh

#SETTINGS
####################
NB_THREADS="1 2 4 8 12 16 20 24"
NB_SAMPLES=3 #number of times the bench runs
PINNING=1
NB_REPLAYERS=1
LOG_TYPE="CONCURRENT" #use as "LOG_REPLAY_"TYPE""
LOG_REPLAY_FLAG=1 #0 or 1
FLUSH_LAT=0
SOLUTIONS="usePCWM" #usePCWMeADRT1 usePCWMeADRT2 usePCWMeADR usePCWM useHTM
DEFAULT_PRESET=4 #default
    #0=full compilation
    #1=replayer only
    #2=replayer+paging+simplehash
    #3=replayer+paging
    #4=replayer+paging+hashmap
    #5=replayer+paging+swap
    #6=all off
####################

if [ $1 -ge 0 ] || [ $1 -le 6 ] #ensures its in the 0..6 range
then
    DEFAULT_PRESET=$1
fi

if [ $DEFAULT_PRESET -ne 0 ]
then
    start=$(($DEFAULT_PRESET))
    end=$(($DEFAULT_PRESET))
fi
if [ $DEFAULT_PRESET -eq 0 ]
then
    start=1
    end=6
fi
if [ $DEFAULT_PRESET -eq 6 ] || [ $DEFAULT_PRESET -eq 2 ]
then
  NB_REPLAYERS=0
  LOG_REPLAY_FLAG=0
else
    if [ $NB_REPLAYERS -eq 0 ]
    then
        NB_REPLAYERS=1
    fi
fi

cd ../../
./makeall.sh $DEFAULT_PRESET
# sleep 2
### TODO!!!
# cd nvhtm
# make clean
# make -j40 OPTIMIZE=1 DISABLE_FLAG_IS_IN_TX=1 NPROFILE=1
cd -

### this file must exist here
# cp ../../nvhtm/CPU_FREQ_kHZ.txt ./code

# ./build-tpcc.sh nvhtm ${paging}
for ((compilation = $start; compilation <= $end; compilation++))
do
  if [ $DEFAULT_PRESET -eq 0 ] #TODO: 0 setting not working properly yet
  then
    case $compilation in 
        1) run="_r" ;;
        2) run="_rpsh" ;; #m=minihash
        3) run="_rp" ;;
        4) run="_rph" ;;
        5) run="_rps" ;;
        6) run="" ;;
    esac
  else
      run="";
  fi

  mkdir -p data${run}

  for s in $(seq $NB_SAMPLES)
  do
    # useEpochCommit1 --> blocks
    # useHTM useLogicalClocks usePhysicalClocks usePCWM2 usePCWM useCcHTM
    # for sol in usePCWM usePCWM3 usePhysicalClocks useLogicalClocks usePCWM2 useCcHTMbest
    for sol in ${SOLUTIONS[@]}
    do
      mkdir -p data${run}
      for i in $(seq $NB_BENCHES)
      do
        echo "NB_REPLAYERS=${NB_REPLAYERS} PINNING=${PINNING} FLUSH_LAT=${FLUSHLAT} LOG_REPLAY_CONCURRENT=${LOG_REPLAY_FLAG} ${sol}=1 ERROR_FILE=./data/err${test_name[$i]}_${sol}_s${s}.tsv PROFILE_FILE=./data/prof${test_name[$i]}_${sol}_s${s}.tsv " \
          > nvhtm_params.txt
        # echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER" \
        #   > data/${test_name[$i]}_${sol}_s${s}.tsv
        case $DEFAULT_PRESET in
        6) echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPERCPAGEIN\tPOACTIV\tRAMUSAGE\tHSADDTS\tHSADDCOUNT" \
              > data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
        5) echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPERCPAGEIN\tPOACTIV\tRAMUSAGE\tSWAPCOUNT\tTIME_ADDPAGE(MS)\tTADDPAGE_TICKS\tTIME_RMPAGE(MS)\tTRMPAGE_TICKS\tTIME_SELPAGE(MS)\tTSELPAGE_TICKS\tCURR_SIZE_OF_HEAP" \
              > data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
        4) echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPERCPAGEIN\tPOACTIV\tRAMUSAGE\tHSADDTS\tHSADDCOUNT\tTIMERPLWAIT\tTIME_ADDPAGE(MS)\tTADDPAGE_TICKS\tTIME_RMPAGE(MS)\tTRMPAGE_TICKS\tTIME_SELPAGE(MS)\tTSELPAGE_TICKS\tCURR_SIZE_OF_HEAP" \
              > data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
        3) echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPERCPAGEIN\tPOACTIV\tRAMUSAGE\tHSADDTS\tHSADDCOUNT\tTIMERPLWAIT\tTIME_ADDPAGE(MS)\tTADDPAGE_TICKS\tTIME_RMPAGE(MS)\tTRMPAGE_TICKS\tTIME_SELPAGE(MS)\tTSELPAGE_TICKS\tCURR_SIZE_OF_HEAP" \
              > data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
        2) echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPERCPAGEIN\tPOACTIV\tRAMUSAGE\tHSADDTS\tHSADDCOUNT\tTIMERPLWAIT\tTIME_ADDPAGE(MS)\tTADDPAGE_TICKS\tTIME_RMPAGE(MS)\tTRMPAGE_TICKS\tTIME_SELPAGE(MS)\tTSELPAGE_TICKS\tCURR_SIZE_OF_HEAP" \
              > data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
        *) echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER" \
              > data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
        esac
        for n in ${NB_THREADS[@]}
        do
          echo "$sol ${test[$i]} ${memory[$i]} $n"
          ### TODO: this is not bullet proof! would be better to produce a file than stdout
          # try 3 times
          timeout 3m ${test[$i]} ${memory[$i]} $n > out.txt
          # if [ "$?" -ne "0" ]
          # then
          #   timeout 10m ${test[$i]} $n > out.txt
          #   if [ "$?" -ne "0" ]
          #   then
          #     timeout 10m ${test[$i]} $n > out.txt
          #   fi
          # fi
          cat out.txt | head
          cat out.txt | tail
          # cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  | tail -n 1 \
            # >> data/${test_name[$i]}_${sol}_s${s}.tsv
          case $DEFAULT_PRESET in
          6) cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 1 \
              | awk '/[|]/{if(NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
          5) cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 2 \
              | awk '/[|]/{if(NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
          4) cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 2 \
              | awk '/[|]/{if (NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
          3) cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 2 \
              | awk '/[|]/{if (NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
          2) cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 2 \
              | awk '/[|]/{if (NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
          *) cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  | tail -n 1 \
              >> data/${test_name[$i]}_${sol}_s${s}.tsv
          esac
          
          echo $(tail -n 1 data/${test_name[$i]}_${sol}_s${s}.tsv)
          pkill "${test[$i]} ${memory[$i]} $n"
        done
      done
      sol2=$(tr -dc '[:upper:]\n\r' <<< $sol)
      sol3=$(sed -e 's/CWM//g' <<< "${sol2}")
      sol4=$(sed -e 's/TM//g' <<< "${sol3}")
      mkdir -p data${DEFAULT_PRESET}${sol4}${NB_WAREHOUSES}
      mkdir -p data${DEFAULT_PRESET}${sol4}${NB_WAREHOUSES}/data
      mv data/* data${DEFAULT_PRESET}${sol4}${NB_WAREHOUSES}/data/
    done
  done
done

# for i in $(seq $NB_BENCHES)
# do
#   # useEpochCommit1 --> blocks
#   # useHTM useLogicalClocks usePhysicalClocks usePCWM2 usePCWM useCcHTM
#   #usePHTM useHTM useLogicalClocks usePhysicalClocks usePCWM2 usePCWM
#   # usePCWM usePCWM2 usePHTM useLogicalClocks usePhysicalClocks
#     # useLogicalClocks usePhysicalClocks usePCWM usePCWM2 usePCWM3 useCcHTMbest
#   for s in $(seq $NB_SAMPLES)
#   do
#     for sol in useCrafty
#     do
#     #LOG_REPLAY_BUFFER_WBINVD
#       echo "DISABLE_LOG_REPLAY FLUSH_LAT=0 ${sol}=1 \
#         ERROR_FILE=./data/err${test_name[$i]}_${sol}_s${s}.tsv \
#         PROFILE_FILE=./data/prof${test_name[$i]}_${sol}_s${s}.tsv " \
#         > nvhtm_params.txt
#       echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER" \
#         > data/${test_name[$i]}_${sol}_s${s}.tsv
#       #  40 48 56 64
#       for n in 1 2 4 8 12 16 20 24 28 32 33 40 48 64
#       do
#         echo "${test[$i]} $n"
#         ### TODO: this is not bullet proof! would be better to produce a file than stdout
#         timeout 10m ${test[$i]} $n > out.txt
#         if [ "$?" -ne "0" ]
#         then
#           timeout 10m ${test[$i]} $n > out.txt
#           if [ "$?" -ne "0" ]
#           then
#             timeout 10m ${test[$i]} $n > out.txt
#           fi
#         fi
#         cat out.txt | head
#         cat out.txt | tail
#         cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  | tail -n 1 \
#           >> data/${test_name[$i]}_${sol}_s${s}.tsv
#         echo $(tail -n 1 data/${test_name[$i]}_${sol}_s${s}.tsv)
#         pkill "${test[$i]} $n"
#       done
#     done
#   done
# done
#
# cd ../../deps/tinystm ; make clean ; make ; cd - ; ./build-tpcc.sh pstm
# mkdir -p ./data
#
# for s in $(seq $NB_SAMPLES)
# do
#   # useEpochCommit1 --> blocks
#   # useHTM useLogicalClocks usePhysicalClocks usePCWM2 usePCWM useCcHTM
#   #usePHTM useHTM useLogicalClocks usePhysicalClocks usePCWM2 usePCWM
#   # usePCWM usePCWM2 usePHTM useLogicalClocks usePhysicalClocks
#   for i in $(seq $NB_BENCHES)
#   do
#     for sol in usePSTM
#     do
#     #LOG_REPLAY_BUFFER_WBINVD
#       echo "DISABLE_LOG_REPLAY FLUSH_LAT=0 ${sol}=1 \
#         ERROR_FILE=./data/err${test_name[$i]}_${sol}_s${s}.tsv \
#         PROFILE_FILE=./data/prof${test_name[$i]}_${sol}_s${s}.tsv " \
#         > nvhtm_params.txt
#       echo -e "THREADS\tTIME\tCOMMITS\tABORTS" \
#         > data/${test_name[$i]}_${sol}_s${s}.tsv
#       #  40 48 56 64
#       for n in 1 2 4 8 12 16 20 24 28 32 33 40 48 64
#       do
#         echo "${test[$i]} $n"
#         ### TODO: this is not bullet proof! would be better to produce a file than stdout
#         timeout 10m ${test[$i]} $n > out.txt
#         if [ "$?" -ne "0" ]
#         then
#           timeout 10m ${test[$i]} $n > out.txt
#           if [ "$?" -ne "0" ]
#           then
#             timeout 10m ${test[$i]} $n > out.txt
#           fi
#         fi
#         cat out.txt | head
#         cat out.txt | tail
#         cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*$/p' -n  | tail -n 1 \
#           >> data/${test_name[$i]}_${sol}_s${s}.tsv
#         echo $(tail -n 1 data/${test_name[$i]}_${sol}_s${s}.tsv)
#       done
#     done
#   done
# done
