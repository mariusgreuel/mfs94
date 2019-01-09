;_TIMER.ASM
;Zeit messen mit dem 8253
;Marius Greuel '95

	%noincl
	%nosyms
	masm51
	page 65,132

	include BIOSDATA.SEG

TIMER0			= 40h		;Port des Timer 0
TIMER_CTRL		= 43h		;Port der Timerkontrolle
TCMD_T0_MODE_2		= 34h		;Befehl Timer 0 auf Mode 2
TCMD_READ_T0		= 4		;Befehl Timer 0 lesen
TIMER0_TIME		= 0
TIMER0_TIMEOUT          = 1190          ;1 ms

	dosseg
	.model small, c
	.code
	.386
	locals

	page
;************************* Codesegment ***************************************

	assume cs:_TEXT, ds:nothing, es:nothing, ss:nothing

;FAR C-calling convention
C_C	equ far
;Public-symbols:
	public InitTimer
	public ReadTimer

;Initialisiert den Timer 0
;> -
;< AX = 0, wenn Fehler
InitTimer proc C_C
	cli
	mov al,TCMD_T0_MODE_2
	out TIMER_CTRL,al		;Timer 0 auf Modus 2 setzen
	mov al,TIMER0_TIME
	out TIMER0,al
	out TIMER0,al
	sti
        mov ax,1
        ret

	mov cx,100
	cli
	call ReadTimer
	mov ebx,eax
@@L1:	call ReadTimer
	sub eax,ebx
	jnz @@L2
	dec cx
	jnz @@L1
	jmp @@L4
@@L2:	cmp eax,TIMER0_TIMEOUT
	ja @@L4
	mov ax,1
@@L3:	sti
	ret
@@L4:	xor ax,ax
	jmp @@L3
InitTimer endp

;liest den Z„hler aus
;> -
;< Zeit in EAX Ticks
ReadTimer proc C_C
	uses ds
	mov ax,@BIOS_DATA
	mov ds,ax
	mov edx,ds:[@BD_TIMER_CNTR]
	mov al,TCMD_READ_T0
	out TIMER_CTRL,al
	cmp edx,ds:[@BD_TIMER_CNTR]
	pushf
	in al,TIMER0
	mov ah,al
	in al,TIMER0
	xchg al,ah
	not ax
	popf
	jz @@L1
	mov ax,-1
@@L1:	ror eax,16
	mov ax,dx
	ror eax,16
	mov edx,235902770	;Tickerkonstante
	imul edx
	mov eax,edx
	ror edx,16		;fr 16 Bits Systeme
	ret
ReadTimer endp

	end
