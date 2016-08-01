import os
from setuptools import setup

if 'CC' not in os.environ:
    os.environ['CC'] = 'g++'

setup(
    name='pdfrender',
    version='0.1',
    setup_requires=['cffi>=1.0.0'],
    cffi_modules=['pdfrender_build.py:ffi'],
    install_requires=['cffi>=1.0.0'],
    packages=['pdfrender'],
)
