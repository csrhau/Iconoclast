#!/bin/bash -x

modprobe msr

APP_BASE=../../../

PATH="$PATH:${APP_BASE}/Scripts"
PROFILER=${APP_BASE}/Tools/PerformanceEventProfiler/bin/sleuth.so

KNOWNS="PAPI_TOT_INS"

MAX_FREQ=$(freq_avail.sh | head -n 1)
MIN_FREQ=$(freq_avail.sh | head -n 1)

FREQ=$MAX_FREQ

  
SSR=64000000
for binary in ./bin/*.x; do
  RESDIR=results/$binary
  mkdir -p $RESDIR
  for frequency in `freq_avail.sh`; do
    cpufreq-set -g performance --min $frequency --max $frequency -r
    cpufreq-info
    LD_PRELOAD=$PROFILER SLEUTH_OUTPUT_FILE=$RESDIR/${frequency}.out SLEUTH_ADDITIONAL_METRICS=${KNOWNS} SLEUTH_SAMPLE_RATE=${SSR} $binary 
  done
done

#LD_PRELOAD=$PROFILER SLEUTH_OUTPUT_FILE=$(RESDIR)/$(FREQ).out


#forces us back to normal state
cpufreq-set -g ondemand --min 1MHz --max 100GHz
