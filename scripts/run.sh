#!/bin/bash

#
#   __  __                      
#  |  \/  |__ _ __ _ _ __  __ _ 
#  | |\/| / _` / _` | '  \/ _` |
#  |_|  |_\__,_\__, |_|_|_\__,_|
#              |___/        
#
# Chaotix is a UNIX-like operating system that consists of a kernel written in C and
# i?86 assembly, and userland binaries written in C.
#    
# Copyright (c) 2023 Nexuss, John Paul Wohlscheid, rilysh, Milton612, and FueledByCocaine
#
# This file may or may not contain code from https://github.com/mosmeh/yagura, and/or
# https://github.com/mit-pdos/xv6-public. Both projects have the same license as this
# project, and the license can be seen below:
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

set -e

KERNEL='kernel/kernel'
INITRD='initrd'

QEMU_DISPLAY_ARGS=(-display "sdl,gl=off,show-cursor=off")
if [ "$1" = "shell" ]; then
	QEMU_DISPLAY_ARGS=(-display none -vga none)
fi

unset QEMU_BINARY_PREFIX
unset QEMU_BINARY_SUFFIX
unset QEMU_VIRT_TECH_ARGS

if command -v wslpath >/dev/null; then
	PATH=${PATH}:/mnt/c/Windows/System32
	QEMU_INSTALL_DIR=$(reg.exe query 'HKLM\Software\QEMU' /v Install_Dir /t REG_SZ | grep '^    Install_Dir' | sed 's/    / /g' | cut -f4- -d' ')
	QEMU_BINARY_PREFIX="$(wslpath -- "${QEMU_INSTALL_DIR}" | tr -d '\r\n')/"
	QEMU_BINARY_SUFFIX='.exe'
	QEMU_VIRT_TECH_ARGS=(-accel "whpx,kernel-irqchip=off" -accel tcg)
	KERNEL=$(wslpath -w "${KERNEL}")
	INITRD=$(wslpath -w "${INITRD}")
fi

QEMU_BIN="${QEMU_BINARY_PREFIX}qemu-system-i386${QEMU_BINARY_SUFFIX}"
"${QEMU_BIN}" -kernel "${KERNEL}" \
	-initrd "${INITRD}" \
	-d guest_errors \
	"${QEMU_DISPLAY_ARGS[@]}" \
	-device ac97 \
	-chardev stdio,mux=on,id=char0 \
	-serial chardev:char0 \
	-mon char0,mode=readline \
	-m 512M \
	"${QEMU_VIRT_TECH_ARGS[@]}"
