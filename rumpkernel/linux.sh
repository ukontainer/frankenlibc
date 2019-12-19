#!/bin/sh

if [ -z ${BUILD_QUIET} ] ; then
    VERBOSE=1
else
    VERBOSE=
fi

rumpkernel_buildrump()
{

export RUMP_PREFIX=${PWD}/librumpuser/
export KOPT='buildrump=yes'
export LKL_EXT_OPT="rumprun=no ${KOPT}"
export EXTRA_CFLAGS="${EXTRA_CFLAGS}"

./buildrump/buildrump.sh \
	-V RUMP_CURLWP=hypercall -V RUMP_LOCKS_UP=yes \
	-V MKPIC=no -V RUMP_KERNEL_IS_LIBC=1 \
	-F CFLAGS="${EXTRA_CFLAGS}" \
	-k -s ${RUMPSRC} -o ${RUMPOBJ} -d ${RUMP} \
	${BUILD_QUIET} ${STDJ} \
	-F AFLAGS="${EXTRA_AFLAGS}" \
	${EXTRAFLAGS} \
	-l linux tools build install

}

rumpkernel_createuserlib()
{
# build musl libc for Linux
(
	if [ -z "${BUILD_QUIET}" ] ; then set -x ;fi
	set -e
	echo "=== building musl ==="
	cd musl

	# musl needs to be built with gcc for libc.so
        if [ -n "$(${CC} -v 2>&1 | grep "clang")" ]; then
		# XXX: should properly handle cross-build environment
		CC=gcc
	fi

	LKL_HEADER="${RUMP}/"
	CIRCLE_TEST_REPORTS="${CIRCLE_TEST_REPORTS-./}"
	CC=${CC:-gcc} ./configure --with-lkl=${LKL_HEADER} --disable-shared --enable-debug \
		    --disable-optimize --prefix=${RUMPOBJ}/musl CFLAGS="${EXTRA_CFLAGS}"
	# XXX: bug of musl Makefile ?
	${MAKE} obj/src/internal/version.h
	CC=${CC:-gcc} ${MAKE} install
	# install libraries
	${INSTALL-install} -d ${OUTDIR}/lib
	${INSTALL-install} ${RUMPOBJ}/musl/lib/libpthread.a \
			   ${RUMPOBJ}/musl/lib/libcrypt.a \
			   ${RUMPOBJ}/musl/lib/librt.a \
			   ${RUMPOBJ}/musl/lib/libm.a \
			   ${RUMPOBJ}/musl/lib/libdl.a \
			   ${RUMPOBJ}/musl/lib/libutil.a \
			   ${RUMPOBJ}/musl/lib/libresolv.a \
			   ${OUTDIR}/lib
)
}

rumpkernel_install_header()
{
	## FIXME: MUSL_LIBC is somehow misleading as franken also uses.
	## LINUX_LIBC?
	appendvar FRANKEN_FLAGS "-DMUSL_LIBC"
	appendvar EXTRA_CPPFLAGS "-DCONFIG_LKL"
	appendvar EXTRA_CFLAGS "-DCONFIG_LKL"

	# install headers
	cp -a ${RUMP}/include/* ${OUTDIR}/include
	cp -a ${RUMPOBJ}/musl/include/* ${OUTDIR}/include

	# only for mach-o
	if [ "${OS}" = "darwin" ]; then
		cd franken/ucontext && \
		    ln -sf `xcrun --show-sdk-path`/usr/include ./ && \
		    cd ../..
	fi

}

[ ${OS} = "freebsd" ] && appendvar UNDEF "-U__FreeBSD__"
[ ${OS} = "darwin" ] && appendvar UNDEF "-U__APPLE__"
# static c++ binary needs __dso_handle (used by atexit of deconstructor),
# defined by gnu library
if [ "${OS}" = "linux" ] ; then
    export EXTRA_LDSCRIPT_CC="-Wl,-defsym,__dso_handle=0 -Wl,-defsym,__cxa_thread_atexit_impl=0"
fi
if [ "${OS}" = "darwin" ] ; then
    export EXTRA_LDSCRIPT_CC="-Wl,-alias,_rumpns__stext,___eh_frame_start \
     -Wl,-alias,_rumpns__stext,___eh_frame_end -Wl,-alias,_rumpns__stext,___eh_frame_hdr_start \
     -Wl,-alias,_rumpns__stext,___eh_frame_hdr_end"
fi

rumpkernel_install_extra_libs ()
{
	appendvar UNDEF "-D__linux__ -DCONFIG_LKL -D__RUMPRUN__ -D__linux"
	if [ "${OS}" = "linux" ]; then
	    sudo setcap cap_net_raw=ep ${BINDIR}/rexec \
		|| echo "setcap failed. ignoring"
	fi
	return 0
}

rumpkernel_explode_libc()
{
(
	cd ${RUMPOBJ}/explode/musl
	${AR-ar} x ${RUMPOBJ}/musl/lib/libc.a

	# fixup for case-insensitive fs on macOS
	if [ ${OS} = "darwin" ] ; then
		rm -f _exit.o _Exit.o
		${AR-ar} x ${RUMPOBJ}/musl/lib/libc.a _Exit.o
		mv _Exit.o musl_Exit.o
		${AR-ar} x ${RUMPOBJ}/musl/lib/libc.a _exit.o
	fi

	cp ${RUMPOBJ}/${RUMP_KERNEL}.o ./

	LKL_CROSS=$(${CC:-gcc} -dumpmachine)
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

	# XXX: ld.gold generates _end BSS symbol at link time
	if [ ${OS} = "linux" ] ; then
		${LKL_CROSS}objcopy --redefine-sym _end=rumpns__end ${RUMPOBJ}/linux/tools/lkl/liblkl.a
	fi
)
	LIBC_DIR=musl

}

rumpkernel_build_extra()
{
	CFLAGS="${EXTRA_CFLAGS}" \
		${MAKE} -C rumpkernel/ RUMP_KERNEL=${RUMP_KERNEL}
	cp rumpkernel/${RUMP_KERNEL}.o ${RUMPOBJ}/
	${MAKE} -C rumpkernel/ clean RUMP_KERNEL=${RUMP_KERNEL}
	return 0
}

rumpkernel_maketools()
{
	return 0
}

rumpkernel_build_test()
{
	OBJDIR=${RUMPOBJ}/tests

	# XXX qemu-arm still has an issue on pthread_self() during setuid
	if [ ${OS} != "qemu-arm" ] ;
	then
		${MAKE} -C tests/iputils clean
		CC="${BINDIR}/${COMPILER}" LDFLAGS="-static" ${MAKE} -C tests/iputils ping ping6
		cp tests/iputils/ping tests/iputils/ping6 ${OBJDIR}/
		${MAKE} -C tests/iputils clean
	fi
}

rumpkernel_install_libcxx()
{
	C_COMPILER=${OUTDIR}/bin/${TOOL_PREFIX}-cc
	CXX_COMPILER=${OUTDIR}/bin/${TOOL_PREFIX}-c++
	LLVM_ROOT_PATH=${PWD}/llvm
	LLVM_PATH=${LLVM_ROOT_PATH}/llvm

# build libunwind for Linux
(
        if [ -z "${BUILD_QUIET}" ] ; then set -x ; fi
        set -e
        echo "#define __WORDSIZE 64" > "${OUTDIR}/include/bits/wordsize.h"
        sed -i -e "$ s/#endif/#define __GLIBC_PREREQ(maj, min) 0\n#endif/g" ${OUTDIR}/include/features.h
        echo "=== building libunwind ==="
        mkdir -p ${RUMPOBJ}/libunwind
        cd ${RUMPOBJ}/libunwind
        LIBUNWIND_FLAGS="-I${OUTDIR}/include -D_LIBUNWIND_IS_BAREMETAL=1"
        cmake \
          -DCMAKE_CROSSCOMPILING=True \
          -DCMAKE_C_COMPILER=${C_COMPILER} \
          -DCMAKE_C_FLAGS="${LIBUNWIND_FLAGS}" \
          -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
          -DCMAKE_CXX_FLAGS="${LIBUNWIND_FLAGS}" \
          -DCMAKE_INSTALL_PREFIX="${OUTDIR}" \
          -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
          -DLIBUNWIND_ENABLE_SHARED=0 \
          -DLIBUNWIND_ENABLE_STATIC=1 \
          -DLIBUNWIND_ENABLE_CROSS_UNWINDING=1 \
          -DLLVM_COMPILER_CHECKED=1 \
          -DLLVM_PATH=${LLVM_PATH} \
          ${LLVM_ROOT_PATH}/libunwind
        ${MAKE} VERBOSE=${VERBOSE}
        ${MAKE} install
)
# build libc++abi for Linux
(
	if [ -z "${BUILD_QUIET}" ] ; then set -x ; fi
        set -e
        echo "=== building libc++abi ==="
        mkdir -p ${RUMPOBJ}/libcxxabi
        cd ${RUMPOBJ}/libcxxabi
        LIBCXXABI_FLAGS="-I${OUTDIR}/include -D_GNU_SOURCE"
        cmake \
          -DCMAKE_CROSSCOMPILING=True \
          -DCMAKE_C_COMPILER=${C_COMPILER} \
          -DCMAKE_C_FLAGS="${LIBCXXABI_FLAGS}" \
          -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
          -DCMAKE_CXX_FLAGS="${LIBCXXABI_FLAGS}" \
          -DCMAKE_INSTALL_PREFIX="${OUTDIR}" \
          -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
          -DCMAKE_SHARED_LINKER_FLAGS="-L${OUTDIR}/lib" \
          -DLIBCXXABI_USE_LLVM_UNWINDER=1 \
          -DLIBCXXABI_ENABLE_STATIC_UNWINDER=1 \
          -DLIBCXXABI_LIBUNWIND_PATH=${LLVM_ROOT_PATH}/unwind \
          -DLIBCXXABI_LIBCXX_INCLUDES=${LLVM_ROOT_PATH}/libcxx/include \
          -DLIBCXXABI_ENABLE_SHARED=0 \
          -DLIBCXXABI_ENABLE_STATIC=1 \
          -DLIBCXXABI_BAREMETAL=1 \
          -DLLVM_COMPILER_CHECKED=1 \
          -DLLVM_PATH=${LLVM_PATH} \
          ${LLVM_ROOT_PATH}/libcxxabi
        ${MAKE} VERBOSE=${VERBOSE}
        ${MAKE} install
)
# build libc++ for Linux
(
	if [ -z "${BUILD_QUIET}" ] ; then set -x ; fi
        set -e
        cp -f ${RUMP}/include/generated/uapi/linux/version.h ${OUTDIR}/include/linux/
        echo "=== building libc++ ==="
        mkdir -p ${RUMPOBJ}/libcxx
        cd ${RUMPOBJ}/libcxx
        LIBCXX_FLAGS="-I${OUTDIR}/include -D_GNU_SOURCE"
        if [ "${OS}" = "darwin" ] ; then STATIC_ABI_LIBRARY=0 ;
        else STATIC_ABI_LIBRARY=1; fi
        cmake \
          -DCMAKE_CROSSCOMPILING=True \
          -DCMAKE_C_COMPILER=${C_COMPILER} \
          -DCMAKE_C_FLAGS="${LIBCXX_FLAGS}" \
          -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
          -DCMAKE_CXX_FLAGS="${LIBCXX_FLAGS}" \
          -DCMAKE_INSTALL_PREFIX="${OUTDIR}" \
          -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
          -DCMAKE_SHARED_LINKER_FLAGS="${OUTDIR}/lib" \
          -DLIBCXX_CXX_ABI=libcxxabi \
          -DLIBCXX_CXX_ABI_LIBRARY_PATH="${OUTDIR}/lib" \
          -DLIBCXX_CXX_ABI_INCLUDE_PATHS=${LLVM_ROOT_PATH}/libcxxabi/include \
          -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=${STATIC_ABI_LIBRARY} \
          -DLIBCXX_ENABLE_SHARED=0 \
          -DLIBCXX_ENABLE_STATIC=1 \
          -DLIBCXX_HAS_MUSL_LIBC=1 \
          -DLIBCXX_HAS_GCC_S_LIB=0 \
          -DLLVM_COMPILER_CHECKED=1 \
          -DLLVM_PATH=${LLVM_PATH} \
          ${LLVM_ROOT_PATH}/libcxx
        ${MAKE} VERBOSE=${VERBOSE}
        ${MAKE} install
)
# append cxxflags for libc++
(
if [ "${OS}" = "linux" ] ; then
	sed -i --follow-symlinks "5s/$/ -stdlib=libc++ -lc++ -lc++abi/" ${OUTDIR}/bin/${TOOL_PREFIX}-c++
elif [ "${OS}" = "darwin" ] ; then
	sed -i --follow-symlinks "5s/$/ -stdlib=libc++ -lc++ -lc++abi -lunwind/" ${OUTDIR}/bin/${TOOL_PREFIX}-c++
fi
)
}
