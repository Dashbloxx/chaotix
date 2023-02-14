# Building a cross toolchain
Psychix will no longer be using the normal gcc compiler. Psychix will start using an elf cross compiler for the targeted architecture. For example, when building for x86, Psychix will use `i386-elf-gcc` instead of `gcc`. This tutorial will show you how to build the different toolchains.
# Building i386-elf binutils
Download the latest binutils `*.tar.gz` package from the GNU mirrors [here](http://ftp.gnu.org/). Then, unzip it:
```sh
tar -xz -f BINUTILS_PACKAGE.tar.gz
```
After that, make a seperate directory and `cd` to it:
```sh
mkdir build-binutils
cd build-binutils
```
Now, let's run the auto configure script:
```sh
../BINUTILS_PACKAGE/configure --prefix=/usr/bin --target=i386-elf -v
```
After all of that works successfully, run the following command:
```sh
sudo gmake all install
```
This will build everything, and install it to `/usr/bin`...
You can now `cd` back to the root directory:
```sh
cd ..
```