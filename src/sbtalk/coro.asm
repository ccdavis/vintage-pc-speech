; co_switch: save SS:SP (after pushing callee-saved regs) into *save, load new SS:SP, pop and return.
; void co_switch(struct ctx *save /*ax*/, unsigned new_ss /*dx*/, unsigned new_sp /*bx*/);
        .model small
        .code
        public  _co_switch
_co_switch proc near
        push    bp
        push    si
        push    di
        mov     si, ax
        mov     [si], sp
        mov     [si+2], ss
        cli
        mov     ss, dx
        mov     sp, bx
        sti
        pop     di
        pop     si
        pop     bp
        ret
_co_switch endp

; int cpu_class(void): 0 = 8086/88, 2 = 286, 3 = 386+  (FLAGS bits 12-15 behaviour in real mode)
        public  _cpu_class
_cpu_class proc near
        pushf
        pop     ax
        and     ax, 0fffh
        push    ax
        popf
        pushf
        pop     ax
        and     ax, 0f000h
        cmp     ax, 0f000h
        jne     not86
        mov     ax, 0
        ret
not86:  pushf
        pop     ax
        or      ax, 0f000h
        push    ax
        popf
        pushf
        pop     ax
        and     ax, 0f000h
        jnz     is386
        mov     ax, 2
        ret
is386:  mov     ax, 3
        ret
_cpu_class endp

; void spk_bit8(unsigned char b /*al*/, unsigned delay /*dx*/, unsigned onoff /*bx: bl=on, bh=off*/)
; 8 bits MSB first out of port 61h, 'delay' iterations of LOOP between bits (calibrated by the caller).
        public  _spk_bit8
_spk_bit8 proc near
        push    cx
        push    si
        mov     si, 8
        mov     ah, al
nextbit:
        shl     ah, 1
        mov     al, bh
        jnc     outit
        mov     al, bl
outit:  out     61h, al
        mov     cx, dx
dly:    loop    dly
        dec     si
        jnz     nextbit
        pop     si
        pop     cx
        ret
_spk_bit8 endp

; void cpu_loops(unsigned n /*cx*/): n iterations of LOOP, the same delay primitive spk_bit8 uses
        public  _cpu_loops
_cpu_loops proc near
lp:     loop    lp
        ret
_cpu_loops endp
        end
