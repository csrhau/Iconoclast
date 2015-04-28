#!/bin/sh -x

BASEDIR=`pwd`;
mkdir -p results

cd ~sir/Downloads/Mantevo/miniMD_ref_1.0/

modprobe msr

for run in 1 2; do
for THREADS in 2; do
  for FREQ in 2.20Ghz; do
    echo Experiment: $FREQ
    cpufreq-set -g performance --min $FREQ --max $FREQ -r
    cpufreq-info | grep 'current policy'
    $BASEDIR/../../Tools/Power/build/power ./miniMD_openmpi-dbg -n 100 -s 64 --half_neigh 0 --num_threads $THREADS > $BASEDIR/results/minimd-freq-$FREQ-threads$THREADS.out.${run}
    sleep 3;
  done
done
done
