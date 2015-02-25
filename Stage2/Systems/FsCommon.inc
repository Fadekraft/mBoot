; *******************************************************
; Mollen-OS Stage 2 Bootloader
; Copyright 2015 (C)
; Author: Philip Meulengracht
; Version 1.0
; *******************************************************
; FsCommon.inc
; - Contains shared filesystem routines
; 

%ifndef _MBOOT_FILESYSTEM_COMMON_INC_
%define _MBOOT_FILESYSTEM_COMMON_INC_

BITS 16

; Definitions


; Includes
%include "Systems/Fat.inc"


; **************************
; BIOS ReadSector 
; IN:
; 	- ES:BX: Buffer
;	- EAX: Sector start
; 	- ECX: Sector count
; 	- EDX: Sector size in bytes
;
; Registers:
; 	- Conserves all but ES:BX
; **************************
ReadSector:
	pushad

	; Set initial 
	mov 	word [DiskPackage.Segment], es
	mov 	word [DiskPackage.Offset], bx
	mov 	dword [DiskPackage.Sector], eax

	.sLoop:
		; Setup Package
		mov 	word [DiskPackage.SectorsToRead], 1

		; Setup INT
		push 	edx
		mov 	al, 0
		mov 	ah, 0x42
		mov 	dl, byte [bDriveNumber]
		mov 	si, DiskPackage
		int 	0x13

		; It's important we check for offset overflow
		pop 	edx
		mov 	ax, word [DiskPackage.Offset]
		add 	ax, dx
		mov 	word [DiskPackage.Offset], ax
		test 	ax, ax
		jne 	.NoOverflow

	.Overflow:
		; So overflow happened
		add 	word [DiskPackage.Segment], 0x1000
		mov 	word [DiskPackage.Offset], 0x0000

	.NoOverflow:
		; Loop
		inc 	dword [DiskPackage.Sector]
		loop 	.sLoop

	.End:
	; Restore registers 
	popad

	; Save position
	push 	eax
	xor 	eax, eax
	mov 	ax, word [DiskPackage.Segment]
	mov 	es, ax
	mov 	bx, word [DiskPackage.Offset]
	pop 	eax
	ret

; ****************************
; LoadFile
; IN:
; 	- DS:SI: Filename
; 	- ES:BX: Buffer
; OUT:
;	- EAX: Filesize
; ****************************
LoadFile:
	; Save state
	push 	ecx
	push 	edx
	push 	edi

	; Get type of FS
	xor 	ecx, ecx
	mov 	cl, byte [bFileSysType]

	; MFS? 
	cmp 	cl, 0
	je 		.MFS
	cmp 	cl, 1
	je 		.MFS

	; Fat? 
	cmp 	cl, 2
	je 		.FAT
	cmp 	cl, 3
	je 		.FAT
	cmp 	cl, 4
	je 		.FAT

	; Wtf?
	jmp 	.Done

	; MFS
	.MFS:
		jmp .Done

	; FAT
	.FAT:
		call 	FatLoadFile

	.Done:
		pop 	edi
		pop 	edx
		pop 	ecx
		ret


; ****************************
; SetupFS
; IN:
; 	- AL: Filesystem type
; 	- AH: Drive Number
; ****************************
SetupFS:
	; Save state
	pushad
	push 	ax

	; Save
	mov 	byte [bFileSysType], al

	; Print
	mov 	esi, szPrefix
	call 	Print

	; 0, 1 => MFS
	cmp 	al, 0
	je 		.NotSupported

	cmp 	al, 1
	je 		.NotSupported

	; 2, 3, 4 => FAT
	cmp 	al, 2
	je 		.SetupFAT12

	cmp 	al, 3
	je 		.SetupFAT16

	cmp 	al, 4
	je 		.SetupFAT32

	; End of list
	jmp 	.NotSupported


	; FAT
	.SetupFAT12:
		; Print
		mov 	esi, szFsSetup
		call 	Print

		mov 	esi, szFat12
		call 	Print

		; Setup
		pop 	ax
		mov 	al, FAT_TYPE_FLOPPY12
		call 	SetupFat
		jmp 	.Done

	.SetupFAT16:
		; Print
		mov 	esi, szFsSetup
		call 	Print

		mov 	esi, szFat16
		call 	Print

		; Setup
		pop 	ax
		mov 	al, FAT_TYPE_HDD16
		call 	SetupFat
		jmp 	.Done
		
	.SetupFAT32:
		; Print
		mov 	esi, szFsSetup
		call 	Print

		mov 	esi, szFat32
		call 	Print

		; Setup
		pop 	ax
		mov 	al, FAT_TYPE_HDD32
		call 	SetupFat
		jmp 	.Done

	; Not supported
	.NotSupported:
		; Print
		mov 	esi, szNotSupported
		call 	Print

		;Damn
		call 	SystemsFail
		cli
		hlt

	; Done
	.Done:
		; Print
		mov 	esi, szSuccess
		call 	Print

		; Restore & Return
		popad
		ret


; ****************************
; Variables
; ****************************

; Strings - 0x0D (LineFeed), 0x0A (Carriage Return)
szFsSetup						db		"Setting up ", 0x00
szFsLoading						db 		"Loading Kernel", 0x00
szNotSupported					db 		"Filesystem is not supported", 0x00
szMfs							db 		"MFS", 0x00
szFat12							db		"FAT 12", 0x00
szFat16							db		"FAT 16", 0x00
szFat32 						db 		"Fat 32", 0x00

; Misc
bFileSysType					db 		0

; This is used for the extended read function (int 0x13)
DiskPackage:				db 0x10
							db 0
	.SectorsToRead			dw 0
	.Offset					dw 0
	.Segment 				dw 0
	.Sector 				dq 0

%endif