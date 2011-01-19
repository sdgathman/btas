from cisam import *
import struct

class Field(object):
  __slots__ = 'beg','end','size','name'

class CharField(Field):
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

class NumField(Field):
  __slots__ = 'pad','fmt'
  def __init__(self,name,pos,len,desc=None):
    self.name = name
    self.beg = pos
    if len > 8: len = 8
    self.end = pos + len
    if len > 4:
      self.pad = 8 - len
      self.fmt = '!q'
    elif len > 2:
      self.pad = 4 - len
      self.fmt = '!l'
    else:
      self.pad = 2 - len
      self.fmt = '!h'

  def getvalue(self,buf):
    b = buf[self.beg:self.end]
    if self.pad:
      fill = ord(b[0]) & 0x80 and '\xff' or '\x00'
      return struct.unpack(self.fmt,fill * self.pad + b)[0]
    return struct.unpack(self.fmt,b)[0]

  def setvalue(self,buf,val):
    buf[self.beg:self.end] = struct.pack(self.fmt,val)[self.pad:]

class Record(object):
  __slots__ = '_map_','_buf_','_flds_'
  def __init__(self,flds,buf=None):
    map = {}
    for f in flds:
      map[f.name] = f
    object.__setattr__(self, '_map_',map)
    self._flds_ = flds
    self._buf_ = buf

  def __getattr__(self,name):
    try:
      return self._map_[name].getvalue(self._buf_)
    except KeyError:
      raise AttributeError(name)

  def __setattr__(self,name,val):
    try:
      f = self._map_[name]
      f.setvalue(self._buf_,val)
    except KeyError:
      object.__setattr__(self, name,val)

  def getrow(self):
    return [f.getvalue(self._buf_) for f in self._flds_]

  def gethdr(self):
    return [f.name for f in self._flds_]

class Table(object):
  def __init__(self,fname):
    self.fd = None

  def close(self):
    self.fd.close()
    self.fd = None
    
  def first(self):
    self.fd.read(ISFIRST)
  def next(self):
    self.fd.read(ISNEXT)

  def getrows(self):
    try:
      self.first()
      while True:
        yield self.rec.getrow()
        self.next()
    except error,x:
      if self.fd.iserrno != 110: raise

class DataDict(Table):
  typemap = {
    'C': CharField,
    'N0': NumField
  }
  def __init__(self,dir='/edx'):
    flds = (
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
    self.rec = Record(flds)
    self.fd = None

  def open(self):
    self.fd = isopen(self.btasdir + '/' + self.fname,READONLY)
    self.rec._buf_ = self.fd.recbuf

  def createDesc(self):
    r = self.rec
    return r.fldname,r.fldpos,r.fldlen,r.fldtype,r.fldmask,r.flddesc

  def createField(self):
    r = self.rec
    return typemap[r.fldtype](r.fldname,r.fldpos,r.fldlen)

class BtasFields:
  def __init__(self):
    self.flds = []

