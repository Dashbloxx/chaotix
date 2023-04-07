# GDT
## Global Descriptor Table
The global descriptor table is a data structure that you can feed to the CPU that describes different segments in memory to the CPU. It can allow a kernel to set up memory protection, especially when it comes to seperating the kernel from userland (programs).