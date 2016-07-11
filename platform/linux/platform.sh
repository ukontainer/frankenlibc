#!/bin/sh

sudo setcap cap_net_raw=ep ${BINDIR}/rexec
