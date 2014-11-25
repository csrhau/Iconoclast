#!/bin/bash -x

BENCHMARK_DIR=/home/sir/Downloads/rodinia/rodinia_3.0/openmp/leukocyte/
OUTPUT_DIR=$(pwd)
FREQ=3.20GHz
POWER=~sir/Dropbox/Work/Projects/Iconoclast/Tools/Power/build/power

modprobe msr
cpufreq-set -g performance --min $FREQ --max $FREQ -r
cpufreq-info | grep 'current policy'


mkdir $OUTPUT_DIR/data
cd $BENCHMARK_DIR
for RUN in `seq 1 3`; do
  $POWER ./OpenMP/leukocyte 45 4 ../../data/leukocyte/testfile.avi > $OUTPUT_DIR/data/leukocyte_small_run_${RUN}.out
done
