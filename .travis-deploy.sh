#!/bin/bash

DESTDIR=${DESTDIR:-/opt/rump}
RUMPOBJ=${RUMPOBJ:-rumpobj}
PUB_FILEPREFIX=frankenlibc

if [ x$DEPLOY = "xskip" ] ; then
    exit
fi

# XXX: revert this
# ||
#   [ $TRAVIS_BRANCH != "lkl-musl-macho" ] 
# skip deploy if it's not upstream repo and branch
if [ $TRAVIS_REPO_SLUG != "ukontainer/frankenlibc" ] ; then
    exit
fi

if [ $CC = "gcc" ] && [ $TRAVIS_ARCH = "amd64" ] ; then
    PUB_FILEPREFIX=frankenlibc-gcc
fi

if [ $DESTDIR = "/opt/rump-tiny" ] ; then
    PUB_FILEPREFIX=${PUB_FILEPREFIX}-tiny
fi

cp $RUMPOBJ/tests/ping $DESTDIR/bin
cp $RUMPOBJ/tests/ping6 $DESTDIR/bin
cp $RUMPOBJ/tests/hello $DESTDIR/bin
cp $RUMPOBJ/tests/lkick $DESTDIR/bin


# deploy to github release
tar cfz /tmp/frankenlibc.tar.gz $DESTDIR
set -x
TAG=dev
ACCEPT_HEADER="Accept: application/vnd.github.jean-grey-preview+json"
TOKEN_HEADER="Authorization: token $GITHUB_TOKEN"
ENDPOINT="https://api.github.com/repos/$TRAVIS_REPO_SLUG/releases"


echo "Creating new release as version ${TAG}..."
REPLY=$(curl -s -H "${ACCEPT_HEADER}" -H "${TOKEN_HEADER}" -d "{\"tag_name\": \"${TAG}\", \"name\": \"${TAG}\"}" "${ENDPOINT}")

# Check error
RELEASE_ID=$(echo "${REPLY}" | jq .id)

# retry for pre-existing tag case
if [ "${RELEASE_ID}" = "null" ] || [ "${RELEASE_ID}" = "" ] ; then
RELEASE_ID=$(curl -H "${TOKEN_HEADER}" ${ENDPOINT} | jq -r ".[] | select(.tag_name == \"$TAG\") | .id")
fi

if [ "${RELEASE_ID}" = "null" ]; then
  echo "Failed to create release. Please check your configuration. Github replies:"
  echo "${REPLY}"
  exit 1
fi

echo "Github release created as ID: ${RELEASE_ID}"
RELEASE_URL="https://uploads.github.com/repos/$TRAVIS_REPO_SLUG/releases/${RELEASE_ID}/assets"


# Uploads artifacts
FILE=/tmp/frankenlibc.tar.gz
MIME=$(file -b --mime-type "${FILE}")
NAME=$PUB_FILEPREFIX-$ARCH-$TRAVIS_OS_NAME.tar.gz

# delete previous asset
ASSET_ID=$(curl -s -H "${TOKEN_HEADER}" ${ENDPOINT} | jq -r ".[] | select(.tag_name == \"$TAG\") | .assets | .[] | select(.name == \"$NAME\") | .id")
echo "Deleting previous assets ${NAME} if any..."

curl  \
    -X DELETE \
    -H "${TOKEN_HEADER}" \
    -H "Accept: application/vnd.github.v3+json" \
    "${ENDPOINT}/assets/"$ASSET_ID || true

echo "Uploading assets ${NAME} as ${MIME}..."
curl -v \
     -H "${ACCEPT_HEADER}" \
     -H "${TOKEN_HEADER}" \
     -H "Content-Type: ${MIME}" \
     --data-binary "@${FILE}" \
     "${RELEASE_URL}?name=${NAME}"

echo "Finished."

