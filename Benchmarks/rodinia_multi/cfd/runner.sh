#!/bin/bash -x

BENCHMARK_DIR=/home/sir/Downloads/rodinia/rodinia_3.0/openmp/cfd/
OUTPUT_DIR=$(pwd)
FREQ=3.20GHz
POWER=~sir/Dropbox/Work/Projects/Iconoclast/Tools/Power/build/power

modprobe msr
cpufreq-set -g performance --min $FREQ --max $FREQ -r
cpufreq-info | grep 'current policy'


mkdir $OUTPUT_DIR/data
cd $BENCHMARK_DIR
for RUN in `seq 1 3`; do
  $POWER ./euler3d_cpu ../../data/cfd/fvcorr.domn.097K > $OUTPUT_DIR/data/cfd_small_run_${RUN}.out
done
