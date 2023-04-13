# Toolchain
## About
This page will explain how to build a cross-toolchain, which is required if we want to build chaotix.
## How to build a cross-toolchain
Let's say that you want to build chaotix for the i686 architecture. Therefore, we will need to make a toolchain that can build for i686. This page will explain how to compile a cross-toolchain.
The chaotix operating system's repo comes with a script called `toolchain.sh`, found in the `scripts` subdirectory of the project. You must provide it one argument; the architecture that the cross-toolchain should cross-compile for. Here's an example:
```sh
scripts/toolchain.sh i686-elf
```
Note that the chaotix operating system only supports the `i686` architecture for now, so therefore we will just need a `i686-elf` cross-toolchain.
After the entire process of building the cross-toolchain, you will want to add the toolchain's `bin` path to the PATH environment variable:
```sh
export PATH="$HOME/Toolchains/i686-elf-cross:$PATH"
```
If you want this to be added to PATH automatically, append the line above to `~/.bashrc`...