#!/bin/sh
modprobe msr
cpufreq-set -g performance --min 3.20GHz --max 3.20GHz -r
./Iconoclast/Tools/Power/build/power ./TimeSpaceDecomposition/reference_deqn/build/deqn ./TimeSpaceDecomposition/reference_deqn/build/bigsquare.in > log.output
rm -rf *.visit *.vtk
