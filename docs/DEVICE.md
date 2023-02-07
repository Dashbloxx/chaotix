# Device filesystem
## What is the Device filesystem?
The device filesystem, also known as devfs, is a folder in the filesystem, which contains different files which represent different hardware/kernel features. When you write something to a device file, you are writing data to the hardware/kernel feature that it represents. For example:
```echo hello > /dev/tty``` will print `hello` to the terminal.
You can also read from certain files in the device filesystem.
