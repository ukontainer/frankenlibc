COMPILE.c?=$(CC) -c $(CPPFLAGS) $(CFLAGS)
COMPILE.S?=$(CC) -c $(CPPFLAGS) $(ASFLAGS)
RUMPOBJ?=../../../../rumpobj
OBJDIR=$(RUMPOBJ)/franken
OBJ+=$(OBJDIR)/makecontext.o $(OBJDIR)/context.o
INCLUDES=-I. -I./libucontext-ios-arm64/include
CPPFLAGS+=$(INCLUDES)
CFLAGS+=-fno-stack-protector -Wno-nullability-completeness

default:			$(OBJ)

$(OBJDIR)/makecontext.o:	libucontext-ios-arm64/makecontext.c
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -o $@ $^

$(OBJDIR)/context.o	:	libucontext-ios-arm64/context.S
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -o $@ $^

.PHONY:				clean
clean:
				rm -f $(OBJ)
