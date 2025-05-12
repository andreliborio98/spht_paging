source ../paths.sh

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

SCRIPT_PATH=$(pwd)

FOLDER=data_mini_bench_workers

cd $DATA_PATH
mkdir -p mini_bench
cd mini_bench
EXPERIMENT_FOLDER=$(date +%Y-%m-%dT%H_%m_%S)
mkdir -p $EXPERIMENT_FOLDER
cd $EXPERIMENT_FOLDER
mkdir -p data
scp $NODE:$DM/scripts/$FOLDER/* data/
#cp /home/vagrant/saved-data/* data/


echo "data is in $PWD"

cd $SCRIPT_PATH
