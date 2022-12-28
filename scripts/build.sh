#!/bin/bash

(cd kernel;./build.sh)
(cd userland;./build.sh)
./initrd.sh