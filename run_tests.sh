#!/bin/bash

set -e

! qemu-system-i386 \
    -kernel kernel/kernel \
    -initrd initrd \
    -append 'panic=poweroff init=/bin/init-test' \
    -d guest_errors \
    -no-reboot \
    -serial stdio \
    -vga none -display none \
    -m 512M \
    2>&1 | tee >(cat 1>&2) | grep -q PANIC
