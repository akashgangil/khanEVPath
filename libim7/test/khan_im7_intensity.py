#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
sys.path.append('../libim7')

import numpy as np
import libim7.libim7 as im7
import matplotlib.pyplot as plt
from pylab import *
import glob

y_frame0 = []
y_frame1 = []

def main():
  print "Called main"
  path = "/net/hp100/ihpcae/marshall_andrew/PIV/LSB/reacting/Cam_Date=120402_Time=162032"
  filelist = glob.glob(path+"/*.im7")
  x = list(xrange(len(filelist)+1))[1:]
  y_frame0 = []
  y_frame1 = []
  
  for file in filelist:
    print "Processing File " , file
    buf, att = im7.readim7(file)
    y_frame0.append(mean(buf.get_frame(0)))
    y_frame1.append(mean(buf.get_frame(1)))
  
  plt.plot(x, y_frame0, 'b-', x, y_frame1, 'r-')
  plt.axis([0, 1000, 0, 500])
  savefig('fig.png')

def mean(img):
  sum_intensity = 0
  for row in range (len(img)):
    sum_intensity += sum(img[row])
  total_pixels = len(img) * len(img[0])
  return sum_intensity / total_pixels

if __name__ == "__main__":
  main()
