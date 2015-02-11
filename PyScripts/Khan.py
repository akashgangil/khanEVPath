#!/usr/bin/python

import sys, os
sys.path.append(os.path.dirname(__file__)+'/libim7/libim7')
    
import libim7 as im7
import numpy as np
import mahotas

class Khan:
  def __init__(self, path):
    self.path = path.split("/")
    self.s = path 
    if path:
      self.buffer, self.attr = im7.readim7(self.s)

  def Destroy(self):
    self.path = None
    self.s = None
    self.buffer.delete()
    self.attr.delete()
    return ""

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

  def Dbuffer1(self):
      frame0 = self.buffer.get_frame(0)
      return np.array_str(frame0)
 
  def Dbuffer2(self):
      frame1 = self.buffer.get_frame(1)
      return np.array_str(frame1)
  
  def Dmask1(self):
      frame0 = self.buffer.get_frame(0)
      bfunc = np.vectorize(lambda a, b: 1 if a>=b else 0)
      binarize_frame0 = bfunc(frame0, mahotas.thresholding.otsu(frame0))
      return np.array_str(binarize_frame0)

  def Dmask2(self):
      frame1 = self.buffer.get_frame(1)
      bfunc = np.vectorize(lambda a, b: 1 if a>=b else 0)
      binarize_frame1 = bfunc(frame1, mahotas.thresholding.otsu(frame1))
      return np.array_str(binarize_frame1)

  def IntensityFrame1(self):
      frame0 = self.buffer.get_frame(0)
      bfunc = np.vectorize(lambda a, b: a if a>=b else 0)
      binarize_frame0 = bfunc(frame0, mahotas.thresholding.otsu(frame0)) 
      return str(np.mean(binarize_frame0))

  def IntensityFrame2(self):
      frame1 = self.buffer.get_frame(1)
      bfunc = np.vectorize(lambda a, b: a if a>=b else 0)
      binarize_frame1 = bfunc(frame1, mahotas.thresholding.otsu(frame1))
      return str(np.mean(binarize_frame1)) 

  def getIntensityFrame(self, frame, mask):
      bfunc = np.vectorize(lambda a, b: a if a>=b else 0)
      binarize_frame = bfunc(np.fromstring(frame, dtype=np.uint8), 
                             np.fromstring(mask, dtype=np.uint8))
      print "buffer ", np.sum(np.fromstring(frame, dtype=np.uint8)) 
      print "  mask ", np.sum(np.fromstring(mask, dtype=np.uint8))
      return str(np.mean(binarize_frame))
