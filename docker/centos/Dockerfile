FROM centos:7

RUN yum install -y --nogpgcheck \
	gcc gcc-c++ \
	make \
	patch \
	tar bzip2 \
	which \
	bash \
	&& yum clean all

RUN useradd -ms /bin/bash mpcuser && \
	chown -R mpcuser:mpcuser /opt

USER mpcuser
WORKDIR /home/mpcuser
RUN mkdir -p /home/mpcuser/{build,install}
CMD /bin/bash

LABEL \
	name = "Multi-Processor Computing Framework (MPC)" \
	description = "MPC provides implementations for the MPI, OpenMP and POSIX Threads standards to improve the scalability and performances of HPC applications" \
	license= "CeCILL-C (LGPL-compatible)" \
	builddate = "20171214" \
	maintainer.project = "Marc Perache <marc.perache@cea.fr>" \
	maintainer.docker = "Julien Adam <adamj@paratools.com>"

