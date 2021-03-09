# Sandal - A lightweight bootloader for linux systems.

### Features:
    * Written in Assembly and C++ (although no functions from std lib are used)
    * Capable to boot modern Linux distributions (tested with Ubuntu 20.04 using qemu)
    * Two staged loader with small footprint (stage 0: 512 bytes, stage 1: ~5600 bytes with -Os)
    * ext2 filesystem support: Can locate and load linux kernel file from filesystem
    * Terminal output for information and debugging

### Limitations and future improvements:
    * Path to kernel is hardcoded in binary --> create a config file in filesystem to be more flexible avoid the need to flash the bootloader every time the kernel file changes
    * Disk I/O depends on BIOS functionality and only Disks with 2 TiB are supported; Only ext2 files up to 4 GiB are supported but luckily you rarely see kernels > 4 GiB ;)
    * Boot protocol has no initrd supported yet
    * Add chain loading support
    * Only MBR-formatted disks supported; add GPT functionality with stage 1 located at own partition

