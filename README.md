# Psychix
## About
Psychix is a UNIX-like operating system written in C that uses code from [Xv6](https://github.com/mit-pdos/xv6-public) and [Yagura](https://github.com/mosmeh/yagura).
## Supported architectures
- [X] [`i?86`](https://en.wikipedia.org/wiki/X86)
- [ ] [`amd64`](https://en.wikipedia.org/wiki/X86-64)
- [ ] [`ARM`](https://en.wikipedia.org/wiki/ARM_architecture_family)
- [ ] [`RISC-V`](https://riscv.org/)
- [ ] [`PowerPC`](https://en.wikipedia.org/wiki/PowerPC)
- [ ] [`68K`](https://en.wikipedia.org/wiki/Motorola_68000_series)

Although psychix cannot switch to [long mode](https://wiki.osdev.org/Setting_Up_Long_Mode) (64-bit mode) on an `amd64` machine, it can still run in protected mode like it usually does.
## Building
Building psychix is easy. First of all, you'll need a `i686-elf` toolchain (if you don't have the toolchain, use psychix's toolchain script which builds it for you). After that, simply run `make`. That builds the entire operating system (the kernel, the libc, and the userland binaries). You can then test psychix with the `run.sh` script.
## License
Psychix is licensed under the MIT license. See the file `LICENSE` for more information.