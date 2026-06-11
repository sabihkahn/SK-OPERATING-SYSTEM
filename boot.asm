; Multiboot standard constants
MODULEALIGN equ  1 << 0             ; Align loaded modules on page boundaries
MEMINFO     equ  1 << 1             ; Provide a memory map
VIDEO_MODE  equ  1 << 2             ; CRITICAL: Tell GRUB to initialize a video mode

FLAGS       equ  MODULEALIGN | MEMINFO | VIDEO_MODE 
MAGIC       equ  0x1BADB002         ; "Magic number" lets GRUB find the header
CHECKSUM    equ -(MAGIC + FLAGS)    ; Checksum to prove we are Multiboot

; Define the Multiboot section for the linker
section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

    ; --- MULTIBOOT VIDEO MODE TABLE EXTENSION ---
    ; These values are required when the VIDEO_MODE flag (Bit 2) is set.
    dd 0        ; Mode type: 0 = Linear Framebuffer (highly recommended for modern HW)
    dd 1024     ; Preferred Width (Pixels)
    dd 768      ; Preferred Height (Pixels)
    dd 32       ; Preferred Bits Per Pixel (Depth)

; Set up a stack area so our C code has a place to store variables
section .bss
align 16
stack_bottom:
    resb 16384                      ; Reserve 16 KB for the kernel stack
stack_top:

; Executive code section
section .text
global _start
_start:
    ; Set up the stack pointer register (ESP)
    mov esp, stack_top

    ; CRITICAL STEP: GRUB passes a pointer to the Multiboot Information Structure in EBX.
    ; You must push this to the stack so your C kernel can read the screen's memory address!
    push ebx

    ; Call our C kernel main function
    extern kernel_main
    call kernel_main

    ; If the kernel accidentally returns, clear interrupts and halt the CPU
.halt:
    cli
    hlt
    jmp .halt