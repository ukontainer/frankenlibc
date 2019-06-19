RUMPKERN_CPPFLAGS="-D__linux__ -DCONFIG_LKL"

checkcheckout ()
{

	[ -f "${LKL_SRCDIR}/arch/lkl/Makefile" ] || \
	    die "Cannot find ${LKL_SRCDIR}/arch/lkl/Makefile!"

	[ ! -z "${TARBALLMODE}" ] && return

	if ! ${BRDIR}/checkout.sh checkcheckout ${LKL_SRCDIR} \
	    && ! ${TITANMODE}; then
		die 'revision mismatch, run checkout (or -H to override)'
	fi
}

makebuild ()
{
	echo "=== Linux build LKLSRC=${LKL_SRCDIR} ==="
        if [ -n "$(awk -W version < /dev/null 2>&1 | grep "mawk")" ]; then
		die "mawk is not supported."
	fi
	cd ${LKL_SRCDIR}
	LKL_VERBOSE="V=0"
	if [ ${NOISE} -gt 1 ] ; then
		LKL_VERBOSE="V=1"
		set -x
	fi

	LKL_CROSS=$(${CC} -dumpmachine)
	if [ ${LKL_CROSS} = "$(cc -dumpmachine)" ]
	then
		LKL_CROSS=
	else
		LKL_CROSS=${LKL_CROSS}-
	fi

	# clang does not have triple prefix
        if [ -n "$(${CC} -v 2>&1 | grep "clang")" ]; then
		LKL_CROSS=
	fi

	export LKL_VERBOSE
	export LKL_CROSS

	# need proper RUMP_PREFIX and RUMP_INCLUDE configuration from caller
	if [ -z "${RUMP_PREFIX:-}" ]; then
		die "No RUMP_PREFIX env configured. exit."
	fi

	set -e
	mkdir -p ${OBJDIR}/linux

        {
	cd tools/lkl
	rm -f ${OBJDIR}/linux/tools/lkl/lib/lkl.o
	${MAKE} CC=${CC} CROSS_COMPILE=${LKL_CROSS} ${LKL_EXT_OPT} -j ${JNUM} ${LKL_VERBOSE} O=${OBJDIR}/linux
	} 2>&1 ${OBJDIR}/linux-build.log

        if [ $? -ne 0 ]; then
		cat ${OBJDIR}/linux-build.log | tail -100
        fi

	cd ../../
	${MAKE} CROSS_COMPILE=${LKL_CROSS} ${LKL_EXT_OPT} headers_install ARCH=lkl O=${DESTDIR}/ \
	     PREFIX=/ INSTALL_HDR_PATH=${DESTDIR}/ ${LKL_VERBOSE}

	set +x
}

makeinstall ()
{

	# XXX for app-tools
	mkdir -p ${DESTDIR}/bin/
	mkdir -p ${DESTDIR}/include/rumprun

	# need proper RUMP_PREFIX and RUMP_INCLUDE configuration from caller
	${MAKE} CROSS_COMPILE=${LKL_CROSS} ${LKL_EXT_OPT} headers_install libraries_install DESTDIR=${DESTDIR}\
	     -C ./tools/lkl/ O=${OBJDIR}/linux  PREFIX=/ ${LKL_VERBOSE}
	# XXX: for netconfig.h
	mkdir -p ${DESTDIR}/include/rump/
	cp -pf ${BRDIR}/brlib/libnetconfig/rump/netconfig.h ${DESTDIR}/include/rump/
}

#
# install kernel headers.
# Note: Do _NOT_ do this unless you want to install a
#       full rump kernel application stack
#
makekernelheaders ()
{
	return
}

maketests ()
{
	printf 'SKIP: Linux test currently not implemented yet ... \n'
	return
	printf 'Linux test ... \n'
	${MAKE} -C ${LKL_SRCDIR}/tools/lkl test O=${OBJDIR}/linux || die LKL test failed
}
