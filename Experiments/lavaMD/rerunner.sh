#!/bin/sh -x

BASEDIR=`pwd`;
BENCHMARK_DIR=/home/sir/Downloads/rodinia/rodinia_3.0/openmp/lavaMD/


mkdir -p results
cd $BENCHMARK_DIR

modprobe msr

for config in 2.40GHz:1 3.20GHz:2; do
  FREQ=${config%:*};
  THREADS=${config#*:};
  echo Experiment: $FREQ
  cpufreq-set -g performance --min $FREQ --max $FREQ -r
  cpufreq-info | grep 'current policy'
  $BASEDIR/../../Tools/Power/build/power ./lavaMD -cores $THREADS -boxes1d 20 > $BASEDIR/results/freq-$FREQ-threads-$THREADS.out
  sleep 3;
done
