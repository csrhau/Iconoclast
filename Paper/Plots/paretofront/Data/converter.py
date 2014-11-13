#!/usr/bin/env python

from __future__ import print_function
from collections import namedtuple
import argparse
import csv

def process_arguments():
  parser = argparse.ArgumentParser(description="Convert to various metrics",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument("-if", "--infile", type=argparse.FileType('r'), required=True,
                      help="Path to data file to process")
  parser.add_argument("-of", "--outfile", type=argparse.FileType('w'), required=True,
                      help="Path to data file to write to")
  parser.add_argument('-rt', '--runtime', type=float, default=10, help='Application runtime')
  parser.add_argument('-bf', '--base_freq', type=float, default=3.2, help='Base frequency, ghz')
  return parser.parse_args()

def ed2p(power, runtime):
    print 
    return float(power) * runtime * runtime

def main():
    args = process_arguments()
    DataSample = namedtuple('DataSample', args.infile.readline())
    in_data = map(DataSample._make, csv.reader(args.infile))
    out_data = []
    for idp in in_data:
        idpf = float(idp.Frequency)
        sample_rt = (args.base_freq / idpf) * args.runtime
        result = DataSample(idpf, One=ed2p(idp.One,sample_rt),
                                  Two=ed2p(idp.Two,sample_rt / 2),
                                  Three=ed2p(idp.Three,sample_rt / 3),
                                   Four=ed2p(idp.Four,sample_rt / 4))
        out_data.append(result)
    for odp in out_data:
        print(odp, file=args.outfile)




if __name__ == '__main__':
    main()
