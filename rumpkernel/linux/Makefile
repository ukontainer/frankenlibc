RUMPOBJ?=../../rumpobj
OBJDIR=$(RUMPOBJ)/rumpkernel/
RUMPDIR?=../../rump
LIBDIR=$(RUMPDIR)/lib
BINDIR?=$(RUMPDIR)/bin
REXEC=${BINDIR}/rexec
INCDIR=$(RUMPDIR)/include
LDFLAGS+=-static
CFLAGS+=-g -I${INCDIR}

DEPS= #$(LIBDIR)/libc.a

OBJ=

default:		$(OBJ)

.PHONY:				clean
clean:		
				rm -f $(OBJ)
