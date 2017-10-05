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


class MpcBinutils(AutotoolsPackage):
    """GNU binutils, which contain the linker, assembler, objdump and others"""

    homepage = "http://www.gnu.org/software/binutils/"
    url      = "https://ftp.gnu.org/gnu/binutils/binutils-2.28.tar.bz2"

    version('2.27', '2869c9bf3e60ee97c74ac2a6bf4e9d68')
    version('2.25', 'd9f3303f802a5b6b0bb73a335ab89d66')

    variant('plugins', default=False,
            description="enable plugins, needed for gold linker")
    variant('gold', default=True, description="build the gold linker")
    variant('libiberty', default=False, description='Also install libiberty.')

    patch('cr16.patch')
    patch('update_symbol-2.26.patch', when='@2.26')

    patch('mpc-binutils-2.25.patch', when='@2.25')
    patch('mpc-binutils-2.27.patch', when='@2.27')

    depends_on('zlib')

    depends_on('m4', type='build')
    depends_on('flex', type='build')
    depends_on('bison', type='build')
    depends_on('gettext')

    def configure_args(self):
        spec = self.spec

        configure_args = [
            '--with-system-zlib',
            '--disable-dependency-tracking',
            '--disable-werror',
            '--enable-interwork',
            '--enable-multilib',
            '--enable-shared',
            '--enable-64-bit-bfd',
            '--enable-targets=all',
            '--with-sysroot=/',
        ]

        if '+gold' in spec:
            configure_args.append('--enable-gold')

        if '+plugins' in spec:
            configure_args.append('--enable-plugins')

        if '+libiberty' in spec:
            configure_args.append('--enable-install-libiberty')

        return configure_args
