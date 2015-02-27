; *******************************************************
; Mollen-OS Stage 1 Bootloader - FAT32 
; Copyright 2015 (C)
; Author: Philip Meulengracht
; Version 1.0
; *******************************************************
; Memory Map:
; 0x00000000 - 0x000004FF		Reserved
; 0x00000500 - 0x00007AFF		Second Stage Bootloader (~29 Kb)
; 0x00007B00 - 0x00007BFF		Stack Space (256 Bytes)
; 0x00007C00 - 0x00007DFF		Bootloader (512 Bytes)
; 0x00007E00 - 0x0007FFFF		Kernel Loading Bay (480.5 Kb)
; Rest above is not reliable

; In this bootloader we use the kernel loading bay a lot
; 0x00007E00 - 0x00007FFF 		GetNextCluster (Usage)
; 0x00008000 - ClusterSize		Used by LocateFile Procedure


; 16 Bit Code, Origin at 0x0
BITS 16
ORG 0x7C00


; Jump Code, 3 Bytes
jmp short Main
nop

; *************************
; FAT Boot Parameter Block
; *************************
szOemName					db		"MollenOS"
wBytesPerSector				dw		0
bSectorsPerCluster			db		0
wReservedSectors			dw		0
bNumFATs					db		0
wRootEntries				dw		0
wTotalSectors				dw		0
bMediaType					db		0
wSectorsPerFat				dw		0
wSectorsPerTrack			dw		0
wHeadsPerCylinder			dw		0
dHiddenSectors				dd 		0
dTotalSectors				dd 		0

; *************************
; FAT32 Extension Block
; *************************
dSectorsPerFat32			dd 		0
wFlags						dw		0
wVersion					dw		0
dRootDirStart				dd 		0
wFSInfoSector				dw		0
wBackupBootSector			dw		0

; Reserved 
dReserved0					dd		0 	;FirstDataSector
dReserved1					dd		0 	;ReadCluster
dReserved2					dd 		0 	;ReadCluster

bPhysicalDriveNum			db		0
bReserved3					db		0
bBootSignature				db		0
dVolumeSerial				dd 		0
szVolumeLabel				db		"NO NAME    "
szFSName					db		"FAT32   "


; *************************
; Bootloader Entry Point
; *************************

Main:
	jmp 	0x0:FixCS

FixCS:
	; Disable Interrupts, we are modifying
	; important registers
	cli

	; Fix segment registers to 0
	xor 	ax, ax
	mov		ds, ax
	mov		es, ax
	mov		fs, ax
	mov		gs, ax

	; Set stack
	mov		ss, ax
	mov		ax, 0x7BFF
	mov		sp, ax

	; Done, now we need interrupts again
	sti

	; Step 0. Save DL
	mov 	byte [bPhysicalDriveNum], dl

	; Step 1. Calculate FAT32 Data Sector
	xor		eax, eax
	mov 	al, byte [bNumFATs]
	mov 	ebx, dword [dSectorsPerFat32]
	mul 	ebx
	xor 	ebx, ebx
	mov 	bx, word [wReservedSectors]
	add 	eax, ebx
	mov 	dword [dReserved0], eax

	; Step 2. Read FAT Table
	mov 	esi, dword [dRootDirStart]

	; Read Loop
	.cLoop:
		mov 	bx, 0x0000
		mov 	es, bx
		mov 	bx, 0x8000
		
		; ReadCluster returns next cluster in chain
		call 	ReadCluster
		push 	esi

		; Step 3. Parse entries and look for szStage2
		mov 	di, 0x8000
		mov 	si, szStage2
		mov 	cx, 0x000B
		mov 	dx, 0x0020
		;mul by bSectorsPerCluster

		; End of root?
		.EntryLoop:
			cmp 	[es:di], ch
			je 		.cEnd

			; No, phew, lets check if filename matches
			pusha
        	repe    cmpsb
        	popa
        	jne 	.Next

        	; YAY WE FOUND IT!
        	; Get clusterLo & clusterHi
        	push    word [es:di + 14h]
        	push    word [es:di + 1Ah]
        	pop     esi
        	pop 	eax ; fix stack
        	jmp 	LoadFile

        	; Next entry
        	.Next:
        		add     di, 0x20
        		dec 	dx
        		jnz 	.EntryLoop

		; Dont loop if esi is above 0x0FFFFFFF5
		pop 	esi
		cmp 	esi, 0x0FFFFFF8
		jb 		.cLoop

	; Ehh if we reach here, not found :s
	.cEnd:
	cli
	hlt

; **************************
; Load 2 Stage Bootloader
; IN:
; 	- ESI Start cluster of file
; **************************
LoadFile:
	; Lets load the fuck out of this file
	; Step 1. Setup buffer
	push 	0x0000
	pop 	es
	mov 	bx, 0x0500

	; Load 
	.cLoop:
		; Clustertime
		call 	ReadCluster

		; Check
		cmp 	esi, 0x0FFFFFF8
		jb 		.cLoop

	; Done, jump
	mov 	dl, byte [bPhysicalDriveNum]
	mov 	dh, 4
	jmp 	0x0:0x500

	; Safety catch
	cli
	hlt


; **************************
; FAT ReadCluster
; IN: 
;	- ES:BX Buffer
;	- SI ClusterNum
;
; OUT:
;	- ESI NextClusterInChain
; **************************
ReadCluster:
	pusha

	; Save Bx
	push 	bx

	; Calculate Sector
	xor 	eax, eax
	xor 	bx, bx
	mov 	ax, si
	sub 	ax, 2 
	mov 	bl, byte [bSectorsPerCluster]
	mul 	bx
	add 	eax, dword [dReserved0]

	; Eax is now the sector of data
	pop 	bx
	mov 	cl, byte [bSectorsPerCluster]

	; Read
	call 	ReadSector

	; Save position
	mov 	word [dReserved2], bx
	push 	es

	; Si still has cluster num, call next
	call 	GetNextCluster
	mov 	dword [dReserved1], esi

	; Restore
	pop 	es

	; Done
	popa
	mov 	bx, word [dReserved2]
	mov 	esi, dword [dReserved1]
	ret


; **************************
; BIOS ReadSector 
; IN:
; 	- ES:BX: Buffer
;	- AX: Sector start
; 	- CL: Sector count
;
; Registers:
; 	- Conserves all but ES:BX
; **************************
ReadSector:
	pusha

	; Set initial 
	mov 	word [DiskPackage.Segment], es
	mov 	word [DiskPackage.Offset], bx
	mov 	word [DiskPackage.Sector], ax

	.sLoop:
		; Setup Package
		mov 	word [DiskPackage.SectorsToRead], 1

		; Setup INT
		mov 	al, 0
		mov 	ah, 0x42
		mov 	dl, byte [bPhysicalDriveNum]
		mov 	si, DiskPackage
		int 	0x13

		; It's important we check for offset overflow
		mov 	dx, word [wBytesPerSector]
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
	popa

	; Save position
	push 	ax
	mov 	ax, word [DiskPackage.Segment]
	mov 	es, ax
	mov 	bx, word [DiskPackage.Offset]
	pop 	ax
	ret

; **************************
; GetNextCluster
; IN:
; 	- SI ClusterNum
;
; OUT:
;	- ESI NextClusterNum
;
; Registers:
; 	- Trashes EAX, BX, ECX, DX, ES
; **************************
GetNextCluster:
	; Calculte Sector in FAT
	xor 	eax, eax
	xor 	edx, edx
	mov 	ax, si
	shl 	ax, 2 			; REM * 4, since entries are 32 bits long, and not 8
	div 	word [wBytesPerSector]
	add 	ax, word [wReservedSectors]
	push 	dx

	; AX contains sector
	; DX contains remainder
	mov 	ecx, 1
	mov 	bx, 0x0000
	mov 	es, bx
	mov 	bx, 0x7E00
	push 	es
	push 	bx

	; Read Sector
	call 	ReadSector
	pop 	bx
	pop 	es

	; Find Entry
	pop 	dx
	xchg 	si, dx
	mov 	esi, dword [es:bx + si]
	ret


; **************************
; Variables
; **************************
szStage2					db 		"SSBL    STM"

; This is used for the extended read function (int 0x13)
DiskPackage:				db 0x10
							db 0
	.SectorsToRead			dw 1
	.Offset					dw 0x0500
	.Segment 				dw 0x0000
	.Sector 				dq 0


; Fill out bootloader
times 510-($-$$) db 0

; Boot Signature
db 0x55, 0xAA