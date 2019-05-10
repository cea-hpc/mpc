from os import environ
from distutils.core import setup, Extension

environ['CC'] = "gcc"
environ['CFLAGS'] = ""

module = Extension('arpc4py',
                    libraries = ['mpc_framework'],
                    sources = ['api.c'])

setup (name = 'arpc4py',
       version = '1.0',
       description = 'Python Extension for C/C++ ARPC interface',
       author= "Julien Adam",
       author_email = "adamj@paratools.com",
       ext_modules = [module])
