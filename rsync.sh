#!/bin/bash

source paths.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

find . -name ".DS_Store" -delete
find . -name "._*" -delete

ssh $NODE "mkdir -p $DM "
rsync -avz --exclude={'.git/*','.gitignore','.vscode/*','deps/*','nvhtm/pmdata0/*','nvhtm/pmdata1/*','nvhtm/CPU_FREQ_GHZ.txt','nvhtm/CPU_FREQ_kHZ.txt'} . $NODE:$DM

### Go over each dependency and compile


### attach to the container with 
# docker exec -it nvhtm_container bash
### stop and remove with
# docker container stop nvhtm_container && docker container rm nvhtm_container

### MAKE
# make clean ; make DEBUG=1
### RUN (epoch patient)
# ./test_spins EXPERIMENT_TIME=4000000 SPINS_EXEC=1 FLUSH_LAT=1000 spinInCycles=1 FORCE_LEARN=1 tid0Slowdown=0 usePCWC=1 NB_THREADS=14 PINNING=1
