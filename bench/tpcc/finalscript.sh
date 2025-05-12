MEM_B1="1000 975 950 925 900 850 700" #percentages *10 in case a decimal is needed
MEM_B2="1000 950 900 850 800"
CONFIG="2 4 5"

rm -rf data*
rm -rf B*
# mkdir B1_results
mkdir B2_results

mv benches_args.sh benches_argsOG.sh

for config in ${CONFIG[@]}
do
    # for mem in ${MEM_B1[@]}
    # do
    #     ./finalscript_benches_argsB1.sh "${mem}"
    #     ./bench.sh "${config}"
    #     sleep 2
    #     mkdir "B1_results/B1_${mem}%mem"
    #     mv "data${config}P32" "B1_results/B1_${mem}%mem"
    # done

    for mem in ${MEM_B2[@]}
    do
        ./finalscript_benches_argsB2.sh "${mem}"
        ./bench.sh "${config}"
        sleep 2
        mkdir "B2_results/B2_${mem}%mem"
        mv "data${config}P32" "B2_results/B2_${mem}%mem"
    done
done

mv benches_argsOG.sh benches_args.sh
