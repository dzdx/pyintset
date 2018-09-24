#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
from setuptools import setup, Extension

with open('README.rst') as readme_file:
    readme = readme_file.read()

with open('HISTORY.rst') as history_file:
    history = history_file.read()

extension = Extension('intset',
    sources=['./src/intsetobject.c', './src/intset.c', "./src/number.c"],
    include_dirs=['./src'],
    extra_compile_args=['-std=c99']
)

setup(
    name='pyintset',
    version='0.1.4',
    long_description=readme + '\n\n' + history,
    author="dzdx",
    author_email='dzidaxie@gmail.com',
    url='https://github.com/dzdx/pyintset',
    include_package_data=True,
    license="MIT license",
    platforms=["any"],
    zip_safe=False,
    keywords='intset',
    ext_modules=[extension],
    classifiers=[
        'Development Status :: 2 - Pre-Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Natural Language :: English',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
    ],
    test_suite='tests.test',
)
