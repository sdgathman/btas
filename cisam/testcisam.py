import unittest
from cisam import *
from array import array

testf = [BT_NUM,4,BT_NUM,2,BT_CHAR,32,BT_BIN,64,BT_NUM,4,BT_RECNO,4]

class CisamTestCase(unittest.TestCase):

  def testCreate(self):
    fname = "/tmp/cisamtest"
    kd = 0,[(0,8)]
    flds = array('B',[BT_CHAR,8,BT_CHAR,24])
    try:
      iserase(fname)
    except:
      pass
    fd = isbuild(fname,kd,ISINOUT+ISEXCLLOCK,flds)
    self.failUnless(fd.rlen == 32)
    fd.recbuf[0:8] = 'TESTING '
    fd.recbuf[8:32] = 'ABC123                  '
    fd.write(1)

    fd.recbuf[0:8] = 'ALPHA   '
    fd.recbuf[8:32] = 'ANOTHER TEST            '
    fd.write(0)		# iswrcurr

    fd.read(ISFIRST)	# shouldn't be needed for iswrcurr
    fd.read(ISNEXT)
    self.failUnless(fd.recbuf[0:8] == 'TESTING ')
    fd.recbuf[0:8] = 'TEST123 '
    fd.rewrite(None);
    fd.read(ISPREV)
    self.failUnless(fd.recbuf[0:8] == 'ALPHA   ')

    fd.delete(None);
    fd.read(ISFIRST)
    self.failUnless(fd.recbuf[0:8] == 'TEST123 ')

    # check that indexinfo is correct
    di = fd.indexinfo(0)
    self.failUnless(di[0] == 1)
    self.failUnless(di[1] == 32)
    self.failUnless(di[3] == 1)
    ki = fd.indexinfo(1)
    self.failUnless(ki == (0,((0,8,0),),8))
    self.failUnless(map(ord,fd.getflds()) == list(flds))

    # test appending fields to record
    fd.addflds(array('B',[BT_NUM,4]))
    self.failUnless(fd.rlen == 36)
    fd.close()
    #iserase(fname)

def suite(): return unittest.makeSuite(CisamTestCase,'test')

if __name__ == '__main__':
  unittest.main()
