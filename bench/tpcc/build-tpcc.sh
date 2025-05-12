backend=$1 # e.g: nvhtm
paging=$2 #0(disabled) or 1(enabled)
replayer=$3

rm lib/*.o || true

rm Defines.common.mk
rm Makefile
rm Makefile.flags
rm include/thread.h
rm src/thread.c
rm include/tm.h

cp ../backends/$backend/tm.h ./include/
cp ../backends/$backend/thread.c ./src/
cp ../backends/$backend/thread.h ./include/
cp ../backends/$backend/Makefile .
# cp ../backends/$backend/Makefile.common .
cp ../backends/$backend/Makefile.flags .
cp ../backends/$backend/Defines.common.mk .

rm $(find . -name *.o)

cd code;
rm tpcc

CPU_FREQ=$(cat CPU_FREQ_kHZ.txt | tr -d '[:space:]')
make_command="make -j8 -f Makefile DEF_CPU_FREQ=$CPU_FREQ PAGING=$paging REPLAYER=$replayer $MAKEFILE_ARGS"
echo " ==========> $make_command"
$make_command
echo " ==========> Check "nvhtm_params.txt" for REPLAYER and SOLUTION"
