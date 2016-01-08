COMPILE.c?=$(CC) -c $(CPPFLAGS) $(CFLAGS)
COMPILE.S?=$(CC) -c $(CPPFLAGS) $(ASFLAGS)
RUMPOBJ?=../../rumpobj
OBJDIR=$(RUMPOBJ)/franken
OBJ=$(OBJDIR)/virtio.o
OBJ+=$(OBJDIR)/virtio_blk.o
OBJ+=$(OBJDIR)/fs.o
OBJ+=$(OBJDIR)/virtio_net.o
RUMP?=$(RUMPOBJ)/rump
INCLUDES=-I../include -I../../platform/include -I. -I$(RUMP)/include -I../../librumpuser/
INCLUDES+=-I../thread/ -I../ucontext/ -I../../platform/linux -I../init
LKL_INCLUDES=-I../../lkl-linux/include/uapi -I../../lkl-linux/include
LKL_INCLUDES+=-I../../lkl-linux/arch/lkl/include/ -I../../lkl-linux/arch/lkl/include/uapi
LKL_INCLUDES+=-I../../lkl-linux/arch/lkl/include/generated -I../../lkl-linux/arch/lkl/include/generated/uapi
INCLUDES+=$(LKL_INCLUDES)
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

$(OBJDIR)/virtio_net.o:		virtio_net.c $(DEPS)
				@mkdir -p $(OBJDIR)
				$(COMPILE.c) -o $@ virtio_net.c

.PHONY:				clean
clean:		
				rm -f $(OBJ)
