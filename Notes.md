# Notes

## Vagrant

- Configure Vagrant file with private network ip
        config.vm.network "private_network", ip: "192.168.33.10"

- Add vm configuration for ./ssh/config
        Host vm
            HostName 192.168.33.10
            User vagrant
            IdentityFile ~/.ssh/id_rsa

- Add public ssh key to ~/.ssh/authorized_keys on Vagrant VM

- Install required build tools on Vagrant VM
        sudo apt-get install build-essential r-base-core python3 python3-pip python3-matplotlib

## Configure

- Use get_cpu_data.sh to configure according to the machine processor.
- For HTM only, mini_bench_workers.sh line 44: useHTM
- Change global_structs.c at line 163 when running on a Vagrant VM without PM
- Use rsync.sh to transfer files to the remote server. Requires updating paths.sh.

## Compilation

1. Build all dependencies in /deps. Building `tinystm` requires creating a `lib` directory.
2. Run `get_cpu_data.sh`
3. Build `/nvhtm`
4. If running on the local filesystem instead of PM, create directories `0` and `1` in the directory in which the test script is run.

## New HTM implementation

1. Duplicate impl_htmOnly
2. Add new implementation to Makefile
3. Add entrypoint to new implementation on mini benchmarks. Example: tm.h at line 306. Also main.c line 372.

## New SPHT implementation without links

1. Duplicate impl_pcwm
2. Add new implementation to Makefile
3. Add entrypoint to new implementation on mini benchmarks. Example: tm.h at line 306. Also main.c line 372.

## Process output data

1. Update DATA_PATH in paths.sh
2. Use process_data_workers.sh and update solution and workload at lines 37 and 39.

On the local machine with output files:
        ./rsync.sh vm

On the Vagrant VM:
        cd projs/test_nvhtm_wait_phase
        ./process_data_workers.sh nvram

On the local machine with output files:
        scp vm:/home/vagrant/mini_bench/\*/\*.pdf ~/Desktop/

## Save output data

        mkdir /home/vagrant/saved-data
        scp nvram:~/projs/test_nvhtm_wait_phase/data_mini_bench_workers/* /home/vagrant/saved-data
