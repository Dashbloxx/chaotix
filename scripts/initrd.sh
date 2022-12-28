#!/bin/bash

find base -mindepth 1 ! -name '.gitkeep' -printf "%P\n" | sort | cpio -oc -D base -F initrd