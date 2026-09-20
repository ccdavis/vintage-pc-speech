; int32_t kfx_imul32(int32_t x, int32_t y): low 32 bits of x*y, one 386 IMUL instead of Watcom's __I4M loop.
; Watcom 16-bit register convention: x in DX:AX, y in CX:BX, result in DX:AX.
        .386
_TEXT   segment word public use16 'CODE'
        assume cs:_TEXT
        public  kfx_imul32_
kfx_imul32_ proc near
        shl     edx, 16
        mov     dx, ax
        shl     ecx, 16
        mov     cx, bx
        imul    edx, ecx
        mov     eax, edx
        shr     edx, 16
        ret
kfx_imul32_ endp
_TEXT   ends
        end
