; Multiboot standard constants
MODULEALIGN equ  1 << 0             ; Align loaded modules on page boundaries
MEMINFO     equ  1 << 1             ; Provide a memory map
FLAGS       equ  MODULEALIGN | MEMINFO 
MAGIC       equ  0x1BADB002         ; "Magic number" lets GRUB find the header
CHECKSUM    equ -(MAGIC + FLAGS)    ; Checksum to prove we are Multiboot

; Define the Multiboot section for the linker
section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

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

    ; Call our C kernel main function
    extern kernel_main
    call kernel_main

    ; If the kernel accidentally returns, clear interrupts and halt the CPU
.halt:
    cli
    hlt
    jmp .halt
