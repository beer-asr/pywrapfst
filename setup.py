from distutils.core import setup, Extension
from Cython.Build import cythonize
import sys
import os

exts = cythonize(
    Extension('pywrapfst',
        sources=['src/pywrapfst.pyx'],
        language='c++',
        extra_compile_args=['-std=c++11'],
        libraries=['fst', 'fstscript', 'fstfar', 'fstfarscript'],
        include_dirs=[os.path.join(sys.prefix, 'include')]
    ),
    include_path=['src'],
)

setup(
    name='pywrapfst',
    version='1.0',
    description='python OpenFst wrapper',
    ext_modules=exts
)

