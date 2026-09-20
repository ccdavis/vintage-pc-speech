/* Real-mode stub for ESPKD, copied into a DOS memory block (see espkd.c rm_* offsets).
 * Assembled with the DJGPP as in 16-bit mode; all data references are absolute offsets
 * inside the block (no relocations), so `objcopy -O binary` of the object is the image.
 *
 *   0x00..0x3F  variables (patched from protected mode with _farpoke)
 *   0x40  INT 14h entry: answers status/init/receive for our port in real mode,
 *         far-jumps to the DPMI real-mode callback for AH=1 (send byte), chains
 *         other ports to the old vector.
 *   0x80  IRQ 8 (INT 70h) entry for /SPK: one PWM sample per RTC tick from the ring
 *         at 0x100, RTC + PIC acknowledge, and every `period` ticks a far call into
 *         the wake callback that resumes the synthesizer in protected mode.
 *   0xE0  rmwait: spin in real mode for CX BIOS ticks (/TEST: interrupts arriving in
 *         real mode while protected-mode handlers are installed).
 *   0x100 PWM sample ring, 8192 bytes (PIT channel 2 counts, 16-bit head/tail). */
        .code16
        .text
        .org 0x00
        .word 2                 /* 0x00 COM index (0 = COM1) */
        .long 0                 /* 0x02 old INT 14h vector (off:seg) */
        .long 0                 /* 0x06 real-mode callback for AH=1 */
        .long 0                 /* 0x0A real-mode callback: wake the synthesizer */
        .word 0                 /* 0x0E spk ring head (producer, protected mode) */
        .word 0                 /* 0x10 spk ring tail (consumer, this stub / PM IRQ 8) */
        .word 512               /* 0x12 wake countdown */
        .word 512               /* 0x14 wake period (ticks) */
        .byte 73                /* 0x16 PWM count for silence */
        .byte 0                 /* 0x17 flags: bit 0 = send every INT 14h function to protected mode */
        .long 0                 /* 0x18 INT 14h calls answered here */
        .long 0                 /* 0x1C IRQ 8 ticks handled here */
        .long 0                 /* 0x20 wake calls from here */
        .ascii "ESPKD1"         /* 0x24 signature */
        .word 0                 /* 0x2A */

        .org 0x40
int14:
        cmpw    %cs:0x00, %dx
        jne     chain
        cmpb    $1, %ah
        je      put
        testb   $1, %cs:0x17
        jnz     put
        cmpb    $2, %ah
        je      rx
        movw    $0x60B0, %ax    /* THRE+TEMT; CTS+DSR+DCD */
        incl    %cs:0x18
        iret
rx:     movb    $0x80, %ah      /* timeout: nothing to receive */
        incl    %cs:0x18
        iret
put:    ljmpw   *%cs:0x06       /* the callback's iret returns to the INT 14h caller */
chain:  ljmpw   *%cs:0x02

        .org 0x80
irq8:
        pushw   %ax
        pushw   %bx
        movw    %cs:0x10, %bx
        cmpw    %cs:0x0E, %bx
        je      idle
        incw    %cs:0x10
        andw    $0x1FFF, %bx
        movb    %cs:0x100(%bx), %al
        jmp     out
idle:   movb    %cs:0x16, %al
out:    outb    %al, $0x42      /* reload PIT channel 2 (mode 0): pulse width = sample */
        movb    $0x0C, %al
        outb    %al, $0x70
        inb     $0x71, %al      /* RTC register C: acknowledge the periodic interrupt */
        movb    $0x20, %al
        outb    %al, $0xA0
        outb    %al, $0x20
        incl    %cs:0x1C
        decw    %cs:0x12
        jnz     done
        movw    %cs:0x14, %ax
        movw    %ax, %cs:0x12
        cmpl    $0, %cs:0x0A
        je      done
        incl    %cs:0x20
        pushfw
        lcallw  *%cs:0x0A       /* wake: resumes the synthesizer in protected mode, iret back */
done:   popw    %bx
        popw    %ax
        iret

        .org 0xE0
rmwait:
        sti
        pushw   %ds
        xorw    %ax, %ax
        movw    %ax, %ds
        movw    (0x46C), %bx
1:      movw    (0x46C), %ax
        subw    %bx, %ax
        cmpw    %cx, %ax
        jb      1b
        popw    %ds
        lret

        .org 0x100
