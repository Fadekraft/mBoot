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
EndOfGdt:

; The actual Gdt Header
TemporaryGdt: 
	dw EndOfGdt - StartOfGdt - 1
	dd StartOfGdt

%endif