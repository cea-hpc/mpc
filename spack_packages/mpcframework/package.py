##############################################################################
# Copyright (c) 2013-2016, Lawrence Livermore National Security, LLC.
# Produced at the Lawrence Livermore National Laboratory.
#
# This file is part of Spack.
# Created by Todd Gamblin, tgamblin@llnl.gov, All rights reserved.
# LLNL-CODE-647188
#
# For details, see https://github.com/llnl/spack
# Please also see the LICENSE file for our notice and the LGPL.
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
#
# This is a template package file for Spack.  We've put "FIXME"
# next to all the things you'll want to change. Once you've handled
# them, you can save this file and test your package like this:
#
#     spack install mpcframework
#
# You can edit this file again by typing:
#
#     spack edit mpcframework
#
# See the Spack documentation for more information on packaging.
# If you submit this package back to Spack as a pull request,
# please first remove this boilerplate and all FIXME comments.
#
from spack import *
from spack.environment import EnvironmentModifications

import glob
import os

class Mpcframework(Package):
    """FIXME: Put a proper description of your package here."""

    # FIXME: Add a proper url for your package's homepage here.
    homepage = "http://exemple.com"
    url      = "http://exemple.com/MPC_light_3.2.1.tar.gz"

    version('mpc_spack', git='ssh://git@gitocre/mpc.git', branch='mpc_spack')
    version('master', git='ssh://git@gitocre/mpc.git', branch='devel')
    version('devel', git='ssh://git@gitocre/mpc.git', branch='master')
    version('merge_to_devel_hotfixes', git='ssh://git@gitocre/mpc.git', branch='merge_to_devel_hotfixes')

    version('3.2.1_devel', '96e50190285cf0d585867be8f5729490')

    # FIXME: Add dependencies if required.

    #def configure_args(self):
        # FIXME: Add arguments other than --prefix
        # FIXME: If not needed delete this function
     #   args = []
      #  return args

    provides('mpi')
    provides('mpi@:2.2', when='@3.2.0:')
    provides('mpi@:3.0', when='@3.2.0:')
    provides('mpi@:3.1', when='@3.2.0:')


    variant('mpc-process-mode', default=False, description='mpc-process-mode')
    variant('mpc-debug', default=False, description='debug-mode')

    depends_on('mpc-gcc', when='-mpc-process-mode')
    depends_on('mpc-gdb', type='run', when='-mpc-process-mode')
    depends_on('mpc-binutils', when='-mpc-process-mode')
    depends_on('hydra', type='run')
    depends_on('gmp',type=('build', 'link','run' ),  when='-mpc-process-mode')
    depends_on('hwloc')
#    depends_on('openpa')
    depends_on('libxml2')
    depends_on('autoconf',type='build')
    depends_on('automake',type='build')

    variant('mpc-process-mode', default=False, description='mpc-process-mode')
    variant('mpc-debug', default=False, description='debug-mode')
    variant('mpc-ft', default=False, description="Fault-Tolerance support")
    varian('color', default=False, description="colored output");


    def install(self, spec, prefix):
        options = ['-j{0}'.format(make_jobs), '--prefix={0}'.format(prefix)]
       
        if '+mpc-debug' in spec:
            options.extend(['--enable-mpc-debug'])

        if '+mpc-process-mode' in spec:
            options.extend(['--mpc-process-mode'])
            options.extend(['--disable-mpc-gdb'])

        if '+mpc-ft' in spec:
            options.extend(['--enable-mpc-ft'])

        options.extend(['--spack-build'])
        options.append('--with-hydra={0}'.format(
                spec['hydra'].prefix))
        options.append('--with-hwloc={0}'.format(
                spec['hwloc'].prefix))
        options.append('--with-libxml2={0}'.format(
                spec['libxml2'].prefix))

        if spec.satisfies('-mpc-process-mode'):
            options.append('--with-mpc-gcc={0}'.format(
                    spec['mpc-gcc'].prefix))
            options.append('--with-mpc-binutils={0}'.format(
                    spec['mpc-binutils'].prefix))
            options.append('--with-mpc-gdb={0}'.format(
                    spec['mpc-gdb'].prefix))
            options.append('--with-gmp={0}'.format(
                    spec['gmp'].prefix))


#        options.append('--with-openpa={0}'.format(
#                spec['openpa'].prefix))


        # FIXME: Modify the configure line to suit your build system here.
        build_bin=Executable("./installmpc")
        build_bin(*options)

    def setup_dependent_environment(self, spack_env, run_env, dependent_spec):
        spack_env.set('MPICC',  join_path(self.prefix.bin, '/x86_64/x86_64//bin/mpiwrapp/mpicc'))
        spack_env.set('MPICXX', join_path(self.prefix.bin, '/x86_64/x86_64//bin/mpiwrapp/mpic++'))
        spack_env.set('MPIF77', join_path(self.prefix.bin, '/x86_64/x86_64//bin/mpiwrapp/mpif77'))
        spack_env.set('MPIF90', join_path(self.prefix.bin, '/x86_64/x86_64//bin/mpiwrapp/mpif90'))


    def setup_dependent_package(self, module, dependent_spec):
        self.spec.mpicc = join_path(self.prefix.bin, 'mpicc')
        self.spec.mpicxx = join_path(self.prefix.bin, 'mpicxx')
        self.spec.mpifc = join_path(self.prefix.bin, 'mpif90')
        self.spec.mpif77 = join_path(self.prefix.bin, 'mpif77')



    def setup_environment(self, spack_env, run_env):
        """Adds environment variables to the generated module file.

        These environment variables come from running:

        .. code-block:: console

           $ source mpcvars.sh
        """
        # NOTE: Spack runs setup_environment twice, once pre-build to set up
        # the build environment, and once post-installation to determine
        # the environment variables needed at run-time to add to the module
        # file. The script we need to source is only present post-installation,
        # so check for its existence before sourcing.
        # TODO: At some point we should split setup_environment into
        # setup_build_environment and setup_run_environment to get around
        # this problem.
        mpcvars = glob.glob(join_path(self.prefix, 'mpcvars.sh'))

        if mpcvars:
            run_env.extend(EnvironmentModifications.from_sourcing_file(mpcvars[0]))

    def setup_dependent_environment(self, spack_env, run_env, dependent_spec):
        mpcvars = glob.glob(join_path(self.prefix, 'mpcvars.sh'))

        if mpcvars:
            spack_env.extend(EnvironmentModifications.from_sourcing_file(mpcvars[0]))


    @run_after('install')
    def filters_compilers(self):
#        mpcvars = glob.glob(join_path(self.prefix, 'mpcvars.sh'))
#        run_env.extend(EnvironmentModifications.from_sourcing_file(mpcvars[0]))

        ln_bin=Executable('ln')
        options=['-s','x86_64/x86_64/bin', self.prefix.bin]
        ln_bin(*options)
        options=['-s','x86_64/x86_64/lib', self.prefix.lib]
        ln_bin(*options)
        options=['-s','x86_64/x86_64/include', self.prefix.include]
        ln_bin(*options)

        if self.spec.satisfies('+mpc-process-mode'):
            mpc_compiler_manager=Executable(join_path(self.prefix.bin, 'mpc_compiler_manager'))
            options=['set_default','c',self.compiler.cc]
            mpc_compiler_manager(*options)
            options=['set_default','cxx',self.compiler.cxx]
            mpc_compiler_manager(*options)
            options=['set_default','fortran',self.compiler.fc]
            mpc_compiler_manager(*options)
