from os import environ
import sys
from distutils.core import setup, Extension

environ['CC'] = "gcc"
environ['CFLAGS'] = ""

try:
    arpc_path = environ['ARPC_PATH']
except (KeyError):
    arpc_path = ""

module = Extension('arpc4py',
                    include_dirs = [ arpc_path+'/include' ],
                    library_dirs = [ arpc_path+'/lib' ],
                    libraries = ['arpc'],
                    sources = ['api.c'])

setup (name = 'arpc4py',
       version = '1.0',
       description = 'Python Extension for C/C++ ARPC interface',
       author= "Julien Adam",
       author_email = "adamj@paratools.com",
       ext_modules = [module])
