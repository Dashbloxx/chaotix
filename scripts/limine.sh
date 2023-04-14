#!/bin/bash

# If anyone can submit a pull request allowing us to build limine for a specific architecture provided as the first argument, that would be great...

git clone https://github.com/limine-bootloader/limine .limine
cd .limine
./bootstrap
./configure
make