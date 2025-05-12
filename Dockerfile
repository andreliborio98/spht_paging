FROM debian:buster

RUN apt update && apt upgrade -y
RUN apt install make gcc g++ gdb linux-perf -y
RUN echo "set auto-load safe-path /" > /root/.gdbinit

### copied after build
#COPY nvhtm /root/projs/test_nvhtm_wait_phase

### Dependencies
#COPY arch_dep /root/projs/arch_dep
#COPY htm_alg /root/projs/htm_alg
#COPY THREADING /root/projs/THREADING
#COPY INPUT_HANDLER /root/projs/INPUT_HANDLER
###

ENV NVHTM_HOME=/root/projs/test_nvhtm_wait_phase
WORKDIR /root/projs/test_nvhtm_wait_phase
