;_MPXCOM.ASM
;Knppelpositionen einer RC-Fernsteuerung ber den seriellen Port einlesen
;Marius Greuel '95

        %noincl
	%nosyms
	masm51
	page 65,132
        model small
	include dos.inc

VERSION			= 100h	;Versionsnummer
TICKER_INT		= 8	;Tickerinterrupt
TIMER0			= 40h	;Ports des 8253
TIMER_CTRL		= 43h
INT_CW			= 20h	;Ports des 8259A
INT_MASK		= 21h
EOI			= 20h	;Ende Interrupt Befehl
RBR			= 0	;Datenregister
IER			= 1	;Interrupt Freigaberegister
IIR			= 2	;Interrupt Id Register
LCR			= 3	;Leitungs Kontrollregister
MCR			= 4	;Modem Kontrollregister
MSR			= 6	;Modem Statusregister
DCTS			= 01	;Status DCTS
DDSR			= 02	;Status DDSR
CTS			= 10h	;Status CTS
DSR			= 20h	;Status DSR
DTR			= 1	;Leitung DTR
RTS			= 2	;Leitung RTS
OUT2			= 8	;Leitung IRQ Freigabe
TIMEOUT			= 2	;Zeit fr den Timer in Ticks
TICK_LEN		= 54930	;Ticker-Konstante (Ticks -> ms)
CHANNELS		= 7	;soviel Kan„le einlesen
END_VALUE		= 256	;Endwert der Knppel
MAX_VALUE		= 256	;max. Wert der Knppel
MIN_PULSE_LEN		= 500	;min. L„nge des RC-Pulses in us
MAX_PULSE_LEN		= 2500	;max. L„nge des RC-Pulses in us
MIN_PULSE_DIFF		= 500	;min. Differenz
STD_MIN_PULSE_LEN	= 1050	;Standart-Werte (MPX)
STD_MAX_PULSE_LEN	= 2150
NO_ERROR		= 0
ERR_NOT_ENABLED		= 1
ERR_NOT_FOUND		= 2
ERR_NO_DATA		= 4

;BASE_PORT setzt Portbasis in DX fest
BASE_PORT	macro PORT
		mov dx,PORT
PORT_OFFSET	= 0
		endm

;Addressierung eines neuen Ports erfolgt durch PORT NewPort
SET_PORT	macro PORT
		add dl,PORT-PORT_OFFSET
PORT_OFFSET 	= PORT
		endm

	dosseg
	.model small, c
	.code
	.286
	locals

;************** Daten im Codesegment *****************************************

	even
Channel			dw CHANNELS dup (?)
ZeroTable		dw CHANNELS dup (?)
OldTickerInt		dd ?	;FAR-Pointer auf alten INT8
OldComInt		dd ?	;FAR-Pointer auf alten ComInt
OldIrqMask		db ?	;alte IRQ-Maske
OldLCR			db ?	;altes LCR-Register
OldIER			db ?	;altes IER-Register
OldMCR			db ?	;altes MCR-Register
TimerTicks		dw ?	;Meávariable fr den Timer
SyncLen			dw ?	;min. L„nge des Startimpulses in Ticks
ValueMul		dw ?	;Multiplikator um Knppelwert zu erhalten
ComPort			dw ?	;Port (wird aus der Tabelle ab 40:0 entnommen)
ComInt			db ?	;neuer Interrupt
CurrentChannel		db ?	;Kanal, der gerade gelesen wird
DRIVER_ACTIVE		= 01	;Treiber initialisiert
DATA_VALID		= 02	;Knppelwerte gltig
Flag			db 0	;Statusflag
ONLY_PULSE		= 01	;nur Pulsl„nge auswerten
NEG_PULSE		= 02	;negative Signale
SHOW_PULSE		= 04	;zeigt Pulsl„nge in us
Mode			db 0	;Modus
Timer			db TIMEOUT	;wenn TIMEOUT wird DATA_VALID gel”scht
ErrorCode		dw 0	;Fehlercode

	page
;************************* Codesegment ***************************************

	assume cs:_TEXT,ds:nothing,es:nothing,ss:nothing

;FAR C-calling convention
C_C	equ far
;Public-symbols:
	public InitRC
	public ExitRC
	public GetRCState
	public SetRCTiming
	public ZeroRCSticks
	public GetRCStick
        public GetRCPort
        public GetRCIRQ

;Initialisiert den Timer (setzt Modus 2)
;> -
;< -
InitTimer proc near
	cli
	mov al,34h
	out TIMER_CTRL,al
	mov al,0
	out TIMER0,al
	out TIMER0,al
	sti
	ret
InitTimer endp

;liest den Z„hler aus
;> -
;< Z„hlerstand in AX (in Ticks = 838*10^-9s)
ReadTimer proc near
	mov al,04
	out TIMER_CTRL,al
	in al,TIMER0
	mov ah,al
	in al,TIMER0
	xchg al,ah
	ret
ReadTimer endp
	
;eine Millisekunde warten
;> -
;< -
WaitMs proc near
	push ax cx
	call ReadTimer
	mov cx,ax
@@L1:	call ReadTimer
	sub ax,cx
	cmp ax,-1193		;(1193Ticks = 1ms)
	ja @@L1
	pop cx ax
	ret
WaitMs endp

;Timeout abwarten
;> -
;< -
WaitTimeout proc near
@@L1:	test cs:[Flag],DATA_VALID
	jnz @@L2
	cmp cs:[Timer],0
	jnz @@L1
@@L2:	ret
WaitTimeout endp

;*****************************************************************************

;Impulsl„ngen auswerten und speichern
;> Impulsl„nge in AX
;< -
SetChannel proc near
	cmp ax,cs:[SyncLen]		;testen,ob's ein SYNC-Impuls ist
	jae @@L2
	mov bl,cs:[CurrentChannel]	;nein, aktuellen Kanal holen
	cmp bl,CHANNELS			;  nur soviel Kan„le einlesen
	jae @@L1
	mov bh,0
	shl bx,1
	mov cs:[bx+Channel],ax		;Wert eintragen
	inc cs:[CurrentChannel]
@@L1:	ret
@@L2:	or cs:[Flag],DATA_VALID		;ja, DatenGltig-Flag setzen
	mov cs:[Timer],TIMEOUT
	mov cs:[CurrentChannel],0
	jmp @@L1
SetChannel endp

;Dies ist der neue serielle Interrupt
NewComInt proc near
	push ax bx dx
	BASE_PORT cs:[ComPort]
	SET_PORT IIR
	in al,dx			;testen, ob es unser INT war
	or al,al
        jnz @@L3
	SET_PORT MSR
	in al,dx			;testen, ob das Signal gewechselt hat
	test al,DCTS
        jz @@L3
	test cs:[Mode],NEG_PULSE	;negative Pulse auswerten ?
	jz @@L1
	not al				;  ja, Signal invertieren
@@L1:	test al,CTS			;Signal testen
	pushf
	call ReadTimer			;Timer auslesen
	popf
	jnz @@L2
	test cs:[Mode],ONLY_PULSE	;nur Pulsl„nge auswerten ?
	jz @@L3
	mov cs:[TimerTicks],ax		;  ja ,neuen Startwert festlegen
	jmp @@L3
@@L2:	xchg ax,cs:[TimerTicks]		;Startwert festlegen
	sub ax,cs:[TimerTicks]		;Differenz ermitteln
	call SetChannel			;und Wert eintragen
@@L3:	mov al,EOI
	out INT_CW,al			;Interrupt best„tigen
	pop dx bx ax
	iret
NewComInt endp

;Dies ist der neue Ticker-Interrupt
NewTickerInt proc near
	cmp cs:[Timer],0		;auf Timeout warten
	jz @@L1
	dec cs:[Timer]
	jnz @@L2
@@L1:	and cs:[Flag],not DATA_VALID	;und das Daten-gltig Flag l”schen
@@L2:	jmp cs:[OldTickerInt]
NewTickerInt endp

;*****************************************************************************

;alten COM- und Ticker-INT sichern
;> -
;< -
SaveInts proc near
	push es
	mov al,TICKER_INT
	mov ah,DOS_GET_VECTOR
	int DOS_FUNCTION
	mov word ptr cs:[OldTickerInt],bx
	mov word ptr cs:[OldTickerInt+2],es
	mov al,cs:[ComInt]
	mov ah,DOS_GET_VECTOR
	int DOS_FUNCTION
	mov word ptr cs:[OldComInt],bx
	mov word ptr cs:[OldComInt+2],es
	pop es
	ret
SaveInts endp

;neuen COM- und Ticker-INT setzen
;> -
;< -
SetInts proc near
	push ds
	mov al,TICKER_INT
	mov ah,DOS_SET_VECTOR
	mov dx,offset NewTickerInt
	push cs
	pop ds
	int DOS_FUNCTION
	mov al,cs:[ComInt]
	mov ah,DOS_SET_VECTOR
	mov dx,offset NewComInt
	push cs
	pop ds
	int DOS_FUNCTION
	pop ds
	ret
SetInts endp

;alten COM- und Ticker-INT wiederherstellen
;> -
;< -
RestoreInts proc near
	push ds
	mov al,TICKER_INT
	mov ah,DOS_SET_VECTOR
	lds dx,cs:[OldTickerInt]
	int DOS_FUNCTION
	mov al,cs:[ComInt]
	mov ah,DOS_SET_VECTOR
	lds dx,cs:[OldComInt]
	int DOS_FUNCTION
	pop ds
	ret
RestoreInts endp

;alte IRQ-Maske sichern
;> -
;< -
SaveIrqMask proc near
	in al,INT_MASK
	mov cs:[OldIrqMask],al
	ret
SaveIrqMask endp

;alte IRQ-Maske wiederherstellen
;> -
;< -
RestoreIrqMask proc near
	mov al,cs:[OldIrqMask]
	out INT_MASK,al
	ret
RestoreIrqMask endp

;Sperrt den COM-IRQ
;> -
;< -
ComIrqOff proc near
	mov ah,00001000b
	cmp cs:[ComInt],0bh
	jz @@L1
	mov ah,00010000b
	cmp cs:[ComInt],0ch
	jz @@L1
	ret
@@L1:	in al,INT_MASK
	or al,ah
	out INT_MASK,al		;ComIrqs sperren
	ret
ComIrqOff endp

;Erlaubt den COM-IRQ
;> -
;< -
ComIrqOn proc near
	mov ah,not 00001000b
	cmp cs:[ComInt],0bh
	jz @@L1
	mov ah,not 00010000b
	cmp cs:[ComInt],0ch
	jz @@L1
	ret
@@L1:	in al,INT_MASK
	and al,ah
	out INT_MASK,al		;ComIrqs freigeben
	ret
ComIrqOn endp

;alte COM-Parameter sichern
;> -
;< -
SaveComParameter proc near
	BASE_PORT cs:[ComPort]
	SET_PORT LCR
	in al,dx
	mov cs:[OldLCR],al
	mov al,0			;DLAB = 0
	out dx,al
	SET_PORT IER
	in al,dx
	mov cs:[OldIER],al
	SET_PORT MCR
	in al,dx
	mov cs:[OldMCR],al
	ret
SaveComParameter endp

;neue COM-Parameter setzen
;> -
;< -
SetComParameter proc near
	BASE_PORT cs:[ComPort]
	SET_PORT LCR
	mov al,0			;DLAB = 0
	out dx,al
	SET_PORT IER
	mov al,8			;EDSSI = 1
	out dx,al
	SET_PORT MCR
	mov al,RTS + OUT2
	out dx,al			;RTS = 1,OUT2 = 1 !
	SET_PORT RBR
	in al,dx			;Reste entfernen
	SET_PORT MSR
	in al,dx			;Reste entfernen
	ret
SetComParameter endp

;alte COM-Parameter wiederherstellen
;> -
;< -
RestoreComParameter proc near
	BASE_PORT cs:[ComPort]
	SET_PORT LCR
	mov al,0			;DLAB = 0
	out dx,al
	SET_PORT IER
	mov al,cs:[OldIER]
	out dx,al
	SET_PORT LCR
	mov al,cs:[OldLCR]
	out dx,al
	SET_PORT MCR
	mov al,cs:[OldMCR]
	out dx,al
	SET_PORT RBR
	in al,dx			;Reste entfernen
	SET_PORT MSR
	in al,dx			;Reste entfernen
	ret
RestoreComParameter endp

;sucht die Brcke zwischen RTS und DSR
;> -
;< -
TestIfInterfacePresent proc near
	BASE_PORT cs:[ComPort]
	SET_PORT LCR
	mov al,0			;DLAB = 0
	out dx,al
	SET_PORT IER
	mov al,0			;keine INTs
	out dx,al
	SET_PORT MCR
	mov al,RTS
	out dx,al			;RTS = 1
	SET_PORT RBR
	in al,dx			;Reste entfernen
	call WaitMs
	SET_PORT MSR
	in al,dx			;Reste entfernen
	in al,dx
	test al,DSR			;DSR muá 1 sein
	jz @@L1
	test al,DDSR
	jnz @@L1
	SET_PORT MCR
	mov al,0			;RTS = 0
	out dx,al
	call WaitMs
	SET_PORT MSR
	in al,dx
	test al,DSR			;DSR muá 0 sein
	jnz @@L1
	test al,DDSR
	jz @@L1
	clc
	ret
@@L1:	stc
	ret
TestIfInterfacePresent endp

;holt passend zum Port den Hardwareinterrupt
;> -
;< -
GetIrqPort proc near
	BASE_PORT cs:[ComPort]
	mov al,0ch
	cmp dx,3f8h
	jz @@L1
        cmp dx,3e8h
	jz @@L1
	mov al,0bh
        cmp dx,2f8h
        jz @@L1
        cmp dx,2e8h
        jz @@L1
        mov al,0ch
@@L1:	mov cs:[ComInt],al
	ret
GetIrqPort endp

;sucht die Fernsteuerung und legt den Port und IRQ fest
;> -
;< -
;CF, wenn kein Interface-Kabel eingesteckt ist
SearchPort proc near
	push ds
	mov ax,40h
	mov ds,ax
	xor bx,bx
@@L1:	mov dx,[bx]
	mov cs:[ComPort],dx
	or dx,dx
	jz @@L2
	call GetIrqPort
	call SaveComParameter
	call TestIfInterfacePresent
	pushf
	call RestoreComParameter
	popf
	jnc @@L3
@@L2:	add bx,2
	cmp bx,4*2
	jnz @@L1
	stc
@@L3:	pop ds
	ret
SearchPort endp

;*****************************************************************************

;Bestimmt das Impulstiming
;> AL = Modus
;  SI = Min. L„nge in us
;  DI = Max. L„nge in us
;< -
SetTiming proc near
	mov cs:[Mode],al
	cmp si,di
	jb @@L1
	xchg si,di
@@L1:	mov ax,di
	sub ax,si
	cmp ax,MIN_PULSE_DIFF
	jae @@L2
	mov si,STD_MIN_PULSE_LEN
	mov di,STD_MAX_PULSE_LEN
	jmp @@L4
@@L2:	cmp si,MIN_PULSE_LEN
	jae @@L3
	mov si,STD_MIN_PULSE_LEN
@@L3:	cmp di,MAX_PULSE_LEN
	jbe @@L4
	mov di,STD_MAX_PULSE_LEN
@@L4:	xor ax,ax
	mov dx,si
	mov bx,TICK_LEN
	div bx
	mov si,ax
	xor ax,ax
	mov dx,di
	mov bx,TICK_LEN
	div bx
	mov di,ax
	add ax,si
	mov cs:[SyncLen],ax
	sub di,si
	shr di,1
	xor ax,ax
	mov dx,END_VALUE
	cmp di,dx
	jb @@L5
	div di
@@L5:	mov cs:[ValueMul],ax
	add si,di
	xor bx,bx
@@L6:	mov cs:[bx+ZeroTable],si
	add bx,2
	cmp bx,CHANNELS*2
	jne @@L6
	ret
SetTiming endp

;liest einen Kanal aus
;> Knppel in BX
;< Knppelwert in AX
;  CF, wenn Fehler
Stick	proc near
	xor ax,ax
	cmp bx,CHANNELS
	jae @@F1
	test cs:[Flag],DATA_VALID
	jz @@F1
	shl bx,1
	mov ax,cs:[bx+Channel]
	test cs:[Mode],SHOW_PULSE
	jnz @@L3
	sub ax,cs:[bx+ZeroTable]
	imul cs:[ValueMul]
	mov ax,MAX_VALUE
	cmp dx,ax
	jg @@L1
	neg ax
	cmp dx,ax
	jl @@L1
	mov ax,dx
@@L1:	neg ax
@@L2:	clc
	ret
@@L3:	mov dx,TICK_LEN
	mul dx
	mov ax,dx
	jmp @@L2
@@F1:	stc
	ret
Stick	endp

;nullt die Knppelpositionen
;> -
;< -
ZeroSticks proc near
	call WaitTimeout
	test cs:[Flag],DATA_VALID	;Daten gltig ?
	jz @@F1				;wenn immer noch nicht,raus
@@L1:	xor ax,ax
	mov bx,ax
@@L2:	mov cs:[bx+ZeroTable],ax
	add bx,2
	cmp bx,CHANNELS*2
	jne @@L2
	mov cl,4
@@L3:	cmp cs:[CurrentChannel],1
	jnz @@L3
@@L4:	cmp cs:[CurrentChannel],0
	jnz @@L4
	xor bx,bx
@@L5:	mov ax,cs:[bx+Channel]
	add cs:[bx+ZeroTable],ax	;Knppelwerte eintragen
	add bx,2
	cmp bx,CHANNELS*2
	jne @@L5
	dec cl
	jnz @@L3
	xor bx,bx
@@L6:	shr cs:[bx+ZeroTable],2
	add bx,2
	cmp bx,CHANNELS*2
	jne @@L6
	ret
@@F1:	stc
	ret
ZeroSticks endp

;*****************************************************************************
;FAR - Routinen

;stellt die Pr„senz des Senders und den Port fest.
;> -
;< AX != 0, wenn Fehler
InitRC	proc C_C
	uses si,di
	mov cs:[ErrorCode],0
	test cs:[Flag],DRIVER_ACTIVE
	jnz @@F1
	call SearchPort
	jc @@F2
	call SaveIrqMask
	call SaveComParameter
	call SaveInts
	call ComIrqOff
	call SetInts
	call SetComParameter
	call InitTimer
	mov si,STD_MIN_PULSE_LEN
	mov di,STD_MAX_PULSE_LEN
	call SetTiming
	call ComIrqOn
	or cs:[Flag],DRIVER_ACTIVE
@@L1:	mov ax,cs:[ErrorCode]
	ret
@@F1:	mov cs:[ErrorCode], ERR_NOT_ENABLED
	jmp @@L1
@@F2:	mov cs:[ErrorCode], ERR_NOT_FOUND
	jmp @@L1
InitRC	endp

;setzt den Treiber zurck
;> -
;< AX != 0, wenn Fehler
ExitRC	proc C_C
	uses si,di
	mov cs:[ErrorCode],0
	test cs:[Flag],DRIVER_ACTIVE
	jz @@F1
	call ComIrqOff
	call RestoreInts
	call RestoreComParameter
	call RestoreIrqMask
	and cs:[Flag],not DRIVER_ACTIVE
@@L1:	mov ax,cs:[ErrorCode]
	ret
@@F1:	mov cs:[ErrorCode], ERR_NOT_ENABLED
	jmp @@L1
ExitRC	endp

;holt den Programmstatus
;> -
;< AX != 0, wenn Fehler
GetRCState proc C_C
	mov ax,cs:[ErrorCode]
	ret
GetRCState endp

;Bestimmt das Impulstiming
;> -
;< -
SetRCTiming proc C_C
	uses si,di
	arg @@Mode:WORD, @@MinTime:WORD, @@MaxTime:WORD
	mov cs:[ErrorCode],0
	test cs:[Flag],DRIVER_ACTIVE
	jz @@F1
	mov ax,[@@Mode]
	mov si,[@@MinTime]
	mov di,[@@MaxTime]
	call SetTiming
@@L1:	mov ax,cs:[ErrorCode]
	ret
@@F1:	mov cs:[ErrorCode], ERR_NOT_ENABLED
	jmp @@L1
SetRCTiming endp

;nullt die Knppelpositionen
;> -
;< AX != 0, wenn Fehler
ZeroRCSticks proc C_C
	uses si,di
	test cs:[Flag],DRIVER_ACTIVE
	jz @@F1
	call ZeroSticks
	jc @@F2
@@L1:	mov ax,cs:[ErrorCode]
	ret
@@F1:	mov cs:[ErrorCode], ERR_NOT_ENABLED
	jmp @@L1
@@F2:	mov cs:[ErrorCode], ERR_NO_DATA
	jmp @@L1
ZeroRCSticks endp

;liest einen Kanal aus
;> Knppel in BL
;< Knppelwert in EAX (+1 / -1)
GetRCStick proc C_C
	uses si,di
	arg @@Stick:WORD
	mov cs:[ErrorCode],0
	test cs:[Flag],DRIVER_ACTIVE
	jz @@F1
	mov bx,[@@Stick]
	call Stick
	jc @@F2
@@L1:	ret
@@F1:	mov cs:[ErrorCode], ERR_NOT_ENABLED
	xor ax,ax
	jmp @@L1
@@F2:	mov cs:[ErrorCode], ERR_NO_DATA
	xor ax,ax
	jmp @@L1
GetRCStick endp

GetRCPort proc C_C
        mov ax,ComPort
        ret
GetRCPort endp

GetRCIRQ proc C_C
        mov al,ComInt
        ret
GetRCIRQ endp

	end
