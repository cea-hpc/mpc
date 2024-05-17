ARG img_tag=stretch

FROM debian:${img_tag} AS env

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
RUN mkdir -p /build /install && chown mpcuser:mpcuser /build /install

USER mpcuser
WORKDIR /home/mpcuser
CMD /bin/bash

LABEL \
	name = "Multi-Processor Computing Framework (MPC)" \
	description = "MPC provides implementations for the MPI, OpenMP and POSIX Threads standards to improve the scalability and performances of HPC applications" \
	license= "CeCILL-C (LGPL-compatible)" \
	builddate = "20191020" \
	maintainer.project = "Marc Perache <marc.perache@cea.fr>" \
	maintainer.docker = "Julien Adam <adamj@paratools.com>"

FROM env AS inst
ARG mpc_version=3.3.1

ADD --chown=mpcuser:mpcuser https://france.paratools.com/mpc/releases/MPC_${mpc_version}.tar.gz /build/
RUN cd /build \
	&& tar -xf MPC_${mpc_version}.tar.gz \
	&& /build/MPC_${mpc_version}/installmpc --prefix=/install/ --disable-color -j2 \
	&& rm -rf /build/*
COPY docker_entrypoint.sh /install/
ENTRYPOINT ["/bin/bash", "/install/docker_entrypoint.sh"]

FROM inst AS demo

USER root
RUN apt-get update && apt-get install --no-install-recommends -y \
	vim \
	emacs \
	nano \
	gdb \
	htop \
	&& apt-get autoremove -y \
	&& rm -rf /var/lib/apt/lists/*
USER mpcuser
