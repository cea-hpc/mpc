FROM alpine:3.5

RUN apk update && apk add --no-cache \
	gcc g++ \
	make \
	patch \
	tar bzip2 \
	which \
	bash

RUN adduser -Ds /bin/bash mpcuser && \
	mkdir -p /opt/src && \
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
