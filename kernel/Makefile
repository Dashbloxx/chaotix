CFLAGS += \
	-fno-omit-frame-pointer \
	-isystem..

TARGET := kernel

OBJS := \
	ac97.o \
	boot.o \
	cmdline.o \
	console/fb_console.o \
	console/psf.o \
	console/serial_console.o \
	console/system_console.o \
	console/tty.o \
	fs/dentry.o \
	fs/fifo.o \
	fs/fs.o \
	fs/initrd.o \
	fs/procfs/pid.o \
	fs/procfs/procfs.o \
	fs/procfs/root.o \
	fs/tmpfs.o \
	fs/vfs.o \
	gdt.o \
	graphics/bochs.o \
	graphics/fb.o \
	graphics/multiboot.o \
	growable_buf.o \
	hid/keyboard.o \
	hid/mouse.o \
	hid/ps2.o \
	idt.o \
	interrupt.o \
	irq.o \
	kprintf.o \
	lock.o \
	main.o \
	memory/kmalloc.o \
	memory/page_allocator.o \
	memory/paging.o \
	memory/range_allocator.o \
	pci.o \
	pit.o \
	process.o \
	pseudo_device.o \
	ring_buf.o \
	scheduler.o \
	serial.o \
	syscall/clock.o \
	syscall/exec.o \
	syscall/fs.o \
	syscall/mmap.o \
	syscall/process.o \
	syscall/socket.o \
	syscall/syscall.o \
	system.o \
	time.o \
	unix_socket.o \
	../common/libgen.o \
	../common/math.o \
	../common/string.o \
	../common/strings.o \
 	../common/stdio.o \
 	../common/stdlib.o

DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) linker.ld
	$(CC) $(CFLAGS) -Wl,-Tlinker.ld -Wl,--build-id=none -Wl,-z,noexecstack -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

.S.o:
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)
