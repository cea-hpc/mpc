FROM debian:9

RUN apt-get update && apt-get install --no-install-recommends -y \
	gcc g++ \
	make \
	patch \
	bzip2 \
	&& apt-get autoremove -y \
	&& rm -rf /var/lib/apt/lists/*

RUN useradd -ms /bin/bash mpcuser && \
	chown -R mpcuser:mpcuser /opt

USER mpcuser
WORKDIR /home/mpcuser
RUN mkdir -p /home/mpcuser/build /home/mpcuser/install
CMD /bin/bash

LABEL \
	name = "Multi-Processor Computing Framework (MPC)" \
	description = "MPC provides implementations for the MPI, OpenMP and POSIX Threads standards to improve the scalability and performances of HPC applications" \
	license= "CeCILL-C (LGPL-compatible)" \
	builddate = "20171214" \
	maintainer.project = "Marc Perache <marc.perache@cea.fr>" \
	maintainer.docker = "Julien Adam <adamj@paratools.com>"
