![](.assets/logo.png)

# Psychix
## What is Psychix?
Psychix is an open-source implementation of UNIX. It borrows code from Xv6, and Yagura. Psychix will be in heavy development to make it something useful and more powerful than other UNIX-like operating systems.
## How to build Psychix?
Simply run `make`. That basically builds Psychix, and then you can test it by running:
```
./run.sh
```
We're basically assuming that you're running an operating system that runs using the Linux kernel, or an operating system that works nearly like UNIX, aswell as on an architecture that is compatible with the i386 architecture...
# Plans & checklist
- [X] Unix sockets
- [X] Serial driver
- [ ] Psychix can build itself
- [X] Schelduler
- [X] Process system
- [X] Graphics
- [X] Device filesystem
- [X] Initrd, with good & organized filesystem
- [X] Libc
- [X] x86 support
- [ ] ARM support
- [X] Basic command-line tools including `ls`, `mkdir`, `rm`, `rmdir`, `cat`, `grep`, etc
- [ ] Window system
- [ ] Lua ported
- [ ] GCC ported
- [X] Dedicated developer
- [X] A good license
- [X] A good readme
- [ ] Exact documentation
# License
Although I could rewrite all of the code and change the license, I decided to stick with the license (MIT license), just because its a good and permissive license. See the license at LICENSE. If there are any ports, the license does not apply to them, rather the original software's license does.
