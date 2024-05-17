MPC Docker Image Module
=======================

This module will help users and developers to build ready-to-compile
containers for the MPC framework. By running one of the prepared images, you
will be able to create an installation for MPC. We won't do it for you because
it is not an easy thing for a container to be run with different commands. So, a
container is always started with a '/bin/bash' program and are freely usable.

In this documentation, we will explain how we decide to design the Docker
layout, which images are built and how to build and run them easily.

Preamble
--------
A container is always set up with a non-privileged user named 'mpcuser' and has
its own HOME path (distinct from root). Two directories are basically created in
the HOME:

* `build`: intended to host the MPC build tree
* `install`: intended to host the install directory.

MPC needs the following for building without compiling issues:

* GCC suite: to build (at least) the embedded compiler/linker;
* Patch: to apply MPC-specific patches over extern-deps;
* tar / bzip2 : Some images already provide it, but some others don't;
* make : To run Make-based build systems;
* bash : This should not be necessary, but we observed that MPC is not strictly
sh-compliant (yet) and we rely on a complete 'bash' program to run `installmpc`.

The installation process does not need Autotools anymore and the package is not
included in images. You can consider adding it if necessary for you.  Please
remind that Docker should create the smallest image and anything not required
should be discarded. For this reason, we clear the package manager's cache each
time needed programs have been installed.

This first version contains 3 images, ready to be deployed for MPC:

* Centos 7 : built from the official Centos 7 image. The
  image is labeled `centos`;
* Debian 9 : built from the official Debian 9 image, added to the programs
	above. The image is labeled `debian`;
* Alpine3.5 : Not fully functional yet because of shell compliance (even when
        run with `bash`). We are working on it. The image is
	labeled `alpine`;

Build
-----
To build an image, a directory named according to the OS, contains a
`Dockerfile` command file, listing instructions to build the image. The
process is always the same :

1. Inheritance rule
2. Extra-packages
3. User creation
4. Environment set up
5. LABEL (=image description)

Depending on the image, the list of extra package to install may vary, because
all base OS images do not contain the same programs. To build an image, we
provide a script: `build_image.sh`, taking up to 1 argument: `img=*`, the image
name to build. A complete list of supported images can be found by using the
`list` argument;

For example, to build the `debian` image, You can run:

```
./build_image.sh img=debian
```

This command can take a while, because Docker may have to download the OS base
image and installing package in the build image. Once the command returns, you
can check the image by running `docker images`. You should see something like
that :

```
REPOSITORY    TAG        IMAGE ID        CREATED        SIZE
mpc-env       centos     24d0af4ef114    4 hours ago    322MB
mpc-env       alpine     5bf5646ed942    4 hours ago    160MB
mpc-env       debian     0c78d0eba456    4 hours ago    254MB
```

The next step is to create and run a container from an image.

Run
---

To run a container from an image, we provide a second script: `run_image.sh`. Be
aware that you can still use real Docker calls instead of these wrappers. This
script can take the following arguments:

* `img=*`: like for the build, you have to specify which image you want to use
  for creating a container (default: `alpine`). You can still list available
  images through the `list` argument;
* `--uniq`: This command let Docker know that once you exit the container, it
  can be destroyed. This option is useful to create 'debug' environment that
  could be thrown away once done;
* `--net`: Retrieve the image from the Docker Hub, instead of pre-building it.
  This behavior will be detailed later.

When running a container, we proceed in the following order:

* *If* the requested container already exists __AND__ is running, the current
  stdin/stderr/stdout are attached to the container. The command the prompt is
  attached to, depends on the current process inside the container;
* *else if* the requested container exists but is not running, it is started
  and attached to the current prompt. The mapped command depends on the `CMD`
  provided to the container when created. By default, we set up the `/bin/bash`
  command but it can be overridden by `docker run`.
* *else*, the container is created, started and bound to the current prompt.
  The default `bash` is executed. **The current MPC source path is mounted into
  the container under `/opt/src/mpc/`. This will let you build MPC by running
  `/opt/src/mpc/installmpc --prefix=/home/mpcuser/install` by yourself. Be aware
  that it is a mount point and not a copy. Anything done in the container will
  update your repository** . As MPC containers are designed to be used for
  validation more than for developing features, we plan to make the mount point
  __read-only__ to avoid undesired actions. If you specified `--uniq` the Docker
  `--rm` flag is forwarded to trigger the container deletion once exited.

The script will always ensure that there is only one match per managed
image/container. If you use Docker commands by yourself, this script could
become unable to understand what you want (multiple images/containers pointing
to the same name are not handled by this script).

To avoid building the image every time, MPC environments (i.e.: the created
image above) have been uploaded to the Docker Hub, under the 'paratoolsfrance'
Organization. This will let you download the image directly, instead of building
first (reachable through `docker pull paratoolsfrance/mpc-env:<os>` commands).
To enable this script to access to the Hub, you can pass `--net` to the script.
If the image cannot be found locally, it will try to download it from the Docker
Hub. Please note that

Useful tricks
-------------
* Need root access (temporarily) for a running container ?

```
docker exec -u 0 -it `docker ps -q -f "ancestor=mpc-env:<os>"` bash
```
