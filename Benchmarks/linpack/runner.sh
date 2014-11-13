#!/bin/sh
modprobe msr
cpufreq-set -g performance --min 3.20GHz --max 3.20GHz -r
../../Tools/Power/build/power /opt/intel/composer_xe_2013_sp1.0.080/mkl/benchmarks/linpack/xlinpack_xeon64 lininput_xeon64 > results.txt



