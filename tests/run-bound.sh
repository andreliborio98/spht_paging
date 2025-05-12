#!/bin/bash

REPTHREADS="1 2 4 8 16 32"
#REPTHREADS="1 2 4"
REP="5"
NUMA_OPT="1 2"

APPS="bt-st bt-nt bt-wbinvd bt-allf"
#APPS="bt-st"

# total log size
LOGSIZE=$(( 1024*1024*1024 ))
HEAPSIZE=$((1024*1024*512 ))
#MEMSIZE=$(( 1024*1024 ))

outname="out"

OUTDIR="output-bound"


mkdir -p $OUTDIR

for app in $APPS; do

    for numa in $NUMA_OPT; do

      for t in $REPTHREADS; do

#        FILENAME="$OUTDIR/$app-1GB-$numa-$t"
        FILENAME="$OUTDIR/$app-$LOGSIZE-$HEAPSIZE-0-$numa-$numa-64-$t"
        rm -rf $FILENAME

        echo "$app"
        for rep in $(seq 1 $REP); do

          echo "  $t"
          ./$app $t $numa  >> $FILENAME

        done
    done
  done
done

