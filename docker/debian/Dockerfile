ARG img_tag=stretch

FROM debian:${img_tag} AS base

RUN apt-get update && apt-get install --no-install-recommends -y \
	gcc g++ \
	make \
	patch \
	bzip2 \
	sudo \
	&& apt-get autoremove -y \
	&& rm -rf /var/lib/apt/lists/*

RUN groupadd sudo; exit 0
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers
RUN useradd -G sudo -ms /bin/bash mpcuser

USER mpcuser
WORKDIR /home/mpcuser
RUN mkdir -p /home/mpcuser/build /home/mpcuser/install
CMD /bin/bash

LABEL \
	name = "Multi-Processor Computing Framework (MPC)" \
	description = "MPC provides implementations for the MPI, OpenMP and POSIX Threads standards to improve the scalability and performances of HPC applications" \
	license= "CeCILL-C (LGPL-compatible)" \
	builddate = "20191020" \
	maintainer.project = "Marc Perache <marc.perache@cea.fr>" \
	maintainer.docker = "Julien Adam <adamj@paratools.com>"

FROM base AS install
ARG mpc_version=3.3.1

ADD --chown=mpcuser:mpcuser https://france.paratools.com/mpc/releases/MPC_${mpc_version}.tar.gz /home/mpcuser/
RUN tar xf ./MPC_${mpc_version}.tar.gz \
	&& cd ./build/ \
	&& ../MPC_${mpc_version}/installmpc --prefix=/home/mpcuser/install/ --disable-color -j2 \
	&& rm -rf /home/mpcuser/build/ \
	&& rm -rf /home/mpcuser/MPC_${mpc_version}


