#!/usr/bin/python

from optparse import OptionParser

import sys
sys.path.append('/net/hu21/agangil3/libim7/libim7')

import numpy as np
import mahotas
import scipy
import libim7 as im7
import glob

#s = "/net/hu21/agangil3/data/PIV/LSB/reacting/S = 0.58/1 atm/300 K/30 mps/100 H2/phi = 0.46/0 deg/Cam_Date=121103_Time=112726/B00001.im7" 

class Khan:
  def __init__(self, path):
    self.path = path.split("/")
    self.s = path 

  def __del__(self):
    self.path = None
    self.s = None

  def S(self):
      res = ""
      for i in self.path:
        if 'S = ' in i:
          res = i.split(" ")[2]
      if not res:
        return " "
      else:
        return res

  def Pressure(self):
      res = ""
      for i in self.path:
        if 'atm' in i:
          res = i.split(" ")[0]
      if not res:
        return " "
      else:
        return res

  def Temperature(self):
      res = ""
      for i in self.path:
        if 'K' in i:
          res = i.split(" ")[0]
      if not res:
        return " "
      else:
        return res

  def Velocity(self):
      res = ""
      for i in self.path:
        if 'mps' in i:
          res = i.split(" ")[0]
      if not res:
        return " "
      else:
        return res

  def Composition(self):
     res = ""
     for i in self.path:
        if 'H2' in i:
          res = i
        elif 'CH4' in i:
          res = i
     if not res:
        return " "
     else:
        return res
        
  def Phi(self):
      res = ""
      for i in self.path:
        if 'phi' in i:
          res = i.split(" ")[2]
      if not res:
        return " "
      else:
        return res

  def Angle(self):
      res = ""
      for i in self.path:
        if 'deg' in i:
          res = i.split(" ")[0]
      if not res:
        return " "
      else:
        return res

  def Date(self):
      res = ""
      for i in self.path:
        if 'Cam_Date' in i:
          res = i.split("=")[1].split("_")[0]
      if not res:
        return " "
      else:
        return res

  def Time(self):
      res = ""
      for i in self.path:
        if 'Cam_Date' in i:
          res = i
      if not res:
        return " "
      else:
        return res

  def IntensityFrame1(self):
      buf, att = im7.readim7(self.s)
      frame0 = buf.get_frame(0)
      bfunc = np.vectorize(lambda a, b: a if a>=b else 0)
      binarize_frame0 = bfunc(frame0, mahotas.thresholding.otsu(frame0)) 
      buf.delete()
      att.delete()
      return str(np.mean(binarize_frame0))

  def IntensityFrame2(self):
      buf, att = im7.readim7(self.s)
      frame1 = buf.get_frame(1)
      bfunc = np.vectorize(lambda a, b: a if a>=b else 0)
      binarize_frame1 = bfunc(frame1, mahotas.thresholding.otsu(frame1))
      buf.delete()
      att.delete()
      return str(np.mean(binarize_frame1)) 
