#!/bin/sh

modprobe msr
DELAY=60

for FREQ in `../../Scripts/freq_avail.sh`; do
  echo Experiment: $FREQ
  cpufreq-set -g performance --min $FREQ --max $FREQ -r
  cpufreq-info | grep 'current policy'
  ./build/power `which sleep` $DELAY > ./data/sleep-$FREQ.out
done
