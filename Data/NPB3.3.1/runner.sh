#!/bin/bash -x

modprobe msr

# CPU frequency
cpufreq-set -g performance --min 3.2Ghz --max 3.2Ghz -r
cpufreq-info

KNOWNS="PAPI_TOT_INS,PAPI_SR_INS,PAPI_LD_INS,PAPI_L2_TCR,PAPI_L2_TCW,PAPI_L3_TCR,PAPI_L3_TCW"

RESDIR=`pwd`

cd /home/sir/Downloads/Mantevo/miniMD_ref_1.0/

for SSR in 64000000 1638400000; do
for RUN in `seq 1 3`; do 


LD_PRELOAD=/home/sir/Dropbox/Work/Code/wattson/sleuth/bin/sleuth.so SLEUTH_OUTPUT_FILE=${RESDIR}/results/minimd_ssr_${SSR}_run_${RUN}.out SLEUTH_ADDITIONAL_METRICS=${KNOWNS} SLEUTH_SAMPLE_RATE=${SSR} ./miniMD_openmpi-dbg -n 100 -s 64 --half_neigh 0

done
done


cd -

echo "done!"
