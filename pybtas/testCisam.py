import Cisam

dd = Cisam.DataDict('/edx')
dd.open()
try:
  print dd.rec.gethdr()
  for r in dd.getrows():
    print r
finally:
  dd.close()
