#!/bin/bash

rm -r base/bin/* &>/dev/null
rm -r kernel/kernel &>/dev/null
rm -r kernel/*.o &>/dev/null
rm -r userland/*.o &>/dev/null
rm initrd &>/dev/null
rm psychix.iso &>/dev/null