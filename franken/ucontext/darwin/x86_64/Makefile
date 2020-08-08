COMPILE.c?=$(CC) -c $(CPPFLAGS) $(CFLAGS)
COMPILE.S?=$(CC) -c $(CPPFLAGS) $(ASFLAGS)
RUMPOBJ?=../../../../rumpobj
OBJDIR=$(RUMPOBJ)/franken
# XXX: those are picked from https://github.com/apple/darwin-libplatform
OBJ=$(OBJDIR)/setcontext.o $(OBJDIR)/_ctx_start.o $(OBJDIR)/_setcontext.o $(OBJDIR)/getcontext.o
OBJ+=$(OBJDIR)/getmcontext.o $(OBJDIR)/swapcontext.o $(OBJDIR)/makecontext.o
INCLUDES=-I../.. -I../../include
CPPFLAGS+=$(INCLUDES)
CFLAGS+=-fno-stack-protector -Wno-nullability-completeness

default:			$(OBJ)

$(OBJDIR)/getmcontext.o:	getmcontext.c
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -static -o $@ $^

$(OBJDIR)/swapcontext.o:	swapcontext.c
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -o $@ $^

$(OBJDIR)/setcontext.o:	setcontext.c
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -o $@ $^

$(OBJDIR)/makecontext.o:	makecontext.c
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -o $@ $^

$(OBJDIR)/_ctx_start.o:	_ctx_start.s
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -o $@ $^

$(OBJDIR)/_setcontext.o:	_setcontext.s
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -o $@ $^

$(OBJDIR)/getcontext.o:	getcontext.s
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -o $@ $^

.PHONY:				clean
clean:		
				rm -f $(OBJ)
