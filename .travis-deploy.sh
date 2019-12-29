#!/bin/bash

DESTDIR="/opt/rump"
PUB_FILENAME=frankenlibc.tar.gz

if [ x$DEPLOY = "xskip" ] ; then
    exit
fi

# skip deploy if it's not upstream repo and branch
if [ $TRAVIS_REPO_SLUG != "ukontainer/frankenlibc" ] ||
   [ $TRAVIS_BRANCH != "lkl-musl-macho" ] ; then
    exit
fi

if [ $CC = "gcc" ] ; then
    PUB_FILENAME=frankenlibc-gcc.tar.gz
fi

cp rumpobj/tests/ping $DESTDIR/bin
cp rumpobj/tests/ping6 $DESTDIR/bin
cp rumpobj/tests/hello $DESTDIR/bin
cp rumpobj/tests/lkick $DESTDIR/bin


# deploy to bintray
tar cfz /tmp/frankenlibc.tar.gz $DESTDIR
curl -T /tmp/frankenlibc.tar.gz -u$BINTRAY_USER:$BINTRAY_APIKEY \
     "https://api.bintray.com/content/ukontainer/ukontainer/frankenlibc/dev/$TRAVIS_OS_NAME/$ARCH/$PUB_FILENAME;override=1;publish=1"
# publish !
curl -X POST -u$BINTRAY_USER:$BINTRAY_APIKEY \
     https://api.bintray.com/content/ukontainer/ukontainer/frankenlibc/dev/publish

