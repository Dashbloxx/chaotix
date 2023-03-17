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
Building psychix is easy. First of all, you'll need a `i686-elf` toolchain (if you don't have the toolchain, use psychix's toolchain script which builds it for you). After that, simply run `make`. That builds the entire operating system (the kernel, the libc, and the userland binaries).
## Testing & exporting
After building psychix, you can test it and/or export it as a CD-ROM image.
### Testing
To test psychix, simply run:
```
make run
```
To that all of the features of psychix work, simply run:
```
make test
```
### Exporting
To export psychix, simply run:
```
make cdrom.iso
```
This will combine the psychix kernel, it's initrd, and the GRUB bootloader into a bootable CD-ROM image (a `.iso` file). You can test it on real hardware by flashing that CD-ROM image to a USB drive using [balena etcher](https://www.balena.io/etcher) or [rufus](https://rufus.ie/en/).
Psychix supports multiboot, so you can also combine psychix's kernel & initrd with any other bootloader that supports multiboot, but that isn't to be documented right now.
## License
Psychix is licensed under the MIT license. See the file `LICENSE` for more information.