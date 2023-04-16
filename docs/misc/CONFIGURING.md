# Configuring
## About
The chaotix operating system has a configuration file called `chaotix.cfg` file of the root directory of the project. By default, it's configured to be built for a i?86 computer, although it can be changed. You can edit the configuration file manually, but this page will teach you how to configure it using chaotix's `configure` tool.
To get this tool, go to the `tools` subdirectory, and run `make`:
```sh
cd tools
make
```
Then, you can use the command to edit the configuration file like this:
```sh
./configure ../chaotix.cfg --arch i?68 --model PC --bootloader GRUB
```
This will configure chaotix to be built for a i?86 PC with the GRUB bootloader. Here are all of the possible options:
```
ARCHITECTURE (arch): i?86, amd64, PowerPC, arm1176jzf-s, cortex-a7, AArch64
FORM (model): PC, RPi
BOOTLOADER (bootloader): GRUB, limine, yaboot, petitboot
```
This will be explained further later...