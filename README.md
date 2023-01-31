# Psychix
<details open>
<summary>What is Psychix?</summary>
<br>

Psychix is a UNIX-like operating system that uses some code from Xv6 and yagura. It is designed to
</details>
<details open>
<summary>How can I build Psychix?</summary>
<br>

To build Psychix, you will need to install some packages.
* GRUB (grub2 on Ubuntu)
* GCC (gcc on Ubuntu)
* xorriso (xorriso on Ubuntu)
* qemu (qemu & qemu-system-x86 on Ubuntu)
* cpio
Support of clang will soon be added aswell, although currently I am planning on using cross compilers instead of native GCC...
</details>
<details open>
<summary>Checklist</summary>
<br>

Architecture support:
- [ ] RISC-V architecture support
- [X] ix86 architecture support
- [ ] ARMvx architecture support
- [ ] ARM64 architecture support
- [ ] AMD64 architecture support
Development-related:
- [X] libc
- [X] A dedicated developer
- [X] Psychix can build itself
Contribution-related:
- [X] Our first contribution!
- [ ] Our 10th contribution!
Software:
- [X] Basic command-line tools including `clear`, `rm`, `mkdir`, `cat`, etc.
- [ ] GCC port
- [ ] A window system
Operating system features:
- [ ] Access to the internet (Networking stack)
- [X] Graphics (framebuffer) 
</details>
<details open>
<summary>License</summary>
<br>

See the license at LICENSE. If there are any ports, the license does not apply to them, rather the original software's license does.
</details>