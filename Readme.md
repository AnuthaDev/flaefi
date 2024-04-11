# flaEFI
Flappy bird clone for UEFI written in C11.

![image](repo_assets/game.png)


## Building
Clone the repo along with its submodules.

`cd` into and `make` UEFI-GPT-image-creator. It is used for generating GPT image for QEMU

For generating the bootable EFI executable, enter

`make`

While inside the repo and it should generate a file called `BOOTX64.EFI`

To test it on a PC, put the `BOOTX64.EFI` and `sprites.bmp` inside `EFI/BOOT` directory of a FAT32 partitioned USB drive.


## Demo
Here is a demo of flaEFI running on a UEFI laptop

[![flaEFI running on laptop](https://img.youtube.com/vi/QMowro83PfE/0.jpg)](https://www.youtube.com/watch?v=QMowro83PfE)