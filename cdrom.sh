#!/bin/bash

cp kernel/kernel initrd disk/boot
grub-mkrescue -o psychix.iso disk