#!/usr/bin/env python
# -*- coding: utf-8 -*-

from setuptools import setup, Extension
import sys

gcc_args = ['-msse4.2', '-march=native', '-mtune=native', '-O3']
clang_args = gcc_args + []
args = clang_args if sys.platform == 'darwin' else gcc_args

setup(
    name='pysocks',
    version='0.2',
    license='Proprietary',
    author='Justin Graves',
    author_email='justin@infegy.com',
    description='Send stuff over a socket',
    long_description=__doc__,
    zip_safe=False,
    ext_modules = [
        Extension('pysocks',
        [
            "lib/pysocks.cpp",
        ],
        depends=[
        ],
        include_dirs=['lib'],
        libraries=['z'],
        extra_compile_args=args,
        language='c++')
    ],
    platforms='any',
    classifiers=[
        'Development Status :: Beta',
        'Intended Audience :: Developers',
        'Operating System :: OS Independent',
        'Programming Language :: C++',
        'Topic :: Software Development',
        'Topic :: Software Development :: Libraries :: Python Modules'
    ],
    packages = []
)
