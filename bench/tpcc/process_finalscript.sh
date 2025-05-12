for d in $(ls -d B1_results/*)
do
    cd $d
    mv data* ../../
    cd ../../
    ./process_data.sh
    mv data* "$d"/
    mv mashup* "$d"/
done

for d in $(ls -d B2_results/*)
do
    cd $d
    mv data* ../../
    cd ../../
    ./process_data.sh
    mv data* "$d"/
    mv mashup* "$d"/
done