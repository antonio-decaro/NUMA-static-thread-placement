#!/bin/sh
make -C ./lib/counters

#You should add those vars in .bashrc
export LD_LIBRARY_PATH=/home/hpc1/workspace/benchmark/lib/counters/:$LD_LIBRARY_PATH
export PIN_ROOT=/home/hpc1/installs/pin-3.16-98275-ge0db48c31-gcc-linux/
export PATH=$PIN_ROOT:$PATH

make -C ./lib/instrumentation
make -C ./tests