#!/bin/sh -x
cd $(cd $(dirname $0);pwd)
#GDB="gdb -x gdbcmd --args"
VERBOSE="RUMP_VERBOSE=1"
rsync -avz . rockpi:frankenlibc --exclude '/.git/' --exclude="*.o" --exclude="*.a" --exclude="*.core"
ssh -t rockpi "cd frankenlibc; cp disk/disk.img disk/tmp.img; sh -c \"$VERBOSE $GDB rump/bin/rexec rumpobj/tests/fork-test rootfs:disk/tmp.img\""
