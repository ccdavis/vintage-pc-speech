     .286
     %TITLE 'Text To Speech Device Driver'
     %PAGESIZE 64,132
     %TRUNC
;--------------------------------------------------------------------
;
;   Device driver module for english voice talking program
;   This device driver will only work for an AT-style computer
;   hence the code is tailored for 80286 assembly code
;
;   This code was written by Jonathan Hornstein
;                            Yarra House Software P/L
;                            Melbourne, Australia
;
;--------------------------------------------------------------------
_TEXT	segment byte public 'CODE'
_TEXT	ends
_DATA	segment word public 'DATA'
_DATA	ends
_BSS	segment word public 'BSS'
_BSS	ends
ENDSEG  segment word public 'ENDSEG'
ENDSEG	ends

DGROUP	group	_TEXT,_DATA,_BSS,ENDSEG
ASSUME 	cs:DGROUP,ds:DGROUP, es:NOTHING, ss:NOTHING

STACKSIZ equ 200 ; Size of the programming stack in words for ExecCommand code
extrn   _ExecEnglishCommand:near
extrn   _ExecPhonemeCommand:near
_TEXT	segment
        org 0
        PUBLIC  DGROUP@
;--------------------------------------------------------------------
;	driver header
;--------------------------------------------------------------------
   dd   .+18                   ; Link
   dw   08000h                 ; Attribute
   dw   @@DevStrategy          ; Strategy
   dw   @@DevEnglishInterrupt  ; Interrupt
   db   '$TALK$  '             ; Name

   dd   -1                     ; Link
   dw   08000h                 ; Attribute
   dw   @@DevStrategy          ; Strategy
   dw   @@DevPhonemeInterrupt  ; Interrupt
   db   '$PHONEM$'             ; Name

pReqHdr      dd ?
pCallerStack dd ?
DGROUP@      dw ?   ; Required for the C compiler in tiny model!
;--------------------------------------------------------------------
;
; Device Driver Strategy function
;
;--------------------------------------------------------------------
@@DevStrategy proc far
    mov word ptr cs:pReqHdr+2, es
    mov word ptr cs:pReqHdr, bx
    ret
@@DevStrategy endp
;--------------------------------------------------------------------
;
; Device Driver Interrupt  function
;
;--------------------------------------------------------------------
@@DevEnglishInterrupt proc far
    push ax
    push ds
    push es
    pushf
    mov  word ptr cs:pCallerStack+2,ss  ; Save the old stack pointer
    mov  word ptr cs:pCallerStack,sp
    cli                                 ; Stop Interrupts
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov DGROUP@,ax                      ; required for the C compiler in tiny model!
    mov ss,ax
    mov sp,offset cs:StackTop           ; Set up the stack pointer
    sti                                 ; Enable interrupts
    pusha                               ; Save all registers
    push word ptr cs:pReqHdr+2
    push word ptr cs:pReqHdr
    call _ExecEnglishCommand
    add  sp,4
    popa                                ; Restore registers
    cli                                 ; Stop Interrupts
    mov  ss,word ptr cs:pCallerStack+2
    mov  sp,word ptr cs:pCallerStack
    sti                                 ; Enable interrupts
    popf
    pop  es
    pop  ds
    pop  ax
    ret
@@DevEnglishInterrupt endp
;
@@DevPhonemeInterrupt proc far
    push ax
    push ds
    push es
    pushf
    mov  offset word ptr pCallerStack+2,ss  ; Save the old stack pointer
    mov  offset word ptr pCallerStack,sp
    cli                                 ; Stop Interrupts
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov cs:DGROUP@,ax                   ; required for the C compiler in tiny model!
    mov ss,ax
    mov sp,offset cs:StackTop           ; Set up the stack pointer
    sti                                 ; Enable interrupts
    pusha                               ; Save all registers
    push word ptr cs:pReqHdr+2
    push word ptr cs:pReqHdr
    call _ExecPhonemeCommand
    add  sp,4
    popa                                ; Restore registers
    cli                                 ; Stop Interrupts
    mov  ss,offset word ptr pCallerStack+2
    mov  sp,offset word ptr pCallerStack
    sti                                 ; Enable interrupts
    popf
    pop  es
    pop  ds
    pop  ax
    ret
@@DevPhonemeInterrupt endp
_TEXT ends

_DATA segment
         dw STACKSIZ dup(?)
StackTop dw ?
_DATA ends

ENDSEG segment
__TheEnd label byte
public __TheEnd
ENDSEG ends
       end
