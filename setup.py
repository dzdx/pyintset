from distutils.core import setup, Extension

import shutil

try:
    shutil.rmtree("./build")
except(OSError):
    pass

setup(name="intset", version='1.0',
      ext_modules=[Extension(name='intset',
                             sources=['./python/export.c',
                                      './src/intset.c'],
                             include_dirs=['./python', './src']
                             ),
                   ])
