from distutils.core import setup, Extension

import shutil

try:
    shutil.rmtree("./build")
except(OSError):
    pass

setup(name="intset", version='1.0',
      ext_modules=[Extension('intset', ['./src/intsetobject.c', './src/intset.c', "./src/number.c"])])
