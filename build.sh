#!/bin/bash

set -euo pipefail

for arg in "$@"; do
  if [ "$arg" = "--clean" ]; then
    echo "Cleaning 'build/' and 'ramfs/'..."
    rm -rf "./build/"
    find "./ramfs/" -type f ! -name "GIT_MARKER" -delete
  fi
done

if [ ! -d "./deps/Limine" ]; then
  echo "Limine not found! Make sure to init submodules."
  exit 1
fi

make -C deps/Limine

mkdir -p ramfs

cmake -B build -D CMAKE_TOOLCHAIN_FILE=x86_64-elf-toolchain.cmake -D CMAKE_BUILD_TYPE=Debug .
cmake --build build --target Kernel

rm -rf iso
mkdir -p iso
cp -r src/config/* iso/

mkdir -p iso/boot/limine
cp deps/Limine/limine-uefi-cd.bin iso/boot/limine/
cp deps/Limine/limine-bios.sys iso/boot/limine/
cp deps/Limine/limine-bios-cd.bin iso/boot/limine/

mkdir -p iso/boot/rootfs
./src/scripts/make_cpio.sh ramfs iso/boot/rootfs/initramfs.cpio

mkdir -p iso/EFI/BOOT
cp deps/Limine/BOOTX64.EFI iso/EFI/BOOT/

mkdir -p ./bin

xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
        -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        iso -o ./bin/BorealOS.iso iso

sync
rm -rf iso

./deps/Limine/limine bios-install ./bin/BorealOS.iso