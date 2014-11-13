#!/bin/bash -x

modprobe msr
BASEDIR=../../..
KNOWNS="PAPI_TOT_INS,PAPI_L1_LDM,PAPI_L1_STM,PAPI_TLB_DM,PAPI_L2_TCM,PAPI_STL_ICY"
DELAY=180


#for JOBS in 1 2 3 4; do
for JOBS in 1; do
  for policy in 2; do
#  for POLICY in 1 2; do
    for FREQ in 3.20GHz; do
 #   for FREQ in `$BASEDIR/Scripts/freq_avail.sh`; do
      echo "Experiment: $FREQ $JOBS $POLICY"
      cpufreq-set -g performance --min $FREQ --max $FREQ -r
      cpufreq-info | grep 'current policy'
      ~sir/Dropbox/Work/Projects/Iconoclast/Tools/Power/build/power ../build/lbench -j $JOBS -p $POLICY > $(pwd)/results/freq_${FREQ}_jobs_${JOBS}_policy_${POLICY}_power.out &
      sleep $DELAY
      pkill lbench
      sleep 1
    done
  done
done
