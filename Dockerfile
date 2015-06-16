# Basic build of frankenlibc with Docker

# XXX 3.2 not working yet, will fix shortly
FROM alpine:3.1

MAINTAINER Justin Cormack <justin@specialbusservice.com>

RUN \
  apk update && \
  apk upgrade && \
  apk add \
  build-base \
  linux-headers \
  gawk \
  sed \
  zlib-dev \
  openssl-dev \
  ncurses-dev \
  file \
  wget \
  git \
  rsync \
  m4 \
  strace \
  cdrkit \
  coreutils \
  curl

# While still in testing
RUN \
  apk -X http://nl.alpinelinux.org/alpine/edge/testing update && \
  apk -X http://nl.alpinelinux.org/alpine/edge/testing add libseccomp libseccomp-dev

COPY . /usr/src/frankenlibc

ENV SUDO_UID=1000

RUN \
  cd /usr/src/frankenlibc && \
  ./build.sh -d /usr/local/rump -b /usr/local/bin seccomp && \
  cp rumpobj/tests/hello /usr/local/bin/rump.helloworld && \
  make clean
