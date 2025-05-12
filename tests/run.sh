#!/bin/bash

REPTHREADS="1 2 4 6 8 10 12 16 20 24 32 40 48"
#REPTHREADS="1 2 4"
#REP="10"
REP="5"

#APPS="replay-st replay-nt replay-dram replay-wbinvd replay-allf"
#APPS="replay-st replay-nt replay-wbinvd replay-allf"
APPS="replay-st replay-wbinvd"

# total log size
LOGSIZE=$(( 1024*1024*1024 ))
HEAPSIZE=$((1024*1024*512 ))
#LOGSIZE=$(( 1024*1024))
#HEAPSIZE=$((1024*512 ))

#LOGNUMA="1 2"
LOGNUMA="1 2"
#HEAPNUMA="1 2"
HEAPNUMA="1 2"

outname="out"

OUTDIR="output"

# 0 -> vanilla, 1 -> shards
TYPE="0"
WORKERTHRS="64"

mkdir -p $OUTDIR


# vanilla
for app in $APPS; do

  for hn in $HEAPNUMA; do
    for ln in $LOGNUMA; do

      for t in $REPTHREADS; do

        FILENAME="$OUTDIR/$app-$LOGSIZE-$HEAPSIZE-$TYPE-$hn-$ln-$WORKERTHRS-$t"
        rm -rf $FILENAME

        echo "$app"
        for rep in $(seq 1 $REP); do

          echo "  $t"
          ./$app -w $WORKERTHRS -r $t -l $TYPE -n $ln -h $hn -s $LOGSIZE -S $HEAPSIZE >> $FILENAME

        done
      done
    done
  done
done

exit 0

# shards - workers == replayers
TYPE="1"
for app in $APPS; do

  for hn in $HEAPNUMA; do
    for ln in $LOGNUMA; do

      for t in $REPTHREADS; do

        FILENAME="$OUTDIR/$app-$LOGSIZE-$HEAPSIZE-$TYPE-$hn-$ln-$t-$t"
        rm -rf $FILENAME

        echo "$app"
        for rep in $(seq 1 $REP); do

          echo "  $t"
          ./$app -w $t -r $t -l $TYPE -n $ln -h $hn -s $LOGSIZE -S $HEAPSIZE >> $FILENAME

        done
      done
    done
  done
done

