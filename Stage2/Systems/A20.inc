; *******************************************************
; Mollen-OS Stage 2 Bootloader
; Copyright 2015 (C)
; Author: Philip Meulengracht
; Version 1.0
; *******************************************************
; A20.inc
; - Contains routines for enabling A20 Gate
; 

%ifndef _MBOOT_A20_INC_
%define _MBOOT_A20_INC_

BITS 16

; Definitions


; ********************************
; CheckA20
; OUT: 
; 	- AX: 0 If A20 is disabled, 1 If enabled
; ******************************** 
CheckA20:
    pushf
    push 	ds
    push 	es
    push 	di
    push 	si
 
    cli
 
    xor 	ax, ax ; ax = 0
    mov 	es, ax
 
    not 	ax ; ax = 0xFFFF
    mov 	ds, ax
 
    mov 	di, 0x0500
    mov 	si, 0x0510
 
    mov 	al, byte [es:di]
    push 	ax
 
    mov 	al, byte [ds:si]
    push 	ax
 
    mov 	byte [es:di], 0x00
    mov 	byte [ds:si], 0xFF
 
    cmp 	byte [es:di], 0xFF
 
    pop 	ax
    mov 	byte [ds:si], al
 
    pop 	ax
    mov 	byte [es:di], al
 
    mov 	ax, 0
    je 		CheckA20__Exit
 
    mov 	ax, 1
 
CheckA20__Exit:
    pop 	si
    pop 	di
    pop 	es
    pop 	ds
    popf
 
    ret

; ********************************
; A20MethodKeyboardController
; ******************************** 
A20MethodKeyboardController:
        cli

        call    A20Wait
        mov     al,0xAD
        out     0x64,al

        call    A20Wait
        mov     al,0xD0
        out     0x64,al

        call    A20Wait2
        in      al,0x60
        push    eax

        call    A20Wait
        mov     al,0xD1
        out     0x64,al

        call    A20Wait
        pop     eax
        or      al,2
        out     0x60,al

        call    A20Wait
        mov     al,0xAE
        out     0x64,al

        call    A20Wait
        sti
        ret

A20Wait:
        in      al,0x64
        test    al,2
        jnz     A20Wait
        ret


A20Wait2:
        in      al,0x64
        test    al,1
        jz      A20Wait2
        ret


; ********************************
; A20MethodBios
; ******************************** 
A20MethodBios:
	; Bios functon to enable A20
	mov 	ax, 0x2401
	int 	0x15
	ret

; ********************************
; EnableA20
; ******************************** 
EnableA20:
	; Save State
	pushad

	; Print
	mov 	esi, szPrefix
	call 	Print

	mov 	esi, szA20Setup
	call 	Print

	; Step 1, is it already enabled?
	call 	CheckA20
	cmp 	ax, 1
	je 		.Done

	; First we try to enable trough bios
	call 	A20MethodBios

	; Did it work?
	call 	CheckA20
	cmp 	ax, 1
	je 		.Done

	; Fuck, not supported then, try keyboard controller
	call 	A20MethodKeyboardController

	; Did it work?
	call 	CheckA20
	cmp 	ax, 1
	je 		.Done

	.Error:
		; Nooo :(
		mov 	esi, szFailed
		call 	Print

		; Halt
		call 	SystemsFail
		cli
		hlt

	.Done:
		; Print
		mov 	esi, szSuccess
		call 	Print

		; Restore & Return
		popad
		ret

; ********************************
; Variables
; ********************************

; Strings - 0x0D (LineFeed), 0x0A (Carriage Return)
szA20Setup 							db 		"Enabling A20 Gate", 0x00

%endif