#50% memory
# test[1]="./labyrinth/labyrinth -i labyrinth/inputs/random-x48-y48-z3-n64 -h65536 -t"
# test[2]="./genome/genome -g16384 -s64 -n16777216 -h701442213 -t" 
# test[3]="./vacation/vacation -n2 -q90 -u98 -r1048576 -t4194304 -h307232768 -c"  # low
# test[4]="./intruder/intruder -a10 -l128 -n262144 -s1 -h356515840 -t"
# test[5]="./kmeans/kmeans -m1000 -n1000 -t0.00001 -i ./kmeans/inputs/random-n65536-d32-c16 -h681574 -p" # low
# test[6]="./kmeans/kmeans -m40 -n40 -t0.00001 -i ./kmeans/inputs/random-n65536-d32-c16 -h409600 -p" # low
# test[7]="./yada/yada -a15 -i ./yada/inputs/ttimeu1000000.2 -h1340253367 -t"
# test[8]="./ssca2/ssca2 -s20 -i1.0 -u1.0 -l3 -p3 -h1547045090 -t"
# NB_BENCHES=8

#75% memory
# test[1]="./labyrinth/labyrinth -i labyrinth/inputs/random-x48-y48-z3-n64 -h98304 -t"
# test[2]="./genome/genome -g16384 -s64 -n16777216 -h1052163319 -t" 
# test[3]="./vacation/vacation -n2 -q90 -u98 -r1048576 -t4194304 -h460849152 -c"  # low
# test[4]="./intruder/intruder -a10 -l128 -n262144 -s1 -h534773760 -t"
# test[5]="./kmeans/kmeans -m1000 -n1000 -t0.00001 -i ./kmeans/inputs/random-n65536-d32-c16 -h1022361 -p" # low
# test[6]="./kmeans/kmeans -m40 -n40 -t0.00001 -i ./kmeans/inputs/random-n65536-d32-c16 -h614400 -p" # low
# test[7]="./yada/yada -a15 -i ./yada/inputs/ttimeu1000000.2 -h2010380051 -t"
# test[8]="./ssca2/ssca2 -s20 -i1.0 -u1.0 -l3 -p3 -h2320567635 -t"
# NB_BENCHES=8

#args for paging tests (article) "100% memory"
test[1]="./labyrinth/labyrinth -i labyrinth/inputs/random-x48-y48-z3-n64 -h131072 -t"
test[2]="./genome/genome -g16384 -s64 -n16777216 -h1402884425 -t" 
test[3]="./vacation/vacation -n2 -q90 -u98 -r1048576 -t4194304 -h614465536 -c"  # low
test[4]="./intruder/intruder -a10 -l128 -n262144 -s1 -h713031680 -t"
test[5]="./kmeans/kmeans -m1000 -n1000 -t0.00001 -i ./kmeans/inputs/random-n65536-d32-c16 -h1363148 -p" # low
test[6]="./kmeans/kmeans -m40 -n40 -t0.00001 -i ./kmeans/inputs/random-n65536-d32-c16 -h819200 -p" # low
test[7]="./yada/yada -a15 -i ./yada/inputs/ttimeu1000000.2 -h2680506734 -t"
test[8]="./ssca2/ssca2 -s20 -i1.0 -u1.0 -l3 -p3 -h3094090180 -t"
NB_BENCHES=8

# test[1]="./ssca2/ssca2 -s18 -i1.0 -u1.0 -l9 -p9 -t"
# test[2]="./kmeans/kmeans -m500 -n500 -t0.0009 -i kmeans/inputs/random-n65536-d32-c16 -p " # low
# test[3]="./genome/genome -g65536 -s512 -n8192 -t"
# test[4]="./vacation/vacation -n1 -q60 -u98 -r16384 -t4194304 -c"  # low
# test[5]="./intruder/intruder -a20 -l128 -n65536 -s1 -t"
# test[6]="./yada/yada -a20 -i yada/inputs/ttimeu100000.2 -t"
# test[7]="./vacation/vacation -n4 -q60 -u90 -r1048576 -t4194304 -c"  # high
# test[8]="./kmeans/kmeans -m15 -n15 -t0.00001 -i kmeans/inputs/random-n65536-d32-c16 -p " # high
# test[10]="./bayes/bayes -v32 -r4096 -n2 -p20 -i2 -e2 -t"

# # # drop labyrith, it does not have much info

test_name[1]="LABYRINTH"
test_name[2]="GENOME"
test_name[3]="VACATION_LOW"
test_name[4]="INTRUDER"
test_name[5]="KMEANS_VLOW"
test_name[6]="KMEANS_LOW"
test_name[7]="YADA"
test_name[8]="SSCA2"
test_name[9]="VACATION_HIGH"
test_name[10]="KMEANS_HIGH"
#       # labyrinth
# ### not used
# test_name[10]="BAYES"

# NB_BENCHES=6
