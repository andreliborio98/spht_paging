#!/bin/bash

source paths.sh

### Perf setup (pls review for your machine)
##### intel14_v1
KERNEL_VERSION="4.15.0-58"
##### saturn1
#KERNEL_VERSION="4.15.0-65"

PERF_SETUP=" \
	-v /usr/bin/perf:/usr/bin/perf \
	-v /usr/bin/lsb_release:/usr/bin/lsb_release \
	-v /usr/bin/python3:/usr/bin/python3 \
	-v /usr/share/pyshared:/usr/share/pyshared \
	-v /usr/lib/python36.zip:/usr/lib/python36.zip \
	-v /usr/lib/python3.6:/usr/lib/python3.6 \
	-v /usr/lib/python3.6/lib-dynload:/usr/lib/python3.6/lib-dynload \
	-v /usr/local/lib/python3.6/dist-packages:/usr/local/lib/python3.6/dist-packages \
	-v /usr/lib/python3/dist-packages:/usr/lib/python3/dist-packages \
	-v /usr/share/distro-info:/usr/share/distro-info \
	-v /usr/lib/x86_64-linux-gnu/libbfd-2.30-system.so:/usr/lib/x86_64-linux-gnu/libbfd-2.30-system.so \
	-v /usr/lib/linux-tools/${KERNEL_VERSION}-generic:/usr/lib/linux-tools/${KERNEL_VERSION}-generic \
	-v /usr/lib/linux-tools-${KERNEL_VERSION}:/usr/lib/linux-tools-${KERNEL_VERSION} "

./get_cpu_data.sh 
if [[ "$(docker images -q nvhtm_dcastro 2> /dev/null)" == "" ]]
then
	docker login && docker build -t nvhtm_dcastro:latest .
fi

if [[ "$(docker container list | grep nvhtm_container)" == "" ]]
then
		docker run \
			--cap-add SYS_PTRACE --security-opt seccomp=unconfined \
			-v /home/dcastro/$DM/nvhtm:/root/$DM/nvhtm \
			-v /home/dcastro/$DM/deps/arch_dep:/root/$DM/deps/arch_dep \
			-v /home/dcastro/$DM/deps/htm_alg:/root/$DM/deps/htm_alg \
			-v /home/dcastro/$DM/deps/threading:/root/$DM/deps/threading \
			-v /home/dcastro/$DM/deps/input_handler:/root/$DM/deps/input_handler \
			$PERF_SETUP \
			--name nvhtm_container -it --rm nvhtm_dcastro
fi
