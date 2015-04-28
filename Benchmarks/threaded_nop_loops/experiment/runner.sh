#!/bin/bash -x

modprobe msr
BASEDIR=../../..
KNOWNS="PAPI_TOT_INS,PAPI_L1_LDM,PAPI_L1_STM,PAPI_TLB_DM,PAPI_L2_TCM,PAPI_STL_ICY"
DELAY=120

for FREQ in `$BASEDIR/Scripts/freq_avail.sh`; do
  echo Experiment: $FREQ
  for THREADCOUNT in 1 2 3 4; do
    cpufreq-set -g performance --min $FREQ --max $FREQ -r
    cpufreq-info | grep 'current policy'
    ~sir/Dropbox/Work/Projects/Iconoclast/Tools/Power/build/power ../build/lbench -t $THREADCOUNT > $(pwd)/results/freq_${FREQ}_threads_${THREADCOUNT}_power.out &
    sleep $DELAY
    pkill lbench
    sleep 3
  done
done
