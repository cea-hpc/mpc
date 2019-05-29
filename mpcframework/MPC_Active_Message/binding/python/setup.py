from os import environ
import sys
from distutils.core import setup, Extension

environ['CC'] = "gcc"
environ['CFLAGS'] = ""

try:
    arpc_hdrs = [ environ['ARPC_HDRS'] ]
except (KeyError):
    arpc_hdrs = ""
try:
    arpc_libs = [ environ['ARPC_LIBS'] ]
except (KeyError):
    arpc_libs = ""

try:
    arpc_link = [ environ['ARPC_LDFLAGS'] ]
except (KeyError):
    arpc_link = None

module = Extension('arpc4py',
                    include_dirs = arpc_hdrs,
                    library_dirs = arpc_libs,
                    libraries = arpc_link,
                    sources = ['api.c'])

setup (name = 'arpc4py',
       version = '1.0',
       description = 'Python Extension for C/C++ ARPC interface',
       author= "Julien Adam",
       author_email = "adamj@paratools.com",
       ext_modules = [module])
