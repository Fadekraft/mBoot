; *******************************************************
; Mollen-OS Stage 2 Bootloader
; Copyright 2015 (C)
; Author: Philip Meulengracht
; Version 1.0
; *******************************************************
; Common.inc
; - Contains shared routines and shared data
; 

%ifndef _MBOOT_COMMON_INC_
%define _MBOOT_COMMON_INC_

BITS 16

; Definitions
%define 		BLACK		0x0
%define 		BLUE 		0x1
%define 		GREEN 		0x2
%define 		CYAN 		0x3
%define 		RED 		0x4
%define 		MAGENTA 	0x5
%define 		BROWN 		0x6
%define 		LGRAY 		0x7
%define 		DGRAY		0x8
%define 		LBLUE		0x9
%define 		LGREEN		0xA
%define 		LCYAN		0xB
%define 		LRED		0xC
%define 		LMAGENTA	0xD
%define 		YELLOW		0xE
%define 		WHITE		0xF

; Flags
%define 		MB_INFO_MEMORY			0x1
%define 		MB_INFO_BOOTDEVICE		0x2
%define 		MB_INFO_CMDLINE			0x4
%define 		MB_INFO_MODULES			0x8

; The next two are mutually exclusive
%define 		MB_INFO_AOUT			0x10
%define 		MB_INFO_ELF				0x20

; More Symbols
%define 		MB_INFO_MEM_MAP			0x40
%define 		MB_INFO_DRIVE_INFO		0x80
%define 		MB_INFO_CONFIG_TABLE	0x100
%define 		MB_INFO_BOOT_LDR_NAME	0x200
%define 		MB_INFO_APM_TABLE		0x400
%define 		MB_INFO_VIDEO_INFO		0x800

; R/EAX must contain this
%define 		MULTIBOOT_MAGIC			0x2BADBOO2

; Structure Definitions
struc MultiBoot
	.Flags				resd	1
	.MemoryLo			resd	1
	.MemoryHi			resd	1
	.BootDevice			resd	1
	.CmdLine			resd	1
	.ModuleCount		resd	1
	.ModuleAddr			resd	1
	.Symbols0			resd	1
	.Symbols1			resd	1
	.Symbols2			resd	1
	.Symbols3			resd	1
	.MemMapLength		resd	1
	.MemMapAddr			resd	1
	.DrivesLength		resd	1
	.DrivesAddr			resd	1
	.ROMConfigTable		resd	1
	.BootLoaderName		resd	1
	.ApmTable			resd	1
	.VBEControllerInfo	resd	1
	.VBEModeInfo		resd	1
	.VBEMode			resd	1
	.VBEInterfaceSeg	resd	1
	.VBEInterfaceOff	resd	1
	.VBEInterfaceLen	resd	1
endstruc


; ********************************
; SetTextColor
; IN:
; 	- AL: Background Color
;	- AH: Text Color
; ********************************
SetTextColor:
	; Save state
	push 	ax
	push 	bx

	; Pack AL/AH into BL
	mov 	bl, ah
	shl 	bl, 4
	and 	al, 0x0F
	or 		bl, al

	; Done
	mov 	byte [bTextColor], bl

	; Restore & Return
	pop 	bx
	pop 	ax
	ret


; ********************************
; PrintChar
; IN:
; 	- AL: Char
; ********************************
PrintChar:
	; Save state
	push 	ax
	push 	bx

	; Setup INT
	mov 	ah, 0x0E
	mov 	bl, byte [bTextColor]
	mov 	bh, 0x00
	int 	0x10

	; Restore & Return
	pop 	bx
	pop 	ax
	ret

; ********************************
; Print
; IN:
; 	- SI: NULL-Terminated String
; ********************************
Print:
	; Save state
	pushad

	; Loop
	.cLoop:
		; Load next byte from string
		lodsb

		; Check if NULL (EOS)
		or	al, al
		jz	.PrintDone

		; Print it
		call PrintChar
		jmp	.cLoop

	.PrintDone:
		; Restore & Return
		popad
		ret


/*
* Shows a 16-bit word to console. Need word to dump in% dx.
*/
HexTable:      .ascii "0123456789ABCDEF"
show_16bit_word:
   pushw %bx
   pushw %ax
   movw $HexTable, %bx
   movw %dx, %ax
   /* Do higher 4 bits from higher byte */
   andb $0xf0, %ah
   shr $0x4, %ah
   xchg %ah, %al
   xlat
   call dochar
   movw %dx, %ax
   /* Do lower 4 bits from higher byte */
   andb $0xf, %ah
   xchg %ah, %al
   xlat
   call dochar
   movw %dx, %ax
   /* Do higher 4 bits from lower byte */
   andb $0xf0, %al
   shr $0x4, %al
   xlat
   call dochar
   movw %dx, %ax
   /* Do lower 4 bits from lower byte */
   andb $0xf, %al
   xlat
   call dochar
   popw %ax
   popw %bx
   ret

; ********************************
; SystemsFail
; Only call if fatal error
; ********************************
SystemsFail:
	; No return from this


	cli
	hlt

; ********************************
; Variables
; ********************************
bTextColor							db 		0x0F


; ********************************
; MultiBoot Header
; ********************************
BootHeader:
istruc MultiBoot
	at MultiBoot.Flags,				dd 0
	at MultiBoot.MemoryLo,			dd 0
	at MultiBoot.MemoryHi,			dd 0
	at MultiBoot.BootDevice,		dd 0
	at MultiBoot.CmdLine,			dd 0
	at MultiBoot.ModuleCount,		dd 0
	at MultiBoot.ModuleAddr,		dd 0
	at MultiBoot.Symbols0,			dd 0
	at MultiBoot.Symbols1,			dd 0
	at MultiBoot.Symbols2,			dd 0
	at MultiBoot.Symbols3, 			dd 0
	at MultiBoot.MemMapLength,		dd 0
	at MultiBoot.MemMapAddr,		dd 0
	at MultiBoot.DrivesLength,		dd 0
	at MultiBoot.DrivesAddr,		dd 0
	at MultiBoot.ROMConfigTable,	dd 0
	at MultiBoot.BootLoaderName,	dd 0
	at MultiBoot.ApmTable,			dd 0
	at MultiBoot.VBEControllerInfo,	dd 0
	at MultiBoot.VBEModeInfo,		dd 0
	at MultiBoot.VBEMode,			dd 0
	at MultiBoot.VBEInterfaceSeg,	dd 0
	at MultiBoot.VBEInterfaceOff,	dd 0
	at MultiBoot.VBEInterfaceLen,	dd 0
iend

%endif