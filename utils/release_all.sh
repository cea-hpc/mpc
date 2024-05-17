#!/bin/sh

for i in "bullseye" "buster"; do
	utils/prepare_release.sh deb --distribution=debian:$i --target=$1;
done
for i in "20.04" "22.04" "22.10"; do
	utils/prepare_release.sh deb --distribution=ubuntu:$i --target=$1;
done
for i in 35 36 37; do
	utils/prepare_release.sh rpm --distribution=fedora:$i --target=$1;
done
for i in 9; do
	utils/prepare_release.sh rpm --distribution=rockylinux:$i --target=$1;
done
