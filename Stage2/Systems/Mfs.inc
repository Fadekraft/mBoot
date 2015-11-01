; *******************************************************
; Mollen-OS Stage 2 Bootloader
; Copyright 2015 (C)
; Author: Philip Meulengracht
; Version 1.0
; *******************************************************
; Mfs.inc
; - Contains filesystem routines for MFS
; 

%ifndef _MBOOT_MFS_INC_
%define _MBOOT_MFS_INC_

BITS 16

; Structure Definitions
; Size is 52 bytes
struc MfsBootSectorStruct
	.JumpCode			resb	3
	.Magic				resd	1
	.Version			resb	1
	.Flags				resb	1
	.MediaType		    resb	1
	.BytesPerSector		resw	1
	.SectorsPerTrack	resw	1
	.HeadsPerCylinder	resw	1
	.TotalSectors		resq	1
	.ReservedSectors	resw	1
	.SectorsPerBucket	resw	1
	.MasterBucketSector	resq	1
	.MasterBucketMirror	resq	1
	.VolumeLabel		resb	8
endstruc

; **************************
; MFS GetNextBucket
; IN:
; 	- ESI Bucket
;
; OUT:
;	- ESI NextBucket
; **************************
GetNextBucket:
	; Save states
	push 	eax
	push 	ebx
	push 	ecx
	push 	edx
	push 	es

	; Calculte Bucket Sector Offset
	; BucketsPerSector = BytesPerSector / 4;
    ; SectorOffset = Bucket / BucketsPerSector;
    ; SectorIndex = Bucket % BucketsPerSector;
	xor 	eax, eax
	xor 	ebx, ebx
	xor 	edx, edx ; Zero out edx before division!!
	mov 	ax, word [MfsBootSector + MfsBootSectorStruct.BytesPerSector]
	mov		ebx, 0x4
	div 	ebx
	mov		ebx, eax ; ebx = BucketsPerSector
	xor		edx, edx
	mov		eax, esi
	div		ebx

	; EAX contains sector
	; EDX contains remainder
	push	edx
	push	eax

	mov		eax, dword [qBucketMapSector]
	mov		edx, dword [qBucketMapSector + 4]
	pop		ebx
	xor		ecx, ecx

	; addition
	add		eax, ebx
	adc		edx, ecx

	mov		esi, edx
	xor		edx, edx
	mov 	dx, word [MfsBootSector + MfsBootSectorStruct.BytesPerSector]
	mov 	ecx, 1
	xor		ebx, ebx
	mov 	es, bx
	mov 	bx, MEMLOCATION_FAT_GETCLUSTER
	push 	es
	push 	bx

	; Read Sector
	call 	ReadSector
	pop 	bx
	pop 	es

	; Find Entry
	pop 	edx
	xchg 	esi, edx
	shl		esi, 2
	mov 	esi, dword [es:bx + si]

	; Restore & Return
	pop 	es
	pop 	edx
	pop 	ecx
	pop 	ebx
	pop 	eax
	ret

; **************************
; MFS ReadBucket
; IN: 
;	- ES:BX Buffer
;	- ESI Bucket
;
; OUT:
;	- ESI NextBucket
; **************************
ReadBucket:
	pushad

	; Save Bx
	push	esi
	push 	bx

	; Calculate Sector
	; Sector = Bucket * SectorsPerBucket
	mov		eax, esi
	xor		esi, esi
	mov		si, word [MfsBootSector + MfsBootSectorStruct.SectorsPerBucket]
	xor		ecx, ecx
	call	Mul64

	; Eax;edx is now the sector of data
	mov		esi, edx
	xor 	ebx, ebx
	xor 	ecx, ecx
	xor 	edx, edx
	pop 	bx
	mov 	cx, word [MfsBootSector + MfsBootSectorStruct.SectorsPerBucket]
	mov 	dx, word [MfsBootSector + MfsBootSectorStruct.BytesPerSector]

	; Read
	call 	ReadSector

	; Save position
	pop		esi
	mov 	word [wMfsTemporary0], bx
	push 	es

	; Si still has cluster num, call next
	call 	GetNextBucket
	mov 	dword [wMfsTemporary1], esi

	; Restore
	pop 	es

	; Done
	popad
	mov 	bx, word [wMfsTemporary0]
	mov 	esi, dword [wMfsTemporary1]
	ret


; ****************************
; MfsLoadFile
; IN:
; 	- DS:ESI: Filename
;	- ECX: Filename Length
; 	- ES:BX: Buffer
; OUT:
;	- EAX: Filesize
; ****************************
MfsLoadFile:
	; Save arguments, we dont need them yet
	mov 	ax, es
	mov 	word [wMFSLoadFileFSegment], ax
	mov 	word [wMFSLoadFileFOffset], bx
	mov 	dword [dMFSLoadFileFName], esi

	; Step 1. Read root directory
	mov 	esi, dword [dRootBucket]

	; Read Loop
	.cLoop:
		mov 	bx, 0x0000
		mov 	es, bx
		mov 	bx, MEMLOCATION_FAT_FATTABLE
		
		; ReadCluster returns next cluster in chain
		push	ecx
		call 	ReadBucket
		pop		ecx
		push 	esi

		; Step 3. Parse entries and look for esi
		mov 	edi, MEMLOCATION_FAT_FATTABLE
		mov 	esi, dword [dMFSLoadFileFName]

		; Entry Count (dx)
		; Lengtho f name in CX
		xor 	eax, eax
		xor 	ebx, ebx
		push	ecx
		xor		ecx, ecx
		mov 	bx, word [MfsBootSector + MfsBootSectorStruct.SectorsPerBucket]
		shr		ebx, 1
		mov 	edx, ebx

		; End of root?
		.EntryLoop:
			cmp 	[es:edi], ch
			je 		.ExitLoop

			; No, phew, lets check if filename matches
			pop		ecx
			pushad
			add		edi, 0x48
        	repe    cmpsb
        	popad
        	jne 	.Next

        	; YAY WE FOUND IT!
        	; Get bucket
        	mov    esi, dword [es:edi + 0x4]

        	; Get filesize 
        	mov 	eax, dword [es:di + 0x38]
        	mov 	dword [dMFSLoadFileFSize], eax

        	; Lets load the fuck out of this file
			mov 	ax, word [wMFSLoadFileFSegment]
			mov 	es, ax
			mov 	bx, word [wMFSLoadFileFOffset]

			; Load 
			.dLoop:
				; Clustertime
				call 	ReadBucket

				; Check
				cmp 	esi, 0xFFFFFFFF
				jne		.dLoop


        	; We are done, end this
        	pop 	eax 		; Fix stack
        	jmp 	.Success

        	; Next entry
        	.Next:
        		add     di, 0x400
        		dec 	dx
        		jnz 	.EntryLoop

		; Dont loop if esi is above 0xFFFFFFFF
		pop 	esi
		cmp 	esi, 0xFFFFFFFF
		jne		.cLoop
		jmp 	.NotFound

	.ExitLoop:
		pop 	esi
		jmp 	.NotFound

	.Success:
		; Move filesize
		mov 	eax, dword [dMFSLoadFileFSize]
		ret

	.NotFound:
		; 0 Filesize
		mov 	eax, 0
		ret

; ****************************
; SetupMfs
; IN:
; 	- AL: 0 Expected
; 	- AH: Drive Number
; ****************************
SetupMfs:
	; Save State
	pushad

	; Save data
	mov 	byte [bMfsDriveNumber], ah

	; Copy bootcode to our structure
	mov 	esi, MEMLOCATION_BOOTCODE
	mov 	edi, MfsBootSector
	mov 	ecx, 0x34
	rep 	movsb

	; Done, I don't need distractions
	; when I do my calculations

	; Calculate where the bucket-map is
	xor 	eax, eax
	xor 	ebx, ebx
	xor		ecx, ecx

	; Buckets = TotalSectors / BucketSize
	mov 	eax, dword [MfsBootSector + MfsBootSectorStruct.TotalSectors]
	mov		edx, dword [MfsBootSector + MfsBootSectorStruct.TotalSectors + 4]
	mov 	cx, word [MfsBootSector + MfsBootSectorStruct.SectorsPerBucket]
	call	Div64
	mov 	dword [qBucketCount], eax
	mov 	dword [qBucketCount + 4], edx
	
	; BucketMapSize = Buckets * 4;
	mov		esi, eax
	mov		ecx, edx
	mov		eax, 4
	call	Mul64

	; BucketMapSector = (TotalSectors - ((BucketMapSize / BytesPerSector) + 1))
	xor		ecx, ecx
	mov		cx, word [MfsBootSector + MfsBootSectorStruct.BytesPerSector]
	call	Div64
	mov		ebx, 0x1
	xor		ecx, ecx

	; addition
	add		eax, ebx
	adc		edx, ecx

	mov		ebx, eax
	mov		ecx, edx
	mov		eax, dword [MfsBootSector + MfsBootSectorStruct.TotalSectors]
	mov		edx, dword [MfsBootSector + MfsBootSectorStruct.TotalSectors + 4]

	; subtract
	sub		eax, ebx
	sbb		edx, ecx

	; Save
	mov 	dword [qBucketMapSector], eax
	mov 	dword [qBucketMapSector + 4], edx

	; Now, get root bucket
	mov		eax, dword [MfsBootSector + MfsBootSectorStruct.MasterBucketSector]
	mov		esi, dword [MfsBootSector + MfsBootSectorStruct.MasterBucketSector + 4]
	mov		ecx, 1
	xor		ebx, ebx
	mov		es, bx
	mov		ebx, MEMLOCATION_FAT_GETCLUSTER
	call	ReadSector

	mov		eax, MEMLOCATION_FAT_GETCLUSTER
	mov		ebx, 12
	mov		ecx, [eax + ebx]
	mov		dword [dRootBucket], ecx

	; Done
	popad
	ret


; ****************************
; 64 Bit Division
; IN:
;	- EAX: Low Dword
;	- EDX: High Dword
;	- ECX: Divisor Value
; OUT:
;	- EAX: Low Dword Result
;	- EDX: High Dword Result
; ****************************
Div64:
	; Save ebx & init
	push ebx
	xor	ebx, ebx

	; Save lower 
	push eax
	mov eax, edx
	xor edx, edx

	; Don't divide by zero
	test eax, eax
	je .SkipHigh

	; Divide high by ECX
	div ecx 
	mov	ebx, eax
	
	.SkipHigh:
	pop eax
	div ecx
	mov edx, ebx
	pop ebx
	ret

; ****************************
; 64 Bit Mul
; IN:
;	- EAX: Right hand
;	- ESI: Low Dword left hand
;	- ECX: High Dword left hand
; OUT:
;	- EAX: Low Dword Result
;	- EDX: High Dword Result
; ****************************
Mul64:
	mov edx, eax
	sar edx, 31
	imul ecx, eax ; ecx = y_high *{signed} x
	mov ebx, edx
	imul ebx, esi ; ebx = sign_extension(x) *{signed} y_low
	add ecx, ebx ; ecx = y_high *{signed} x_low + x_high *{signed} y_low
	mul esi ; edx:eax = x_low *{unsigned} y_low
	lea edx, [ecx + edx] ; edx = high(x_low *{unsigned} y_low + y_high *{signed} x_low + x_high *{signed} y_low)
	ret

; ****************************
; Variables
; ****************************

; Strings - 0x0D (LineFeed), 0x0A (Carriage Return)


; Etc
bMfsDriveNumber					db 		0
qBucketMapSector				dq 		0
qBucketCount					dq 		0
dRootBucket						dd		0
wMfsTemporary0					dw 		0
wMfsTemporary1					dd 		0
dMFSLoadFileFName				dd 		0
dMFSLoadFileFSize				dd 		0
wMFSLoadFileFSegment 			dw 		0
wMFSLoadFileFOffset 			dw 		0


; ****************************
; BOOT CODE & BPB
; ****************************
MfsBootSector:
istruc MfsBootSectorStruct
	at MfsBootSectorStruct.JumpCode,				db	0, 0, 0
	at MfsBootSectorStruct.Magic,					dd	0
	at MfsBootSectorStruct.Version,					db	0
	at MfsBootSectorStruct.Flags,   				db	0
	at MfsBootSectorStruct.MediaType,				db	0
	at MfsBootSectorStruct.BytesPerSector,			dw	0
	at MfsBootSectorStruct.SectorsPerTrack,			dw	0
	at MfsBootSectorStruct.HeadsPerCylinder,		dw	0
	at MfsBootSectorStruct.TotalSectors,			dq	0
	at MfsBootSectorStruct.ReservedSectors,			dw	0
	at MfsBootSectorStruct.SectorsPerBucket,		dw	0
	at MfsBootSectorStruct.MasterBucketSector,		dq	0
	at MfsBootSectorStruct.MasterBucketMirror,		dq	0
	at MfsBootSectorStruct.VolumeLabel,				db	0, 0, 0, 0, 0, 0, 0, 0
iend

%endif