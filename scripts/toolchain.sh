export PREFIX="/opt/i686-elf-cross"
export TARGET=i686-elf
mkdir .i686-elf
cd .i686-elf
git clone https://sourceware.org/git/binutils-gdb.git binutils
mkdir build-binutils
cd build-binutils
../binutils/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
sudo make install
cd ..
git clone https://gcc.gnu.org/git/gcc.git gcc
mkdir build-gcc
cd build-gcc
../gcc/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
sudo make install-gcc
sudo make install-target-libgcc
cd ..
cd ..