rm benches_args.sh
MEM_PERC=$1
MEM_MULTI=$(echo "scale=2 ; $MEM_PERC / 1000" | bc)
NB_WAREHOUSES=32

############ REFERENCE ############
# -m: max number of warehouses, >= warehouses
# -w: number of warehouses
# -t: time
# -d: delivery
# -r: new order
# -o: order status
# -p: payments
# -s: stock level

############# FINAL PARAMS ############
#TLDR ==> 10S + 30S WARMUP (hardcoded)
#CONFIG		        100%MEM	        WPAGEOUTMEM(1-24t)
#2R+3D+95P    		100%=2gb 		-> 80%…             #same settings as used in the spht paper
#50O+50P      		100%=1,35gb 	-> 95%…
#50D+25O+25P	    100%=1,35gb 	-> 95%…
#25D+50O+25P	    100%=1,35gb		-> 95%…
#4S+45R+4D+4O+43P   100%=2,7gb      -> 81%…             #as stated in TPCC docs (page 73)

############# CONFIGS ############

## BATCH 2 ##

#SETTINGS
cat > benches_args.sh <<EOL

NB_BENCHES=2
NB_WAREHOUSES=32

test[1]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 10 -s 0 -r 2 -d 3 -o 0 -p 95 -h"
test[2]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 10 -s 4 -r 45 -d 4 -o 4 -p 43 -h" #tpcc specification suggestion (page 71)

memory[1]="$(printf %.f $(echo "2147483648 * $MEM_MULTI" | bc -l)) -n" #100% (1024^3)*2
memory[2]="$(printf %.f $(echo "2899102925 * $MEM_MULTI" | bc -l)) -n" #100% (1024^3)*2,7

test_name[1]="TPCC_PAYMENT"
test_name[2]="TPCC_DocsA"

EOL