from cisam import *
import struct

class CharField:
  def __init__(self,name,pos,len,desc=None):
    self.beg = pos
    self.end = pos + len
    self.size = len
    self.name = name

  def getvalue(self,buf):
    return str(buf[self.beg:self.end]).rstrip()

  def setvalue(self,buf,val):
    size = self.size
    val = val.ljust(size)
    buf[self.beg:self.end] = val[:size]

class NumField:
  def __init__(self,name,pos,len,desc=None):
    self.beg = pos
    if len > 8: len = 8
    self.end = pos + len
    if len > 4:
      self.pad = 8 - len
      self.fmt = '!q'
    else:
      self.pad = 4 - len
      self.fmt = '!l'

  def getvalue(self,buf):
    b = buf[beg:end]
    fill = b[0] & 0x80 and '\xff' or '\x00'
    return struct.unpack(self.fmt,fill * self.pad + b)

  def setvalue(self,buf,val):
    buf[self.beg:self.end] = struct.pack(self.fmt,val)[self.pad:]

class Record:
  def __init__(self,flds,buf=None):
    map = {}
    for f in flds: map[f.name] = f
    self.__dict__['_map_'] = map
    self.__dict__['_buf_'] = buf

  def __getattr__(self,name):
    return self._map_[name].getvalue(self._buf_)
  def __setattr__(self,name,val):
    try:
      f = self._map_[name]
    except KeyError:
      self.__dict__[name] = val
    else:
      f.setvalue(self._buf_,val)

class DataDict:
  typemap = {
    'C': CharField,
    'N0': NumField
  }
  def __init__(self,dir='/edx'):
    self.flds = (
      CharField('tblname',0,18),
      NumField('fldidx',18,2),
      NumField('fldpos',20,2),
      CharField('fldname',22,8),
      CharField('fldtype',30,2),
      NumField('fldlen',32,2),
      CharField('fldmask',34,15),
      CharField('flddesc',49,24)
    )
    self.fname = 'DATA.DICT'
    self.btasdir = dir
    self.rec = Record(self.flds)
    self.fd = None

  def open():
    self.fd = isopen(btasdir + '/' + self.fname,READONLY)

  def close():
    self.fd.close()
    self.fd = None

  def createDesc():
    r = self.rec
    return r.fldname,r.fldpos,r.fldlen,r.fldtype,r.fldmask,r.flddesc

  def createField():
    r = self.rec
    return typemap[r.fldtype](r.fldname,r.fldpos,r.fldlen)

class BtasFields:
  def __init__(self):
    flds = []
