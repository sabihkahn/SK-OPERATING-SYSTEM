# 1. Assemble the boot stub into an ELF32 object file
nasm -f elf32 boot.asm -o boot.o

# 2. Compile the C kernel for a 32-bit freestanding target
gcc -m32 -ffreestanding -c kernel.c -o kernel.o

# 3. Link everything into the final kernel binary using your linker script
ld -m elf_i386 -T linker.ld -o mykernel.bin boot.o kernel.o

# 4. Move the binary into your ISO tree and build the image
cp mykernel.bin iso_root/boot/mykernel.bin

grub-mkrescue -o skos.iso iso_root/