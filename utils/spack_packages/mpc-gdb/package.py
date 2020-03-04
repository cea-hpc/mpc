##############################################################################
# Copyright (c) 2013-2016, Lawrence Livermore National Security, LLC.
# Produced at the Lawrence Livermore National Laboratory.
#
# This file is part of Spack.
# Created by Todd Gamblin, tgamblin@llnl.gov, All rights reserved.
# LLNL-CODE-647188
#
# For details, see https://github.com/llnl/spack
# Please also see the NOTICE and LICENSE files for our notice and the LGPL.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License (as
# published by the Free Software Foundation) version 2.1, February 1999.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
# conditions of the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##############################################################################
from spack import *


class MpcGdb(Package):
    """GDB, the GNU Project debugger, allows you to see what is going on
    'inside' another program while it executes -- or what another
    program was doing at the moment it crashed.
    """

    homepage = "https://www.gnu.org/software/gdb"
    url = "http://ftp.gnu.org/gnu/gdb/gdb-7.10.tar.gz"

    version('7.7', '40051ff95b39bd57b14b1809e2c16152')
    version('7.6.2', '9ebf09fa76e4ca6034157086e9b20348')
    version('7.5', 'c9f5ed81008194f8f667f131234f3ef0')

    variant('python', default=True, description='Compile with Python support')

    patch('mpc-gdb-7.7.patch', when='@7.7')
    patch('mpc-gdb-7.6.2.patch', when='@7.6.5')
    patch('mpc-gdb-7.5.patch', when='@7.5')

    # Required dependency
    depends_on('texinfo', type='build')

    # Optional dependency
    depends_on('python', when='+python')

    def install(self, spec, prefix):
        options = ['--prefix=%s' % prefix]
        options.extend(['--disable-werror'])
        if '+python' in spec:
            options.extend(['--with-python'])
        configure(*options)
        make()
        make("install")
