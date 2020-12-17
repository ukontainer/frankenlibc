#!/bin/sh
cd $(cd $(dirname $0);pwd)
docker rm -f frankenlibc_build || :
docker rm -f frankenlibc_build_aarch64 || :
docker run -d -it -v $PWD:/home/rock/frankenlibc -w /home/rock/frankenlibc --name frankenlibc_build frankenlibc_build bash
docker run -d -it -v $PWD:/home/rock/frankenlibc -w /home/rock/frankenlibc --name frankenlibc_build_aarch64 frankenlibc_build_aarch64 sh
docker exec frankenlibc_build sh -c "CC=aarch64-linux-gnu-gcc ./build.sh -k linux notests"
docker exec frankenlibc_build_aarch64 gcc -ggdb3 -fPIE -pie disk/hello.c -o disk/hello
docker exec frankenlibc_build_aarch64 gcc -ggdb3 -fPIE -pie disk/hello2.c -o disk/hello2
mkdir -p disk/mnt
sudo umount disk/mnt
rm -rf disk/disk.img
dd if=/dev/zero of=disk/disk.img bs=1024 count=102400
mkfs.ext4 -F disk/disk.img
sudo mount -t ext4 -o loop disk/disk.img disk/mnt
sudo cp disk/hello disk/mnt/
sudo cp disk/hello2 disk/mnt/
sudo mkdir -p disk/mnt/lib
sudo cp rump/lib/libc.so disk/mnt/lib/ld-musl-aarch64.so.1
sudo cp rump/lib/libc.so disk/mnt/lib/libc.musl-aarch64.so.1
sudo mkdir -p disk/mnt/bin
#sudo docker cp frankenlibc_build_aarch64:/bin/busybox disk/mnt/bin/ash
#sudo docker cp frankenlibc_build_aarch64:/bin/busybox disk/mnt/bin/ls
sudo cp busybox-1.32.0/busybox_unstripped disk/mnt/bin/busybox
sudo cp busybox-1.32.0/busybox_unstripped disk/mnt/bin/hush
sudo umount disk/mnt
docker rm -f frankenlibc_build || :
docker rm -f frankenlibc_build_aarch64 || :
