#!/bin/sh
docker rm -f frankenlibc_build_aarch64 || :
docker run -d -it -v $PWD:/home/rock/frankenlibc -w /home/rock/frankenlibc --name frankenlibc_build_aarch64 frankenlibc_build_aarch64 sh
docker exec -w /home/rock/frankenlibc/busybox-1.32.0 frankenlibc_build_aarch64 make defconfig
docker exec -w /home/rock/frankenlibc/busybox-1.32.0 frankenlibc_build_aarch64 sh -c "sleep 1; sed '/CONFIG_NOMMU/s/.*/CONFIG_NOMMU=y/' -i .config; sed '/CONFIG_DEBUG /s/.*/CONFIG_DEBUG=y/' -i .config; sed '/CONFIG_DEBUG_PESSIMIZE/s/.*/CONFIG_DEBUG_PESSIMIZE=y/' -i .config"
docker exec -w /home/rock/frankenlibc/busybox-1.32.0 frankenlibc_build_aarch64 make busybox -j4
docker rm -f frankenlibc_build_aarch64 || :
