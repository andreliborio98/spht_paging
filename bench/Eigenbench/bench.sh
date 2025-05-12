#!/bin/bash

source benches_args.sh

cd ../../nvhtm
make clean
make -j40 OPTIMIZE=1
cd -

### this file must exist here
cp ../../nvhtm/CPU_FREQ_kHZ.txt .

make clean ; make OPTIMIZE=1
mkdir -p ./data

NB_SAMPLES=3

for s in $(seq $NB_SAMPLES)
do
  # useEpochCommit1 --> blocks
  # useHTM useLogicalClocks usePCWC-F usePCWM
  for sol in useHTM usePhysicalClocks usePCWM
  do
    for R in 4 16 64
    do
      for W in 2 4 8
      do
        echo "FLUSH_LAT=0 ${sol}=1 ERROR_FILE=./data/err_${sol}_R${R}_W${W}_s${s}.tsv PROFILE_FILE=./data/prof_${sol}_R${R}_W${W}_s${s}.tsv " \
          > nvhtm_params.txt
        echo -e "THREADS\tTIME\tNB_HTM_SUCCESS\tNB_FALLBACK\tNB_ABORTS\tNB_CONFL\tNB_CAPAC" \
          > data/${sol}_R${R}_W${W}_s${s}.tsv
        for N in 1 2 4 8 16 32
        do
          echo "./bench N=$N W=$W R=$R"
          ### TODO: this is not bullet proof! would be better to produce a file than stdout
          ./bench loops=100000 persist=0 lct=0.0 R1=0 W1=0 R2=0 W2=0 seed=123456 Nop_i=0 Nop_o=0  R3_o=0 W3_o=0 k_i=1 k_o=0 \
            N=$N R3_i=$R W3_i=$W A1=8 A2=1024 A3=1048576 \
            | sed '/^[0-9]*\t[0-9]*.[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*\t[0-9]*$/p' -n  | tail -n 1 \
            >> data/${sol}_R${R}_W${W}_s${s}.tsv
          echo $(tail -n 1 data/${sol}_R${R}_W${W}_s${s}.tsv)
        done
      done
    done
  done
done
