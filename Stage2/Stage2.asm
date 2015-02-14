; *******************************************************
; Mollen-OS Stage 2 Bootloader
; Copyright 2015 (C)
; Author: Philip Meulengracht
; Version 1.0
; *******************************************************
; Memory Map:
; 0x00000000 - 0x000004FF		Reserved
; 0x00000500 - 0x00007AFF		Second Stage Bootloader (~29 Kb)
; 0x00007B00 - 0x00007BFF		Stack Space (256 Bytes)
; 0x00007C00 - 0x00007DFF		Bootloader (512 Bytes)
; 0x00007E00 - 0x00008FFF		Used by subsystems in this bootloader
; 0x00009000 - 0x00009FFF		Memory Map
; 0x0000A000 - 0x0000AFFF		Vesa Mode Map / Controller Information
; 0x0000B000 - 0x0007FFFF		Kernel Loading Bay (420 Kb ? ish)
; Rest above is not reliable

; 16 Bit Code, Origin at 0x500
BITS 16
ORG 0x0500

; *****************************
; REAL ENTRY POINT
; *****************************
jmp Entry

; Definitions
%define 		MEMLOCATION_BOOTCODE			0x7C00
%define 		MEMLOCATION_FAT_GETCLUSTER		0x7E00
%define 		MEMLOCATION_FAT_FATTABLE		0x8000
%define 		MEMLOCATION_MEMORY_MAP			0x9000
%define 		MEMLOCATION_VESA_INFO			0xA000
%define 		MEMLOCATION_KERNEL				0xB000
%define 		MEMLOCATION_KERNEL_UPPER		0x100000


; Includes
%include "Systems/Common.inc"
%include "Systems/Memory.inc"
%include "Systems/A20.inc"
%include "Systems/Gdt.inc"

; FileSystem Includes
%include "Systems/FsCommon.inc"


; *****************************
; Entry Point
; *****************************
Entry:
	; Clear interrupts
	cli

	; Clear registers (Not EDX, it contains stuff!)
	xor 	eax, eax
	xor 	ebx, ebx
	xor 	ecx, ecx
	xor 	esi, esi
	xor 	edi, edi

	; Setup segments
	mov		ds, ax
	mov		es, ax
	mov		fs, ax
	mov		gs, ax

	; Setup stack
	mov		ss, ax
	mov		ax, 0x7BFF
	mov		sp, ax
	xor 	ax, ax

	; Enable interrupts
	sti

	; Save information passed by Stage1
	mov 	byte [bDriveNumber], dl
	mov 	byte [bStage1Type], dh

	; Now we can clear EDX
	xor 	edx, edx

	; Set video mode to 80x25 (Color Mode)
	mov 	al, 0x03
	mov 	ah, 0x00
	int 	0x10

	; Set text color
	mov 	al, BLACK
	mov 	ah, GREEN
	call 	SetTextColor

	; Print Message
	mov 	esi, szWelcome0
	call 	Print

	mov 	esi, szWelcome1
	call 	Print

	mov 	esi, szWelcome2
	call 	Print

	mov 	esi, szWelcome3
	call 	Print

	; Get memory map
	call 	SetupMemory

	; Enable A20 Gate
	call 	EnableA20

	; Install GDT
	call 	InstallGdt

	; Setup FileSystem (Based on Stage1 Type)
	xor 	eax, eax
	mov 	al, byte [bStage1Type]
	mov 	ah, byte [bDriveNumber]
	call 	SetupFS

	; Load Kernel
	mov 	esi, szKernel
	xor 	eax, eax
	xor 	ebx, ebx
	mov 	es, ax
	mov 	bx, MEMLOCATION_KERNEL
	call 	LoadFile

	; VESA System Select


	; GO PROTECTED MODE!


	; Kernel Relocation to 1mb (PE, ELF, binary)


	; Parse kernel type (PE, ELF, binary)
	; and get entrypoint


	; Jump to kernel 


	; Safety
	cli
	hlt

; ****************************
; Variables
; ****************************

; Strings - 0x0D (LineFeed), 0x0A (Carriage Return)
szWelcome0 						db 		"                ***********************************************", 0x0D, 0x0A, 0x00
szWelcome1						db 		"                * MollenOS Stage 2 Bootloader (Version 1.0.0) *", 0x0D, 0x0A, 0x00
szWelcome2						db 		"                * Author: Philip Meulengracht                 *", 0x0D, 0x0A, 0x00
szWelcome3 						db 		"                ***********************************************", 0x0D, 0x0A, 0x0A, 0x00
szPrefix 						db 		"                   - ", 0x00
szSuccess						db 		" [OK]", 0x0D, 0x0A, 0x00
szFailed						db 		" [FAIL]", 0x0D, 0x0A, 0x00

szKernel						db 		"KRNL32  MOS"

; Practical stuff
bDriveNumber 					db 		0

; 2 -> FAT12, 3 -> FAT16, 4 -> FAT32
bStage1Type						db 		0