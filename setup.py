from setuptools import setup

setup(
    name='pdfrender',
    description='PDF Renderer',
    version='0.2.2',
    setup_requires=['cffi>=1.7.0'],
    cffi_modules=['pdfrender_build.py:ffi'],
    install_requires=['cffi>=1.7.0'],
    packages=['pdfrender'],
)
