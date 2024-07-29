Build options
=============

This is a non-exhaustive list of build options for quick start. For a complete documentation please refer to :doc:`the full installmpc help page<../installmpc>`.

Files location and external modules configuration
-------------------------------------------------

Unless building with sudo you should specify a prefix for the installation. Use absolute paths to avoid any issue. For a quick install you can use these commands : 

::

    mkdir $HOME/installmpc
    installmpc --prefix=$HOME/installmpc

In case you are building MPC offline you should place the external modules sources in the BUILD directory or in the $HOME/.mpc-deps. You can get all dependencies sources zipped archives with :code:`installmpc --download`. MPC installed dependencies are the following :

- openpa
- Hydra
- OFI
- mpc-compiler-additions
- mpc-allocator
- libfabric



MPC will look for pre-installed modules although you can specify paths for system modules. The option to specify a module path is :code:`--with-[module]=`. The option to disable a module is :code:`--disable-[module]`

Additionally you can specify paths for :

- cuda
- opencl
- openacc
- tbb

MPC modules are installed in the same prefix as MPC. It is possible to reconfigure modules and install them again if you have any specific needs.