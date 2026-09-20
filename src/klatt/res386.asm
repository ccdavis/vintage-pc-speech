; 386 assembly resonator and antiresonator for the fixed-point Klatt engine (16-bit real mode, near calls).
; kfx_res_t is five packed int32: a, b, c, p1, p2 (Watcom -zp1 default for 16-bit: no padding).
; C prototypes with explicit registers (see klatt_fx.c):  r in BX (DS-relative), x in DX:AX, y returned in DX:AX.
        .386
_TEXT   segment word public use16 'CODE'
        assume  cs:_TEXT
        public  kfx_res386_, kfx_ares386_

; y = (a*x + b*p1 + c*p2 + 2048) >> 12;  p2 = p1;  p1 = y
kfx_res386_ proc near
        shl     edx, 16
        mov     dx, ax                  ; edx = x
        mov     eax, [bx]               ; a
        imul    eax, edx                ; a*x
        mov     ecx, [bx+4]             ; b
        imul    ecx, dword ptr [bx+12]  ; b*p1
        add     eax, ecx
        mov     ecx, [bx+8]             ; c
        imul    ecx, dword ptr [bx+16]  ; c*p2
        add     eax, ecx
        add     eax, 2048
        sar     eax, 12
        mov     ecx, [bx+12]
        mov     [bx+16], ecx            ; p2 = p1
        mov     [bx+12], eax            ; p1 = y
        mov     edx, eax
        shr     edx, 16                 ; dx:ax = y
        ret
kfx_res386_ endp

; y = (a*x + b*p1 + c*p2 + 512) >> 10;  p2 = p1;  p1 = x
kfx_ares386_ proc near
        shl     edx, 16
        mov     dx, ax                  ; edx = x
        mov     eax, [bx]
        imul    eax, edx
        mov     ecx, [bx+4]
        imul    ecx, dword ptr [bx+12]
        add     eax, ecx
        mov     ecx, [bx+8]
        imul    ecx, dword ptr [bx+16]
        add     eax, ecx
        add     eax, 512
        sar     eax, 10
        mov     ecx, [bx+12]
        mov     [bx+16], ecx            ; p2 = p1
        mov     [bx+12], edx            ; p1 = x
        mov     edx, eax
        shr     edx, 16
        ret
kfx_ares386_ endp
_TEXT   ends
        end
