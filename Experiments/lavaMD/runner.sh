#!/bin/sh -x

BASEDIR=`pwd`;
BENCHMARK_DIR=/home/sir/Downloads/rodinia/rodinia_3.0/openmp/lavaMD/


mkdir -p results
cd $BENCHMARK_DIR

modprobe msr

for THREADS in `seq 4 -1 1`; do
  for FREQ in `$BASEDIR/../../Scripts/freq_avail.sh`; do
    echo Experiment: $FREQ
    cpufreq-set -g performance --min $FREQ --max $FREQ -r
    cpufreq-info | grep 'current policy'
    $BASEDIR/../../Tools/Power/build/power ./lavaMD -cores $THREADS -boxes1d 20 > $BASEDIR/results/freq-$FREQ-threads-$THREADS.out
    sleep 3;
  done
done
