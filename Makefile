MAKEFLAGS += --jobs=$(shell nproc)
SUBDIRS := kernel userland tools

export CFLAGS := \
	-std=c11 \
	-m32 \
	-static \
	-nostdlib -ffreestanding \
	-U_FORTIFY_SOURCE \
	-Wall -Wextra -pedantic \
	-O2 -g

.PHONY: all run clean $(SUBDIRS) base

all: config kernel initrd

run: config kernel initrd
	scripts/run.sh

config:
	scripts/config.sh

initrd: base
	find $< -mindepth 1 ! -name '.gitkeep' -printf "%P\n" | sort | cpio -oc -D $< -F $@

base: $@/* userland
	$(RM) -r $@/root/src
	-git clone . $@/root/src
	$(RM) -r $@/root/src/.git

$(SUBDIRS):
	$(MAKE) -C $@ all

cdrom.iso: kernel initrd
	cp kernel/kernel initrd disk/boot
	grub-mkrescue -o '$@' disk

clean:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir $@; done
	$(RM) -r base/root/src
	$(RM) initrd disk_image disk/boot/kernel disk/boot/initrd

runsh: kernel initrd
	scripts/run.sh shell

shell: kernel initrd
	scripts/run.sh shell

test: kernel initrd
	scripts/run_tests.sh

tool:
	make -C tools

toolchain:
	scripts/toolchain.sh $(CROSS_TYPE)