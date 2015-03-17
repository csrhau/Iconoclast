#!/usr/bin/env python
from __future__ import print_function
import math

def intercept(pt, dt, pb, n):
  return math.pow((pb * float(dt)**(n+1.0))/float(pt), 1.0/(n+1.0))



def main():
  for i in range(0, 4):
    print(intercept(45, 30, 30, i))

if __name__ == '__main__':
  main()
