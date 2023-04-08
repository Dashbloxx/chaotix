# Chapter 2
## Character devices
Character devices are a great way to represent hardware to the operating system. Character devices are files that can be found in the `/dev` directory. These files don't store anything, rather when you read or write to them, you're reading or writing to the hardware that they represent. For example, the file `/dev/dsp` represents the audio device. When you write to it, it will write to speaker. You can also read from devices. For example, there is a character device that represents the virtual random number generator device (the device isn't actual hardware). We can read from the random number generator device (`/dev/random`), and write to the audio device (`/dev/dsp`):
```sh
cat /dev/random > /dev/dsp
```
Character devices also allow userland programs to communicate with hardware. For example, if a program wants to draw graphics to the screen, then it should write to the framebuffer device (`/dev/fb0`).