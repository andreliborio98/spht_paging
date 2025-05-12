#SETTINGS
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
#4S+45R+4D+4O+43P   100%=2,6gb      -> 83%…             #as stated in TPCC docs (page 73)

############# CONFIGS ############

NB_BENCHES=5

#############
## BATCH 1 ##

# NB_BENCHES=3

test[1]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 10 -s 0 -r 0 -d 0 -o 50 -p 50 -h"
test[2]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 10 -s 0 -r 0 -d 50 -o 25 -p 25 -h"
test[3]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 10 -s 0 -r 0 -d 25 -o 50 -p 25 -h"

memory[1]="1449551462 -n" #100% (1024^3)*1,35
memory[2]="1449551462 -n" #100% (1024^3)*1,35
memory[3]="1449551462 -n" #100% (1024^3)*1,35

# memory[1]="1413312676 -n" #97,5% (1024^3)*1,35*0,975
# memory[2]="1413312676 -n" #97,5% (1024^3)*1,35*0,975
# memory[3]="1413312676 -n" #97,5% (1024^3)*1,35*0,975

# memory[1]="1377073889 -n" #95% (1024^3)*1,35*0,95
# memory[2]="1377073889 -n" #95% (1024^3)*1,35*0,95
# memory[3]="1377073889 -n" #95% (1024^3)*1,35*0,95

# memory[1]="1340835103 -n" #92,5% (1024^3)*1,35*0,925
# memory[2]="1340835103 -n" #92,5% (1024^3)*1,35*0,925
# memory[3]="1340835103 -n" #92,5% (1024^3)*1,35*0,925

# memory[1]="1304596316 -n" #90% (1024^3)*1,35*0,9
# memory[2]="1304596316 -n" #90% (1024^3)*1,35*0,9
# memory[3]="1304596316 -n" #90% (1024^3)*1,35*0,9

test_name[1]="TPCC_ORDSTATUSnPAY"
test_name[2]="TPCC_ORDSTnPAYn50DEL"
test_name[3]="TPCC_50ORDSTnPAYnDEL"

#############
## BATCH 2 ##

# NB_BENCHES=2

test[4]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 10 -s 0 -r 2 -d 3 -o 0 -p 95 -h"
test[5]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 10 -s 4 -r 45 -d 4 -o 4 -p 43 -h" #tpcc specification suggestion (page 71)

memory[4]="2147483648 -n" #100% (1024^3)*2
memory[5]="2899102925 -n" #100% (1024^3)*2,7

# memory[4]="1717986918 -n" #80% (1024^3)*2*0,8
# memory[5]="2319282340 -n" #80% (1024^3)*2,7*0,8

# memory[4]="1288490189 -n" #60% (1024^3)*2*0,6
# memory[5]="1739461755 -n" #60% (1024^3)*2,7*0,6

# memory[4]="858993459 -n" #40% (1024^3)*2*0,4
# memory[5]="1159641170 -n" #40% (1024^3)*2,7*0,4

test_name[4]="TPCC_PAYMENT"
test_name[5]="TPCC_DocsA"

##################################

# test[1]="./code/tpcc -m10 -w10 -t 10 -s 0 -r 0 -d 20 -o 65 -p 15 -n"
# test[2]="./code/tpcc -m25 -w25 -t 10 -s 0 -r 2 -d 2  -o 26 -p 70 -n"

# test[1]="./code/tpcc -m32 -w32 -t 10 -s 0 -r 2 -d 3 -o 0 -p 95 -n"
# test[2]="./code/tpcc -m32 -w32 -t 10 -s 0 -r 3 -d 7 -o 0 -p 90 -n"
# test[3]="./code/tpcc -m32 -w32 -t 10 -s 0 -r 1 -d 1 -o 0 -p 98 -n"

# test[1]="./code/tpcc -m128 -w128 -t 10 -s 0 -r 2 -d 3 -o 0 -p 95 -n"
# test[2]="./code/tpcc -m128 -w128 -t 10 -s 0 -r 3 -d 7 -o 0 -p 90 -n"
# test[3]="./code/tpcc -m128 -w128 -t 10 -s 0 -r 1 -d 1 -o 0 -p 98 -n"

#PREVIOUSLY ENABLED
# test[1]="./code/tpcc -m64 -w64 -t 10 -s 0 -r 2 -d 3 -o 0 -p 95 -n"
# test[2]="./code/tpcc -m64 -w64 -t 10 -s 0 -r 3 -d 7 -o 0 -p 90 -n"
# test[3]="./code/tpcc -m64 -w64 -t 10 -s 0 -r 1 -d 1 -o 0 -p 98 -n"

# test[1]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 100 -s 0 -r 2 -d 3 -o 0 -p 95 -h" # 5294967296 -n" #same settings as the spht paper -> payment focus
# # test[2]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 10 -s 0 -r 95 -d 3 -o 0 -p 2 -n" #neworder focus
# # test[3]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 10 -s 95 -r 2 -d 3 -o 0 -p 0 -n" #stocklevel focus
# # test[4]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 10 -s 3 -r 2 -d 95 -o 0 -p 0 -n" #delivery focus
# test[2]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 100 -s 0 -r 2 -d 3 -o 95 -p 0 -h" # 5294967296 -n" #orderstatus focus
# # test[6]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 10 -s 50 -r 0 -d 0 -o 50 -p 0 -n" #mix stocklevel and orderstatus (read only)
# test[3]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 100 -s 0 -r 0 -d 0 -o 50 -p 50 -h" # 5294967296 -n" #mix orderstatus and payment
# # test[8]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 10 -s 0 -r 50 -d 0 -o 50 -p 0 -n" #mix neworder and orderstatus
# # test[4]="./code/tpcc -m${NB_WAREHOUSES} -w${NB_WAREHOUSES} -t 100 -s 4 -r 45 -d 4 -o 4 -p 43 -h 5294967296 -n" #tpcc specification suggestion (page 71)
# #fixed 5294967296 for no pageouts with hash
# memory[1]="644245094 -n" # having -n here simplifies the next argument in bench.sh (num threads) 
# memory[2]="1503238554 -n"
# memory[3]="1503238554 -n"
# # memory[4]="1503238554 -n"

# test_name[1]="TPCC_PAYMENT"
# # test_name[2]="TPCC_NEWORDER"
# # test_name[3]="TPCC_STOCKLEVEL"
# # test_name[4]="TPCC_DELIVERY"
# test_name[2]="TPCC_ORDERSTATUS"
# # test_name[6]="TPCC_STOCKLVnORDSTATUS"
# test_name[3]="TPCC_ORDSTATUSnPAY"
# test_name[8]="TPCC_NEWORDnORDSTATUS"
# test_name[4]="TPCC_A"
