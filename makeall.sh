#!/bin/bash

####################
#SETTINGS
    #0=full compilation
    #1=replayer only
    #2=paging only
    #3=replayer+paging
    #4=replayer+paging+hashmap
    #5=replayer+paging+swap
    #6=all off

setting=4 #default
ASYNC_PAGER=1
PAGE_INDEX=1
####################

if [ $1 -ge 0 ] || [ $1 -le 6 ] #ensures its in the 0..5 range
then
    setting=$1
fi

if [ $setting -ne 0 ]
then
    start=$setting
    end=$setting
fi
if [ $setting -eq 0 ]
then
    start=1
    end=6
fi

for (( compilation=$start; compilation<=$end; compilation++ ))
do
    case $compilation in
        1) run="REPLAYER=1 PAGING=0 HASHMAP=0 ASYNC_PAGER=${ASYNC_PAGER} PAGE_INDEX=${PAGE_INDEX}" ;;
        2) run="REPLAYER=1 PAGING=1 HASHMAP=2 ASYNC_PAGER=${ASYNC_PAGER} PAGE_INDEX=${PAGE_INDEX}" ;; #simplehash
        3) run="REPLAYER=1 PAGING=1 HASHMAP=0 ASYNC_PAGER=${ASYNC_PAGER} PAGE_INDEX=${PAGE_INDEX}" ;;
        4) run="REPLAYER=1 PAGING=1 HASHMAP=1 ASYNC_PAGER=${ASYNC_PAGER} PAGE_INDEX=${PAGE_INDEX}" ;;
        5) run="REPLAYER=1 PAGING=1 SWAP=1 ASYNC_PAGER=${ASYNC_PAGER} PAGE_INDEX=${PAGE_INDEX}" ;;
        6) run="REPLAYER=0 PAGING=0 HASHMAP=0 ASYNC_PAGER=${ASYNC_PAGER} PAGE_INDEX=${PAGE_INDEX}" ;; #has to be last
    esac
    #2, 3, 4, 5 == #PAGING=1
    cd deps/htm_alg
    make clean && make

    cd ../input_handler
    make clean && make

    cd ../threading
    make clean && make

    cd ../tinystm
    mkdir lib
    make clean && make

    cd ../../nvhtm
    ./get_cpu_data.sh

    make clean && make $run

    cp CPU_FREQ_kHZ.txt ../bench/datastructures
    cp CPU_FREQ_kHZ.txt ../bench/tpcc/code

    cd ../bench/datastructures/
    ./clean-datastructures.sh
    ./build-datastructures.sh nvhtm

    sleep 2
    cd ../tpcc
    case $compilation in
        1) ./build-tpcc.sh nvhtm 0 1 ;; #paging=0 replayer=1
        2) ./build-tpcc.sh nvhtm 1 1 ;; #paging=1 replayer=1
        3) ./build-tpcc.sh nvhtm 1 1 ;; #paging=1 replayer=1
        4) ./build-tpcc.sh nvhtm 1 1 ;; #paging=1 replayer=1
        5) ./build-tpcc.sh nvhtm 1 1 ;; #paging=1 replayer=1
        6) ./build-tpcc.sh nvhtm 0 0 ;; #has to be last
    esac

    # cd ../stamp
    # ########### CLEAN STAMP #############
    # if [ $compilation -eq 0 ] || [ $setting -ne 0 ]
    # then
    #     rm bayes/*.o
    #     rm genome/*.o
    #     rm intruder/*.o
    #     rm vacation/*.o
    #     rm ssca2/*.o
    #     rm kmeans/*.o
    #     rm yada/*.o
    #     rm labyrinth/*.o

    #     rm -rf bayes_r
    #     rm -rf genome_r
    #     rm -rf intruder_r
    #     rm -rf vacation_r
    #     rm -rf ssca2_r
    #     rm -rf kmeans_r
    #     rm -rf yada_r
    #     rm -rf labyrinth_r

    #     rm -rf bayes_p
    #     rm -rf genome_p
    #     rm -rf intruder_p
    #     rm -rf vacation_p
    #     rm -rf ssca2_p
    #     rm -rf kmeans_p
    #     rm -rf yada_p
    #     rm -rf labyrinth_p

    #     rm -rf bayes_rp
    #     rm -rf genome_rp
    #     rm -rf intruder_rp
    #     rm -rf vacation_rp
    #     rm -rf ssca2_rp
    #     rm -rf kmeans_rp
    #     rm -rf yada_rp
    #     rm -rf labyrinth_rp

    #     rm -rf bayes_rph
    #     rm -rf genome_rph
    #     rm -rf intruder_rph
    #     rm -rf vacation_rph
    #     rm -rf ssca2_rph
    #     rm -rf kmeans_rph
    #     rm -rf yada_rph
    #     rm -rf labyrinth_rph

    #     rm -rf bayes_rps
    #     rm -rf genome_rps
    #     rm -rf intruder_rps
    #     rm -rf vacation_rps
    #     rm -rf ssca2_rps
    #     rm -rf kmeans_rps
    #     rm -rf yada_rps
    #     rm -rf labyrinth_rps
    # fi

    # ##################################

    # case $compilation in
    #     1) ./build-stamp.sh nvhtm 0 ;; #paging=0
    #     2) ./build-stamp.sh nvhtm 1 ;; #paging=1
    #     3) ./build-stamp.sh nvhtm 1 ;; #paging=1
    #     4) ./build-stamp.sh nvhtm 1 ;;
    #     5) ./build-stamp.sh nvhtm 1 ;;
    #     6) ./build-stamp.sh nvhtm 0 ;; #has to be last
    # esac
    
    # if [ $setting -eq 0 ]
    # then
    #     case $compilation in 
    #     1)
    #         cp -r bayes bayes_r
    #         cp -r genome genome_r
    #         cp -r intruder intruder_r
    #         cp -r vacation vacation_r
    #         cp -r ssca2 ssca2_r
    #         cp -r kmeans kmeans_r
    #         cp -r yada yada_r
    #         cp -r labyrinth labyrinth_r ;;
    #     2)
    #         cp -r bayes bayes_p
    #         cp -r genome genome_p
    #         cp -r intruder intruder_p
    #         cp -r vacation vacation_p
    #         cp -r ssca2 ssca2_p
    #         cp -r kmeans kmeans_p
    #         cp -r yada yada_p
    #         cp -r labyrinth labyrinth_p ;;
    #     3)
    #         cp -r bayes bayes_rp
    #         cp -r genome genome_rp
    #         cp -r intruder intruder_rp
    #         cp -r vacation vacation_rp
    #         cp -r ssca2 ssca2_rp
    #         cp -r kmeans kmeans_rp
    #         cp -r yada yada_rp
    #         cp -r labyrinth labyrinth_rp ;;
    #     4)
    #         cp -r bayes bayes_rph
    #         cp -r genome genome_rph
    #         cp -r intruder intruder_rph
    #         cp -r vacation vacation_rph
    #         cp -r ssca2 ssca2_rph
    #         cp -r kmeans kmeans_rph
    #         cp -r yada yada_rph
    #         cp -r labyrinth labyrinth_rph ;;
    #     5)
    #         cp -r bayes bayes_rps
    #         cp -r genome genome_rps
    #         cp -r intruder intruder_rps
    #         cp -r vacation vacation_rps
    #         cp -r ssca2 ssca2_rps
    #         cp -r kmeans kmeans_rps
    #         cp -r yada yada_rps
    #         cp -r labyrinth labyrinth_rps ;;
    #     esac
    # fi

    # cd ../../nvhtm
    # ./get_cpu_data.sh
    # make clean && make

    # cd ../bench/datastructures/
    # ./build-datastructures.sh nvhtm

    # cd ../stamp
    # ./build-stamp.sh nvhtm

    echo "$compilation $run"
done

echo "Check PM location:"
mount | grep ext4
echo ""
echo "Verify if command has been executed:" 
echo "sudo sysctl -w vm.max_map_count=500000(0)"