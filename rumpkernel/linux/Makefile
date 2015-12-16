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

OBJ=$(OBJDIR)/netconfig.o

default:		$(OBJ)

$(OBJDIR)/netconfig.o:		netconfig.c $(DEPS)
				@mkdir -p $(OBJDIR)
				${CC} ${CFLAGS} ${LDFLAGS} -c -o $@ netconfig.c

.PHONY:				clean
clean:		
				rm -f $(OBJ)
