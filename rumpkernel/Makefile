COMPILE.c?=$(CC) -c $(CPPFLAGS) $(CFLAGS)
RUMPOBJ?=../../rumpobj
OBJDIR=$(RUMPOBJ)/rumpkernel/
RUMPDIR?=../../rump
LIBDIR=$(RUMPDIR)/lib
BINDIR?=$(RUMPDIR)/bin
REXEC=${BINDIR}/rexec
INCDIR=$(RUMPDIR)/include  -I../franken/ucontext -I../franken/thread -I../librumpuser
LDFLAGS+=-static
CFLAGS+=-g -I${INCDIR}

DEPS=

OBJ=$(RUMP_KERNEL).o

$(RUMP_KERNEL).o:	$(RUMP_KERNEL).c
			@mkdir -p $(OBJDIR)
			$(COMPILE.c) -o $@ $(RUMP_KERNEL).c


default:	$(OBJ)

.PHONY:    clean
clean:
	rm -f $(OBJ)
