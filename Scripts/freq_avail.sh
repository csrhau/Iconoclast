#!/bin/sh

cpufreq-info | grep -m 1 'frequency steps' | cut -f 2 -d ':' | tr ',' '\n' | sed 's/ //g'
