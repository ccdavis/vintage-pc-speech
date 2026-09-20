/* co_switch for DJGPP (32-bit flat): save callee-saved registers and SS:ESP into *save,
 * load the new SS:ESP, pop the other side's registers and return there.  The 32-bit
 * twin of sbtalk/coro.asm.  SS is part of the context because a protected-mode hardware
 * interrupt handler runs on the DPMI host's locked stack (SS = the host's selector).
 * void co_switch(struct ctx *save, unsigned new_ss, unsigned new_esp);   ctx = {esp, ss}
 * The interrupt flag is left alone (mov ss inhibits interrupts for one instruction). */
        .text
        .globl  _co_switch
_co_switch:
        pushl   %ebp
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        movl    20(%esp), %eax          /* save */
        movl    %esp, (%eax)
        movl    %ss, %ecx
        movl    %ecx, 4(%eax)
        movl    24(%esp), %ecx          /* new ss */
        movl    28(%esp), %edx          /* new esp */
        movw    %cx, %ss
        movl    %edx, %esp
        popl    %edi
        popl    %esi
        popl    %ebx
        popl    %ebp
        ret

/* Minimal hardware-interrupt wrappers.  DJGPP's _go32_dpmi_allocate_iret_wrapper switches to a
 * private stack and drops any nested invocation while its in-use flag is set; our handlers
 * resume the synthesizer with interrupts enabled, so the next SB/RTC interrupt is nested by
 * design and must be delivered.  These wrappers only save registers and load the data
 * selector (patched into the immediate at install time through the DS alias of .text). */
        .globl  _isr_sb_wrapper, _isr_sb_sel, _isr_rtc_wrapper, _isr_rtc_sel
_isr_sb_wrapper:
        pushl   %ds
        pushl   %es
        pushl   %fs
        pushl   %gs
        pushal
        .byte   0x66, 0xb8
_isr_sb_sel:
        .word   0
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    %ax, %gs
        call    _sb_isr
        popal
        popl    %gs
        popl    %fs
        popl    %es
        popl    %ds
        iret
_isr_rtc_wrapper:
        pushl   %ds
        pushl   %es
        pushl   %fs
        pushl   %gs
        pushal
        .byte   0x66, 0xb8
_isr_rtc_sel:
        .word   0
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    %ax, %gs
        call    _rtc_isr
        popal
        popl    %gs
        popl    %fs
        popl    %es
        popl    %ds
        iret
