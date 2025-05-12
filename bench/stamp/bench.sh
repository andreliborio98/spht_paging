#!/bin/bash

source benches_args.sh

#SETTINGS
####################
NB_THREADS="1 2 4 8 12 16 20 24"
NB_SAMPLES=10
PINNING=1
NB_REPLAYERS=1
LOG_REPLAY_CONCURRENT=1 #0 or 1
FLUSH_LAT=0
NB_BENCHES=8
SOLUTIONS="usePCWM" #useSharedHTM usePCWMeADRT1 usePCWMeADR usePCWM useHTM
DEFAULT_PRESET=4 
    #0=full compilation
    #1=replayer only
    #2=paging only
    #3=replayer+paging
    #4=replayer+paging+hashmap
    #5=replayer+paging+swap
    #6=all off
#############

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
  LOG_REPLAY_CONCURRENT=0
else
    if [ $NB_REPLAYERS -eq 0 ]
    then
        NB_REPLAYERS=1
    fi
fi

cd ../../
./makeall.sh $DEFAULT_PRESET
cd -

for (($DEFAULT_PRESET = $start; $DEFAULT_PRESET <= $end; $DEFAULT_PRESET++))
do
    if [ $DEFAULT_PRESET -eq 0 ]
    then
        case $DEFAULT_PRESET in 
            1) run="_r" ;;
            2) run="_p" ;;
            3) run="_rp" ;;
            4) run="_rph" ;;
            5) run="_rps" ;;
            6) run="" ;;
        esac
    else
        run="";
    fi
  
    mkdir -p data${run}
  
    for i in $(seq $NB_BENCHES)
    do
        for s in $(seq $NB_SAMPLES)
        do
            for sol in ${SOLUTIONS[@]}
            do
                echo "PINNING=${PINNING} FLUSH_LAT=${FLUSH_LAT} ${sol}=1 NB_REPLAYERS=${NB_REPLAYERS} LOG_REPLAY_CONCURRENT=${LOG_REPLAY_CONCURRENT}\
                      ERROR_FILE=./data${run}/err${test_name[$i]}_${sol}_s${s}.tsv \
                      PROFILE_FILE=./data${run}/prof${test_name[$i]}_${sol}_s${s}.tsv " \
                      > nvhtm_params.txt
                
                case $DEFAULT_PRESET in
                6) echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPOACTIV\tRAMUSAGE\tHSADDTS\tHSADDCOUNT" \
                      > data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
                5) echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPOACTIV\tRAMUSAGE\tADDREPMEM\tREPACTIV\tHSADDTS\tHSADDCOUNT\tSWAPCOUNT" \
                      > data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
                4) echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPOACTIV\tRAMUSAGE\tHSADDTS\tHSADDCOUNT\tTIMERPLWAIT\tPGOUTPOSTINIT\tSWAPCOUNT" \
                      > data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
                3) echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPOACTIV\tRAMUSAGE\tHSADDTS\tHSADDCOUNT\tTIMERPLWAIT\tPGOUTPOSTINIT\tSWAPCOUNT" \
                      > data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
                *) echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPOACTIV\tRAMUSAGE\tADDREPMEM\tREPACTIV\tHSADDTS\tHSADDCOUNT" \
                      > data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
                esac

                # if [ $DEFAULT_PRESET -eq 5 ]
                # then
                #     echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPOACTIV\tRAMUSAGE\tHSADDTS\tHSADDCOUNT" \
                #       > data${run}/${test_name[$i]}_${sol}_s${s}.tsv
                # elif [ $DEFAULT_PRESET -eq 4 ]
                # then
                #     echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPOACTIV\tRAMUSAGE\tADDREPMEM\tREPACTIV\tHSADDTS\tHSADDCOUNT\tSWAPCOUNT" \
                #       > data${run}/${test_name[$i]}_${sol}_s${s}.tsv
                # elif [ $DEFAULT_PRESET -eq 3 ]
                # then #remove 1 parameter later (its for testing also with swap on)
                #     echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPOACTIV\tRAMUSAGE\tHSADDTS\tHSADDCOUNT\tTIMERPLWAIT\tPGOUTPOSTINIT\tSWAPCOUNT" \
                #       > data${run}/${test_name[$i]}_${sol}_s${s}.tsv
                # else
                #     echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC\tNB_EXPLI\tNB_OTHER\tPAGEIN\tPAGEOUT\tPOACTIV\tRAMUSAGE\tADDREPMEM\tREPACTIV\tHSADDTS\tHSADDCOUNT" \
                #       > data${run}/${test_name[$i]}_${sol}_s${s}.tsv
                # fi

                for n in ${NB_THREADS[@]}
                do
                    echo "${test[$i]} $n"
                    timeout 5m ${test[$i]} $n > out.txt
                    if [ $DEFAULT_PRESET -eq 3 ]
                    then
                        sed -n '/replayerStats/{n;p;}' out.txt >> out.txt
                    fi
                    cat out.txt | head
                    cat out.txt | tail

                    case $DEFAULT_PRESET in
                    6) cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 2 \
                        | awk '/[|]/{if(NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
                    5) cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 2 \
                        | awk '/[|]/{if (NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
                    4) cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 3 \
                        | awk '/[|]/{if (NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
                    3) cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 3 \
                        | awk '/[|]/{if (NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
                    *) cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 3 \
                        | awk '/[|]/{if (NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv ;;
                    esac

                    # if [ $DEFAULT_PRESET -eq 5 ]
                    # then 
                    #     cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 2 \
                    #     | awk '/[|]/{if(NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv
                    # elif [ $DEFAULT_PRESET -eq 4 ] # -eq 3 is TEMPORARY!!!!
                    # then
                    #     cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 2 \
                    #     | awk '/[|]/{if (NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv
                    # elif [ $DEFAULT_PRESET -eq 3 ]
                    # then #remove 1 parameter later (its for testing also with swap on)
                    #     cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 2 \ 
                    #     | awk '/[|]/{if (NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv
                    # else
                    #     cat out.txt | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' | tail -n 3 \
                    #     | awk '/[|]/{if (NR>1)print p; p=$0; next}{p=p FS $0} END{if(NR)print p}' >> data${run}/${test_name[$i]}_${sol}_s${s}.tsv
                    # fi
                    # | sed -E 's/ ([^ ]*)$/\t\1/' data/${test_name[$i]}_${sol}_s${s}.tsv > data/${test_name[$i]}_${sol}_s${s}.tsv
                    # | awk '/[|]/{if(NR>1)print p; $1=$1; p=$0; next}{$1=$1; p=p FS $0} END{if(NR)print p}' >> data/${test_name[$i]}_${sol}_s${s}.tsv
                    echo $(tail -n 1 data${run}/${test_name[$i]}_${sol}_s${s}.tsv)
                    pkill "${test[$i]} $n"
                done
            done
        done
    done
done