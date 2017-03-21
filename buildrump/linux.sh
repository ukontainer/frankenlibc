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
	cd ${LKL_SRCDIR}
	VERBOSE="V=0"
	if [ ${NOISE} -gt 1 ] ; then
		VERBOSE="V=1"
	fi

	CROSS=$(${CC} -dumpmachine)
	if [ ${CROSS} = "$(gcc -dumpmachine)" ]
	then
		CROSS=
	else
		CROSS=${CROSS}-
	fi

	set -e
	set -x
	export RUMP_PREFIX=${SRCDIR}/sys/rump
	export RUMP_INCLUDE=${SRCDIR}/sys/rump/include
	mkdir -p ${OBJDIR}/linux

	cd tools/lkl
	rm -f ${OBJDIR}/linux/tools/lkl/lib/lkl.o
	make CROSS_COMPILE=${CROSS} rumprun=yes -j ${JNUM} ${VERBOSE} O=${OBJDIR}/linux

	cd ../../
	make CROSS_COMPILE=${CROSS} headers_install ARCH=lkl O=${DESTDIR}/ PREFIX=/ INSTALL_HDR_PATH=${DESTDIR}/

	set +x
}

makeinstall ()
{

	# XXX for app-tools
	mkdir -p ${DESTDIR}/bin/
	mkdir -p ${DESTDIR}/include/rumprun

	export RUMP_PREFIX=${SRCDIR}/sys/rump
	export RUMP_INCLUDE=${SRCDIR}/sys/rump/include
	make rumprun=yes headers_install libraries_install DESTDIR=${DESTDIR}\
	     -C ./tools/lkl/ O=${OBJDIR}/linux  PREFIX=/
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
	make -C ${LKL_SRCDIR}/tools/lkl test O=${OBJDIR}/linux || die LKL test failed
}
