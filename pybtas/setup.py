import os
from distutils.core import setup, Extension

libs = ["btas"]

setup(name = "cisam", version = "0.1",
	description="Python interface to BTAS/X C-isam API",
	long_description="""\
This is a python extension module to enable python scripts to
attach to read and write BTAS/X file through the C-isam API.
""",
	author="Stuart D. Gathman",
	author_email="stuart@bmsi.com",
	maintainer="Stuart D. Gathman",
	maintainer_email="stuart@bmsi.com",
	licence="LGPL",
	url="http://www.bmsi.com/python/cisam.html",
#	py_modules=["Milter","mime"],
	ext_modules=[Extension("cisam", ["cisammodule.c"],libraries=libs,
	  include_dirs=["/bms/include"],library_dirs=["/bms/lib"])])
