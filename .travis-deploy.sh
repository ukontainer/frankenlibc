#!/bin/bash

DESTDIR="/opt/rump"

# XXX for python3 osx build
if [ $TRAVIS_OS_NAME == "osx" ] ; then
	TMPCWD=`pwd`
	cd $DESTDIR/lib && ln -sf libc.a libm.a && cd $TMPCWD
	cd $DESTDIR/lib && ln -sf libc.a libpthread.a && cd $TMPCWD
	cd $DESTDIR/lib && ln -sf libc.a libdl.a && cd $TMPCWD
	cd $DESTDIR/bin && ln -sf x86_64-rumprun-linux-clang x86_64-rumprun-linux-gcc \
	    && rm -f x86_64-rumprun-linux-cc && cd $TMPCWD
fi


cp rumpobj/tests/ping $DESTDIR/bin
cp rumpobj/tests/ping6 $DESTDIR/bin
cp rumpobj/tests/hello $DESTDIR/bin


# deploy to bintray
tar cfz /tmp/frankenlibc.tar.gz $DESTDIR
curl -T /tmp/frankenlibc.tar.gz -u$BINTRAY_USER:$BINTRAY_APIKEY \
     "https://api.bintray.com/content/libos-nuse/x86_64-rumprun-linux/frankenlibc/dev/$TRAVIS_OS_NAME/frankenlibc.tar.gz;override=1;publish=1"
# publish !
curl -X POST -u$BINTRAY_USER:$BINTRAY_APIKEY \
     https://api.bintray.com/content/libos-nuse/x86_64-rumprun-linux/frankenlibc/dev/publish

