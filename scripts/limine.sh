#!/bin/bash

# Use this script to fetch and compile limine. Here are some examples of this script being used:
#   scripts/limine.sh i686
#   scripts/limine.sh arm64
# We shall NEVER build limine for x86_64. If you want chaotix running in x86_64 with limine, just build limine for i686. Chaotix will set up x86_64.

while true; do
    printf "\033[1;31mLimine bootloader is suspicious and unstrusted...\033[0m\n"
    read -p "Would you still like to use limine bootloader? (Y/N)? " yn
    case $yn in
        [Yy]* ) echo "Setting up limine..."; break;;
        [Nn]* ) echo "Cancelled..."; exit;;
        * ) echo "Please answer Y or N.";;
    esac
done

git clone https://github.com/limine-bootloader/limine .limine
cd .limine
./bootstrap
./configure
make TARGET_ARCH=$1