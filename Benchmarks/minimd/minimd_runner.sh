#!/bin/sh -x


BASEDIR=`pwd`;
cd ~sir/Downloads/Mantevo/miniMD_ref_1.0/

modprobe msr
DELAY=60

for FREQ in `$BASEDIR/../../Scripts/freq_avail.sh`; do
  echo Experiment: $FREQ
  cpufreq-set -g performance --min $FREQ --max $FREQ -r
  cpufreq-info | grep 'current policy'
  $BASEDIR/../../Tools/Power/build/power ./miniMD_openmpi-dbg -n 100 -s 64 --half_neigh 0 > $BASEDIR/data/minimd-$FREQ.out
done
