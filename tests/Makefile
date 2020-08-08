RUMPOBJ?=../rumpobj
OBJDIR=$(RUMPOBJ)/tests
RUMPDIR?=../rump
LIBDIR=$(RUMPDIR)/lib
BINDIR?=$(RUMPDIR)/bin
REXEC=${BINDIR}/rexec
INCDIR=$(RUMPDIR)/include
ifneq ($(OS),darwin)
LDFLAGS+=-static
endif
LDFLAGSXX+=
CFLAGS+=

DEPS=$(LIBDIR)/libc.a

OBJ=$(OBJDIR)/hello $(OBJDIR)/ctor $(OBJDIR)/cat
OBJ+=$(OBJDIR)/fork-test $(OBJDIR)/hello-pthread
OBJ+=$(OBJDIR)/lkick $(OBJDIR)/misc
ifneq ($(wildcard $(CXX)),)
OBJ+=$(OBJDIR)/hellopp
endif

default:		$(OBJ)

run-simple:		$(OBJ)
			RUMP_VERBOSE=1 ${REXEC} $(OBJDIR)/hello
			echo frankenlibc | ${REXEC} $(OBJDIR)/cat

run:			$(OBJ)
			dd if=/dev/zero of=disk.img bs=1024 count=20480
			mkfs.ext4 -F disk.img
			RUMP_VERBOSE=1 ${REXEC} $(OBJDIR)/hello rootfs:disk.img
			${REXEC} $(OBJDIR)/ctor disk.img
			echo frankenlibc | ${REXEC} $(OBJDIR)/cat rootfs:disk.img
ifneq ($(wildcard $(OBJDIR)/ping),)
			${REXEC} ${OBJDIR}/ping rootfs:disk.img -- -c 3 127.0.0.1
			${REXEC} ${OBJDIR}/ping6 rootfs:disk.img -- -c 3 ::1
endif
			RUMP_VERBOSE=1 ${REXEC} $(OBJDIR)/fork-test rootfs:disk.img || true
			RUMP_VERBOSE=1 ${REXEC} $(OBJDIR)/misc
			${REXEC} $(OBJDIR)/hello-pthread
ifneq ($(wildcard $(OBJDIR)/hellopp),)
			${REXEC} $(OBJDIR)/hellopp
endif
			rm -f disk.img

$(OBJDIR)/ctor:		ctor.c $(DEPS)
			@mkdir -p $(OBJDIR)
			${CC} ${CFLAGS} ${LDFLAGS} -o $@ ctor.c

$(OBJDIR)/hello:	hello.c $(DEPS)
			@mkdir -p $(OBJDIR)
			${CC} ${CFLAGS} ${LDFLAGS} -o $@ hello.c

$(OBJDIR)/hellopp:	hellopp.cc $(DEPS)
			@mkdir -p $(OBJDIR)
			${CXX} ${CFLAGS} ${LDFLAGS} -o $@ hellopp.cc ${LDFLAGSXX}

$(OBJDIR)/hello-pthread: hello-pthread.c $(DEPS)
			@mkdir -p $(OBJDIR)
			${CC} ${CFLAGS} ${LDFLAGS} -o $@ hello-pthread.c

$(OBJDIR)/cat:		cat.c $(DEPS)
			@mkdir -p $(OBJDIR)
			${CC} ${CFLAGS} ${LDFLAGS} -o $@ cat.c

$(OBJDIR)/fork-test:	fork-test.c $(DEPS)
			@mkdir -p $(OBJDIR)
			${CC} ${CFLAGS} ${LDFLAGS} -o $@ fork-test.c

$(OBJDIR)/lkick:	lkick.c $(DEPS)
			@mkdir -p $(OBJDIR)
			${CC} ${CFLAGS} ${LDFLAGS} -o $@ lkick.c

$(OBJDIR)/misc:		misc.c $(DEPS)
			@mkdir -p $(OBJDIR)
			${CC} ${CFLAGS} ${LDFLAGS} -o $@ misc.c

.PHONY:			test clean
clean:		
			rm -f $(OBJ)
			${MAKE} clean -C iputils
