import Cisam

dd = Cisam.DataDict('/edx')
dd.open()
try:
  print dd.rec.gethdr()
  dd.first()
  while True:
    print dd.rec.getrow()
    dd.next()
finally:
  dd.close()
