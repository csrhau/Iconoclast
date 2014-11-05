#!/usr/bin/env python

"""A simple parser for power tool output"""

from __future__ import print_function
from collections import namedtuple
import argparse
import re
import sys


PowerOutput = namedtuple('PowerOutput', ['filename', 'frequency', 'realtime',
                                         'systime', 'usrtime', 'pkg_energy',
                                         'pp0_energy', 'pkg_power'])


def process_arguments():
    """Returns a dictionary of command line arguments values."""
    parser = argparse.ArgumentParser(description='Power Output Formatter')
    parser.add_argument('-if', '--infiles', required=True, nargs='+',
                        type=argparse.FileType('r'),
                        help='Output files to consider')
    return parser.parse_args()


def parse_file(infile):
    """Parses a single entry of the form:
    program output
    ...
    Real Time:  1m  00s 628us
    Sys Time: 0s 0us
    User Time:  0s 0us
    PKG Energy: 43.785278J
    PP0 Energy: 0.000000J
    PKG Power:  0.729747W
    """
    frequency_re = re.compile(r"(\d+\.\d+|\d+)\s?(GHz|MHz)")
    frequency_match = re.search(frequency_re, infile.name)
    if frequency_match is None:
        sys.exit("ERROR! Filename {} lacks CPU frequency".format(infile.name))
    if frequency_match.group(2) == 'GHz':
        frequency_unit = 1e9
    elif frequency_match.group(2) == 'MHz':
        frequency_unit = 1e6
    else:
        sys.exit('Unknown frequency unit: {}'.format(frequency_unit))
    frequency_hz = float(frequency_match.group(1)) * frequency_unit
    realtime_re = re.compile(r"^Real Time:\s*(\d+)m\s*(\d+)s\s*(\d+)us$")
    systime_re = re.compile(r"^Sys Time:\s*(\d+)s\s*(\d+)us$")
    usrtime_re = re.compile(r"^User Time:\s*(\d+)s\s*(\d+)us$")
    pkg_energy_re = re.compile(r"^PKG Energy:\s*(\d+\.?\d+?)J$")
    pp0_energy_re = re.compile(r"^PP0 Energy:\s*(\d+\.?\d+?)J$")
    pkg_power_re = re.compile(r"^PKG Power:\s*(\d+\.?\d+?)W$")

    realtime = None
    systime = None
    usrtime = None
    pkg_energy = None
    pp0_energy = None
    pkg_power = None
    for line in infile:
        match = re.match(realtime_re, line)
        if match is not None:
            print(match.group(0))
            if realtime is not None:
                sys.exit("Duplicate Real Time value in {}".format(infile.name))
            realtime = int(match.group(1)) * 60 \
                     + int(match.group(2))      \
                     + int(match.group(3)) * 1e-6  # Mins, Secs, Microsecs
        match = re.match(systime_re, line)
        if match is not None:
            print(match.group(0))
            if systime is not None:
                sys.exit("Duplicate Sys Time value in {}".format(infile.name))
            systime = int(match.group(1)) + int(match.group(2)) * 1e-6
        match = re.match(usrtime_re, line)
        if match is not None:
            print(match.group(0))
            if usrtime is not None:
                sys.exit("Duplicate User Time value in {}".format(infile.name))
            usrtime = int(match.group(1)) + int(match.group(2)) * 1e-6
        match = re.match(pkg_energy_re, line)
        if match is not None:
            print(match.group(0))
            if pkg_energy is not None:
                sys.exit("Duplicate PKG Energy in {}".format(infile.name))
            pkg_energy = float(match.group(1))
        match = re.match(pp0_energy_re, line)
        if match is not None:
            print(match.group(0))
            if pp0_energy is not None:
                sys.exit("Duplicate PP0 Energy in {}".format(infile.name))
            pp0_energy = float(match.group(1))
        match = re.match(pkg_power_re, line)
        if match is not None:
            print(match.group(0))
            if pkg_power is not None:
                sys.exit("Duplicate PKG Power in {}".format(infile.name))
            pkg_power = float(match.group(1))
    return PowerOutput(filename=infile.name,
                       frequency=frequency_hz,
                       realtime=realtime,
                       systime=systime,
                       usrtime=usrtime,
                       pkg_energy=pkg_energy,
                       pp0_energy=pp0_energy,
                       pkg_power=pkg_power)


def main():
    """Program Entry Point"""
    args = process_arguments()
    data = []
    for infile in args.infiles:
        data.append(parse_file(infile))
        infile.close()
    print(data)

if __name__ == '__main__':
    main()
