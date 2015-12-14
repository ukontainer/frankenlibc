#!/bin/sh

qemu-system-arm -M versatilepb -m 256M -nographic -monitor null -serial null -semihosting -kernel "$*"
