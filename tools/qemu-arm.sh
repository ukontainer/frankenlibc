#!/bin/sh

qemu-system-arm -M versatilepb -m 512M -nographic -monitor null -serial null -semihosting -kernel "$@"
