; *******************************************************
; Mollen-OS Stage 2 Bootloader
; Copyright 2015 (C)
; Author: Philip Meulengracht
; Version 1.0
; *******************************************************
; Gdt.inc
; - Contains routines for setting up GDT
; 

%ifndef _MBOOT_GDT_INC_
%define _MBOOT_GDT_INC_

BITS 16

; Definitions
%define 		NULL_DESC 		0
%define 		CODE_DESC 		0x8
%define 		DATA_DESC 		0x10
%define 		DATA16_DESC 	0x18


; ********************************
; Load16BitIdt
; *******************************
LoadIdt16:
	lidt [Idt16]
	ret

; ********************************
; InstallGdt
; ********************************
InstallGdt:
	; Save state
	pushad

	; Clear interrupts
	cli 

	; Load Gdt
	lgdt 	[TemporaryGdt]

	; Restore & Return
	sti
	popad
	ret

;*******************************************
; Global Descriptor Table
;*******************************************
StartOfGdt:
	dd 0                ; null descriptor
	dd 0 

; gdt code:	            ; code descriptor
	dw 0FFFFh           ; limit low
	dw 0                ; base low
	db 0                ; base middle
	db 10011010b        ; access
	db 11001111b        ; granularity
	db 0                ; base high

; gdt data:	            ; data descriptor
	dw 0FFFFh           ; limit low (Same as code)10:56 AM 7/8/2007
	dw 0                ; base low
	db 0                ; base middle
	db 10010010b        ; access
	db 11001111b        ; granularity
	db 0                ; base high

; gdt data 16bit:       ; data descriptor
	dw 0FFFFh           ; limit low (Same as code)10:56 AM 7/8/2007
	dw 0                ; base low
	db 0                ; base middle
	db 10010010b        ; access
	db 00001111b        ; granularity
	db 0                ; base high
EndOfGdt:

; The actual Gdt Header
TemporaryGdt: 
	dw EndOfGdt - StartOfGdt - 1
	dd StartOfGdt


; Interrupt Descriptor Table
Idt16:
	dw 0x3ff		; 256 entries, 4b each = 1K
	dd 0			; Real Mode IVT @ 0x0000

%endif