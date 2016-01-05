COMPILE.c?=$(CC) -c $(CPPFLAGS) $(CFLAGS)
COMPILE.S?=$(CC) -c $(CPPFLAGS) $(ASFLAGS)
RUMPOBJ?=../../rumpobj
OBJDIR=$(RUMPOBJ)/franken
OBJ=$(OBJDIR)/virtio.o
OBJ+=$(OBJDIR)/virtio_blk.o
OBJ+=$(OBJDIR)/fs.o
RUMP?=$(RUMPOBJ)/rump
INCLUDES=-I../include -I../../platform/include -I. -I$(RUMP)/include -I../../librumpuser/
CPPFLAGS+=-nostdinc $(INCLUDES)
CFLAGS+=-fno-stack-protector
DEPS=

default:			$(OBJ)

$(OBJDIR)/virtio.o:		virtio.c $(DEPS)
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -o $@ virtio.c

$(OBJDIR)/virtio_blk.o:		virtio_blk.c $(DEPS)
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -o $@ virtio_blk.c

$(OBJDIR)/fs.o:			fs.c $(DEPS)
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -o $@ fs.c

.PHONY:				clean
clean:		
				rm -f $(OBJ)
