#!/bin/bash


if [ "$1" == "x86_64-elf" ]; then
    export PREFIX="$HOME/Toolchains/$1-cross"
    export TARGET=$1
    mkdir -p .$1
    cd .$1
    git clone https://sourceware.org/git/binutils-gdb.git binutils
    mkdir -p build-binutils
    cd build-binutils
    ../binutils/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
    make
    sudo make install
    cd ..
    git clone https://gcc.gnu.org/git/gcc.git gcc
    mkdir -p build-gcc
    cd build-gcc
    ../gcc/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
    make all-gcc
    make all-target-libgcc
    sudo make install-gcc
    sudo make install-target-libgcc
    cd ..
    cd ..
else
    export PREFIX="$HOME/Toolchains/$1-cross"
    export TARGET=$1
    mkdir -p .$1
    cd .$1
    git clone https://sourceware.org/git/binutils-gdb.git binutils
    mkdir -p build-binutils
    cd build-binutils
    ../binutils/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
    make
    sudo make install
    cd ..
    git clone https://gcc.gnu.org/git/gcc.git gcc
    mkdir -p build-gcc
    cd build-gcc
    ../gcc/configure --target=$TARGET --prefix="$PREFIX" --enable-languages=c,c++ --disable-threads --disable-shared --disable-libssp --disable-multilib --with-newlib
    make all-gcc
    sudo make install-gcc
    cd ..
    cd ..
fi