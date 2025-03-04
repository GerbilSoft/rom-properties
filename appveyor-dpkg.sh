#!/bin/sh
set -ev

add-apt-repository --remove "deb http://apt.postgresql.org/pub/repos/apt/ bionic-pgdg main"

apt-get update
apt-get -y install \
	libcurl4-openssl-dev \
	zlib1g-dev \
	libpng-dev \
	libjpeg-dev \
	nettle-dev \
	pkg-config \
	libtinyxml2-dev \
	gettext \
	libseccomp-dev \
	libfmt-dev \
	\
	libzstd-dev \
	liblz4-dev \
	liblzo2-dev \
	\
	qtbase5-dev \
	qttools5-dev-tools \
	extra-cmake-modules \
	libkf5kio-dev \
	libkf5widgetsaddons-dev \
	libkf5filemetadata-dev \
	libkf5crash-dev \
	\
	libglib2.0-dev \
	libgtk2.0-dev \
	libgdk-pixbuf2.0-dev \
	libthunarx-3-dev \
	libcanberra-dev \
	libgsound-dev \
	libgtk-3-dev \
	libcairo2-dev \
	libnautilus-extension-dev \
	libblkid-dev
