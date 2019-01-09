;_EGA16.ASM
;Bildschirmtreiberdatei EGA 640*350, 16 Farben
;Marius Greuel '95

	page
	%noincl
	%nosyms
	masm51
	page 65,132

	include DOS.INC
	include BIOS.INC

VERSION			= 100h
GDC			= 3ceh
CRTC			= 3d4h
STATUS6845		= 3dah
TEXT_MODE		= 3h
GRAPHIC_MODE		= 10h
X_RES			= 640
Y_RES			= 350
FONT_X_RES		= 8
FONT_Y_RES		= 8
PIXEL_PER_BYTE		= 8
BYTES_PER_LINE		= X_RES/PIXEL_PER_BYTE
GRAPHIC_BUFFER_SIZE	= BYTES_PER_LINE*Y_RES
TEXT_BUFFER_SIZE	= 80*25*2

POINT			struc
X			dw ?
Y			dw ?
			ends

	dosseg
	.model small, c
	.code
	.386
	locals

	page
;************************* Daten im Codesegment ******************************

	even
TextSegment		dw 0b800h
GraphicSegment		dw 0a000h
TextBuffer		dw 0
FontSegment		dw 0f000h
FontPointer		dw 0fa6eh
SavedCursorPos		dw 0
;SavedCursorType	dw 0
SavedVideoMode		db 0
VPage			db 0
Color			db 0
INIT_OK			= 1
SCREEN_SAVED		= 2
VIDEO_MODE_SAVED	= 4
TEXT_MODE_ON		= 10h
GRAPHIC_MODE_ON		= 20h
State			db 0

page
;*********************** Codesegment *****************************************
;*********************** NEAR Routinen ***************************************
				
	assume cs:_TEXT, ds:nothing, es:nothing, ss:nothing

;FAR C-calling convention
C_C	equ far
;Public-symbols:
	public DetectGraph
	public InitGraph
	public ExitGraph
	public WaitForRetrace
	public SetActivePage
	public SetVisiblePage
	public SetColor
	public ClearScreen
	public DrawLine
	public DrawRectangle
	public FillRectangle
	public DrawPolygon
	public FillPolygon
	public DrawChar
	public DrawText

;Test, ob Grafikkarte vorhanden
;> -
;< CF wenn keine entsprechende Grafikkarte
_TestGraphicCard proc near
	mov ah,12h
	mov bl,10h
	int VIDEO_SERVICE
	cmp bl,10h	;EGA da ?
	stc
	jz @@L1
	cmp bl,3	;256 Kbyte ?
@@L1:	ret
_TestGraphicCard endp

;speichert alten Videomodus
;> -
;< -
_SaveVideoMode proc near
	test cs:[State],VIDEO_MODE_SAVED
	jnz @@L1
	mov ah,INT10_GET_MODE
	int VIDEO_SERVICE
	mov cs:[SavedVideoMode],al
	or cs:[State],VIDEO_MODE_SAVED
	cmp al,3
	ja @@L1
	mov ah,INT10_READ_CURSOR
	mov bh,0
	int VIDEO_SERVICE
	mov cs:[SavedCursorPos],dx
	mov ah,DOS_ALLOCATE_MEMORY
	mov bx,(TEXT_BUFFER_SIZE+15)/16
	int DOS_FUNCTION
	jc @@L1
	mov cs:[TextBuffer],ax
	mov ds,cs:[TextSegment]
	mov es,ax
	xor si,si
	mov di,si
	mov cx,TEXT_BUFFER_SIZE/2
	cld
	rep movsw
	or cs:[State],SCREEN_SAVED
@@L1:	ret
_SaveVideoMode endp

;restauriert alten Videomodus
;> -
;< -
_RestoreVideoMode proc near
	test cs:[State],VIDEO_MODE_SAVED
	jz @@L1
	mov al,cs:[SavedVideoMode]
	mov ah,INT10_SET_MODE
	int VIDEO_SERVICE
	and cs:[State],not VIDEO_MODE_SAVED
	test cs:[State],SCREEN_SAVED
	jz @@L1
	mov ds,cs:[TextBuffer]
	mov es,cs:[TextSegment]
	xor si,si
	mov di,si
	mov cx,TEXT_BUFFER_SIZE/2
	cld
	rep movsw
	mov ah,INT10_SET_CURSOR_POS
	mov bh,0
	mov dx,cs:[SavedCursorPos]
	int VIDEO_SERVICE
	mov ah,49h
	mov es,cs:[TextBuffer]
	int DOS_FUNCTION
	and cs:[State],not SCREEN_SAVED
@@L1:	ret
_RestoreVideoMode endp

;setzt Textmodus
;> -
;< -
_SetTextMode proc near
	test cs:[State],TEXT_MODE_ON
	jnz @@L1
	mov ax,TEXT_MODE
	int VIDEO_SERVICE
	and cs:[State],not GRAPHIC_MODE_ON
	or cs:[State],TEXT_MODE_ON
@@L1:	ret
_SetTextMode endp

;setzt Grafikmodus
;> -
;< -
_SetGraphicMode proc near
	test cs:[State],GRAPHIC_MODE_ON
	jnz @@L1
	mov ax,GRAPHIC_MODE
	int VIDEO_SERVICE
	xor ax,ax
	call _SetActivePage
	xor ax,ax
	call _SetVisiblePage
	and cs:[State],not TEXT_MODE_ON
	or cs:[State],GRAPHIC_MODE_ON
@@L1:	ret
_SetGraphicMode endp

;wartet auf Strahlenrcklauf
;> -
;< -
_WaitForRetrace proc near
	mov dx,STATUS6845
	mov ah,8		;Vertical Retrace
@@L1:	in al,dx
	test al,ah
;	jnz @@L1
@@L2:	in al,dx
	test al,ah
	jz @@L2
	ret
_WaitForRetrace endp

;l”scht den Bildschirmpuffer
;> -
;< -
_ClearScreen proc near
	mov al,0
	mov ah,cs:[Color]
	mov dx,GDC
	out dx,ax
	mov ax,0f01h
	out dx,ax
	mov ax,0ff08h
	out dx,ax
	mov es,cs:[GraphicSegment]
	xor di,di
	cld
	mov cx,GRAPHIC_BUFFER_SIZE/2
	mov ax,-1
	rep stosw
	ret
_ClearScreen endp

;tauscht die Videoseite, auf die geschrieben wird
;> -
;< -
_SetActivePage proc near
	and cs:[GraphicSegment],not 0800h
	or ax,ax
	jz @@L1
	or cs:[GraphicSegment],0800h
@@L1:	ret
_SetActivePage endp

;tauscht die Videoseite, die sichtbar ist
;> -
;< -
_SetVisiblePage proc near
	or ax,ax
	mov ax,0ch
	jz @@L1
	or ah,80h
@@L1:	mov dx,CRTC
	out dx,ax
	ret
_SetVisiblePage endp

;setzt Farbe
;> AL = Farbe
;< -
_SetColor proc near
	push dx
	mov cs:[Color],al
	mov dx,GDC
	mov ah,al
	mov al,0
	out dx,ax
	mov ax,0f01h
	out dx,ax
	pop dx
	ret
_SetColor endp

;zeichnet horizontale Linie
;> CX = X1
;  DX = Y1
;  SI = X2
_DrawHLine proc near
	push es
	mov es,cs:[GraphicSegment]
	imul di,dx,BYTES_PER_LINE
	inc si
	mov ax,cx
	shr ax,3
	add di,ax
	mov dx,cx
	and cl,7
	mov al,-1
	shr al,cl
	mov cx,si
	and cl,7
	mov ch,al
	mov ax,0ff00h
	ror ax,cl
	mov ah,ch
	mov cl,3
	shr dx,cl
	shr si,cl
	mov cx,si
	sub cx,dx
	mov dx,GDC
	jnz @@L2
	and ah,al
	mov al,8
	out dx,ax
	or es:[di],ah
	jmp @@L4
@@L2:	push ax
	mov dx,GDC
	mov al,8
	out dx,ax
	or es:[di],ah
	inc di
	dec cx
	jz @@L3
	mov ax,0ff08h
	out dx,ax
	mov al,ah
	shr cx,1
	rep stosw
	jnc @@L3
	stosb
@@L3:	pop ax
	xchg al,ah
	mov al,8
	out dx,ax
	or es:[di],ah
@@L4:	pop es
	ret
_DrawHLine endp

;zeichnet vertikale Linie
;> CX = X1
;  DX = Y1
;  DI = Y2
_DrawVLine proc near
	cmp dx,di
	jbe @@L1
	xchg dx,di
@@L1:	sub di,dx
	imul bx,dx,BYTES_PER_LINE
	mov ax,cx
	shr ax,3
	add bx,ax		;BX=Offset
	mov ah,080h
	and cl,7
	ror ah,cl		;AL=Maske
	mov al,8
	mov dx,GDC
	out dx,ax
	inc di
@@L2:	inc byte ptr [bx]
	add bx,BYTES_PER_LINE
	dec di
	jnz @@L2
	ret
_DrawVLine endp

;zeichnet Linie
;> X1=CX, Y1=DX
;  X2=SI, Y2=DI
;< -
_DrawLine proc near
	cmp cx,si
	jz _DrawVLine
	jb @@L1
	xchg cx,si
	xchg dx,di
@@L1:	sub di,dx		;DI=delta y
	jz _DrawHLine
	sub si,cx		;SI=Delta X
	imul bx,dx,BYTES_PER_LINE
	mov ax,cx
	shr ax,3
	add bx,ax		;BX=Offset
	mov al,8
	mov dx,GDC
	out dx,al
	inc dl
	mov ax,8080h
	and cl,7
	ror ax,cl		;AX=Maske
	or di,di
	js @@L30
@@L10:	cmp si,di
	jb @@L20
	mov cx,si
	mov bp,cx
	inc cx
	shr bp,1
	neg di
@@L11:	or al,ah
@@L12:	add bp,di
	jns @@L13
	add bp,si
	out dx,al
	inc byte ptr [bx]
	ror ah,1
	adc bx,BYTES_PER_LINE
	mov al,ah
	dec cx
	jnz @@L12
	ret
	even
@@L13:	ror ah,1
	jnb @@L14
	out dx,al
	inc byte ptr [bx]
	inc bx
	mov al,ah
	dec cx
	jnz @@L12
	ret
	even
@@L14:	dec cx
	jnz @@L11
	out dx,al
	inc byte ptr [bx]
	ret
	even
@@L20:	mov cx,di
	mov bp,cx
	inc cx
	shr bp,1
	neg si
	jmp @@L22
	even
@@L21:	add bp,si
	jns @@L23
	add bp,di
	ror al,1
	adc bx,BYTES_PER_LINE
@@L22:	out dx,al
	inc byte ptr [bx]
	dec cx
	jnz @@L21
	ret
	even
@@L23:	add bx,BYTES_PER_LINE
	inc byte ptr [bx]
	dec cx
	jnz @@L21
	ret
	even
@@L30:	neg di
	cmp si,di
	jb @@L40
	mov cx,si
	mov bp,cx
	inc cx
	shr bp,1
	neg di
@@L31:	or al,ah
@@L32:	add bp,di
	jns @@L33
	add bp,si
	out dx,al
	inc byte ptr [bx]
	ror ah,1
	adc bx,-BYTES_PER_LINE
	mov al,ah
	dec cx
	jnz @@L32
	ret
	even
@@L33:	ror ah,1
	jnb @@L34
	out dx,al
	inc byte ptr [bx]
	inc bx
	mov al,ah
	dec cx
	jnz @@L32
	ret
	even
@@L34:	dec cx
	jnz @@L31
	out dx,al
	inc byte ptr [bx]
	ret
	even
@@L40:	mov cx,di
	mov bp,cx
	inc cx
	shr bp,1
	neg si
	jmp @@L42
	even
@@L41:	add bp,si
	jns @@L43
	add bp,di
	ror al,1
	adc bx,-BYTES_PER_LINE
@@L42:	out dx,al
	inc byte ptr [bx]
	dec cx
	jnz @@L41
	ret
	even
@@L43:	add bx,-BYTES_PER_LINE
	inc byte ptr [bx]
	dec cx
	jnz @@L41
	ret
_DrawLine endp

;zeichnet geflltes Rechteck
;> X=CX, Y=DX
;  SX=SI, SY=DI
;< -
_FillRectangle proc near
	or cx,cx
	js @@L31
	or dx,dx
	js @@L31
	mov es,cs:[GraphicSegment]
	add si,cx
	mov bp,di
	imul di,dx,BYTES_PER_LINE
	mov dx,cx
	and cl,7
	mov al,-1
	shr al,cl
	mov bh,al
	mov cx,si
	and cl,7
	mov ax,0ff00h
	ror ax,cl
	mov bl,al
	shr dx,3
	shr si,3
	sub si,dx
	add di,dx
	mov dx,GDC
@@L3:	push di
	mov ah,bh
	mov cx,si
	or cx,cx
	jnz @@L4
	and ah,bl
	mov al,8
	out dx,ax
	or es:[di],ah
	jmp @@L6
@@L31:	jmp @@L7
@@L4:	mov al,8
	out dx,ax
	or es:[di],ah
	inc di
	dec cx
	jz @@L5
	mov ax,0ff08h
	out dx,ax
	mov ax,-1
	shr cx,1
	rep stosw
	jnc @@L5
	stosb
@@L5:	mov ah,bl
	mov al,8
	out dx,ax
	or es:[di],ah
@@L6:	pop di
	add di,BYTES_PER_LINE
	dec bp
	jnz @@L3
@@L7:	ret
_FillRectangle endp

;gibt Zeichen aus
;> X=CX, Y=DX
;  AL = Zeichen
;< -
_DrawChar proc near
	cbw
	mov ds,cs:[FontSegment]
	mov si,cs:[FontPointer]
	shl ax,3
	add si,ax
	imul bx,dx,BYTES_PER_LINE
	mov ax,cx
	shr ax,3
	add bx,ax
	mov di,bx
	mov ch,FONT_Y_RES
	and cl,7
	mov dx,GDC
@@L1:	mov bl,[si]
	inc si
	mov bh,0
	ror bx,cl
	mov ah,bl
	mov al,8
	out dx,ax
	or es:[di],ah
	mov ah,bh
	mov al,8
	out dx,ax
	or es:[di+1],ah
	add di,BYTES_PER_LINE
	dec ch
	jnz @@L1
@@L2:	ret
	endp


;*********************** FAR-Routinen Public *********************************

;Grafikkarte vorhanden ?
;> -
;< AX = FALSE, wenn nicht
DetectGraph proc C_C
	call _TestGraphicCard
	jc @@F1
	mov ax,1
@@L1:	ret
@@F1:	xor ax,ax
	jmp @@L1	
DetectGraph endp

;Initialisiert die Grafikkarte
;> -
;< AX = FALSE, wenn Fehler
InitGraph proc C_C
	uses si,di, ds,es
	test cs:[State],INIT_OK
	jnz @@F1
	call _TestGraphicCard
	jc @@F1
	call _SaveVideoMode
	call _SetGraphicMode
	or cs:[State],INIT_OK
	mov ax,1
@@L1:	ret
@@F1:	xor ax,ax
	jmp @@L1
InitGraph endp

;setzt die Grafikkarte in den Ursprungszustand zurck
;> -
;< AX = FALSE, wenn Fehler
ExitGraph proc C_C
	uses si,di, ds,es
	test cs:[State],INIT_OK
	jz @@F1
	push ds es
	call _RestoreVideoMode
	pop es ds
	and cs:[State],not INIT_OK
	mov ax,1
@@L1:	ret
@@F1:	xor ax,ax
	jmp @@L1
ExitGraph endp

;wartet auf Strahlenrcklauf
;> -
;< -
WaitForRetrace proc C_C
	test cs:[State],GRAPHIC_MODE_ON
	jz @@L1
	call _WaitForRetrace
@@L1:	ret
WaitForRetrace endp

;tauscht die Videoseite, auf die geschrieben wird
;> -
;< -
SetActivePage proc C_C
	arg @@Page:WORD
	test cs:[State],GRAPHIC_MODE_ON
	jz @@L1
	mov ax,[@@Page]
	call _SetActivePage
@@L1:	ret
SetActivePage endp

;tauscht die Videoseite, die sichtbar ist
;> -
;< -
SetVisiblePage proc C_C
	arg @@Page:WORD
	test cs:[State],GRAPHIC_MODE_ON
	jz @@L1
	mov ax,[@@Page]
	call _SetVisiblePage
@@L1:	ret
SetVisiblePage endp

;setzt die Farbe
;> -
;< -
SetColor proc C_C
	arg @@Color:WORD
	test cs:[State],GRAPHIC_MODE_ON
	jz @@L1
	mov ax,[@@Color]
	call _SetColor
@@L1:	ret
SetColor endp

;l”scht den Bildschirmpuffer
;> -
;< -
ClearScreen proc C_C
	uses di, es
	test cs:[State],GRAPHIC_MODE_ON
	jz @@L1
	call _ClearScreen
@@L1:	ret
ClearScreen endp

;zeichet eine Linie
;> -
;< -
DrawLine proc C_C
	arg @@X1:WORD, @@Y1:WORD, @@X2:WORD, @@Y2:WORD, 
	uses si,di, ds
	test cs:[State],GRAPHIC_MODE_ON
	jz @@L1
	mov ds,cs:[GraphicSegment]
	mov cx,[@@X1]
	mov dx,[@@Y1]
	mov si,[@@X2]
	mov di,[@@Y2]
	push bp
	call _DrawLine
	pop bp
@@L1:	ret
DrawLine endp

;zeichet ein Rechteck
;> -
;< -
DrawRectangle proc C_C
	arg @@X:WORD, @@Y:WORD, @@XSize:WORD, @@YSize:WORD, 
	uses si,di, ds
	test cs:[State],GRAPHIC_MODE_ON
	jz @@L1
	mov ds,cs:[GraphicSegment]
	mov cx,[@@X]
	mov dx,[@@Y]
	mov si,[@@X]
	add si,[@@XSize]
	push si dx
	call _DrawHLine
	pop dx cx
	mov di,[@@Y]
	add di,[@@YSize]
	push cx di
	call _DrawVLine
	pop dx cx
	mov si,[@@X]
	push si dx
	xchg cx,si
	call _DrawHLine
	pop dx cx
	mov di,[@@Y]
	call _DrawVLine
@@L1:	ret
DrawRectangle endp

;zeichet ein geflltes Rechteck
;> -
;< -
FillRectangle proc C_C
	arg @@X:WORD, @@Y:WORD, @@XSize:WORD, @@YSize:WORD, 
	uses si,di, es
	test cs:[State],GRAPHIC_MODE_ON
	jz @@L1
	mov cx,[@@X]
	mov dx,[@@Y]
	mov si,[@@XSize]
	mov di,[@@YSize]
	push bp
	call _FillRectangle
	pop bp
@@L1:	ret
FillRectangle endp

;zeichet ein Polygon
;> -
;< -
DrawPolygon proc C_C
	arg @@Count:WORD, @@Points:DWORD
	uses si,di, ds,es
	test cs:[State],GRAPHIC_MODE_ON
	jz @@L2
	mov ds,cs:[GraphicSegment]
	mov ax,[@@Count]
	mov bx,ax
	dec bx
	shl bx,2
	les di,[@@Points]
	mov cx,es:[bx+di].X
	mov dx,es:[bx+di].Y
@@L1:	push ax di
	mov si,es:[di].X
	mov di,es:[di].Y
	push si di bp
	call _DrawLine
	pop bp dx cx di ax
	add di,type POINT
	dec ax
	jnz @@L1
@@L2:	ret
DrawPolygon endp

;zeichet ein geflltes Polygon
;> -
;< -
FillPolygon proc C_C
	arg @@Count:WORD, @@Points:DWORD
	uses si,di, ds,es
	test cs:[State],GRAPHIC_MODE_ON
	jz @@L1
	mov es,cs:[GraphicSegment]
	push bp
	mov cx,[@@Count]
	lds si,[@@Points]
	call _FillPolygon
	pop bp
@@L1:	ret
FillPolygon endp

;gibt ein Zeichen aus
;> -
;< -
DrawChar proc C_C
	arg @@X:WORD, @@Y:WORD, @@Char:WORD
	uses si,di, ds, es
	test cs:[State],GRAPHIC_MODE_ON
	jz @@L1
	mov es,cs:[GraphicSegment]
	mov cx,[@@X]
	mov dx,[@@Y]
	mov ax,[@@Char]
	call _DrawChar
@@L1:	ret
DrawChar endp

;gibt ein Zeichen aus
;> -
;< -
DrawText proc C_C
	arg @@X:WORD, @@Y:WORD, @@Text:DWORD
	uses si,di, ds, es
	test cs:[State],GRAPHIC_MODE_ON
	jz @@L3
	mov es,cs:[GraphicSegment]
	mov cx,[@@X]
	mov dx,[@@Y]
	lds si,[@@Text]
	cld
	jmp @@L2
@@L1:	push cx dx si ds
	call _DrawChar
	pop ds si dx cx
	add cx,FONT_X_RES
@@L2:	lodsb
	or al,al
	jnz @@L1
@@L3:	ret
DrawText endp

;zeichnet horizontale Linie
;> BX = Line-Offset
;  SI = X1
;  DI = X2
DRAW_ROW macro
	cmp di,si
	jbe @@L11
	xchg si,di
@@L11:	inc si

	mov cx,si
	and cl,7
	mov ax,0ff00h
	ror ax,cl

	mov cx,di
	and cl,7
	mov ah,-1
	shr ah,cl

	shr si,3
	shr di,3

	mov dx,GDC

	sub si,di
	add di,bp

	or si,si
	jnz @@L12

	and ah,al
	mov al,8
	out dx,ax
	or es:[di],ah
	jmp @@L14
@@L12:	push ax
	mov al,8
	out dx,ax
	or es:[di],ah
	inc di
	dec si
	jz @@L13
	mov ax,0ff08h
	out dx,ax
	mov al,ah
	mov cx,si
	shr cx,1
	rep stosw
	jnc @@L13
	stosb
@@L13:	pop ax
	xchg al,ah
	mov al,8
	out dx,ax
	or es:[di],ah
@@L14:	endm

GET_NEXT_POINTER1 proc
	mov bx,cs:POINTER1
	mov cx,[bx].X
	mov si,cx
	mov dx,[bx].Y
	add bx,-size POINT
	cmp bx,cs:MIN_POINTER
	jnb @@L1
	mov bx,cs:MAX_POINTER
@@L1:	sub dx,[bx].Y
	mov cs:DY1,dx
	neg dx
	mov cs:ADD1,-1
	sub cx,[bx].X
	jnb @@L5
	neg cs:ADD1
	neg cx
@@L5:	mov cs:DX1,cx
	cmp cx,dx
	jbe @@L9
	mov dx,cx
	jmp @@L8
@@L9:	mov cx,dx
	neg cx
@@L8:	sar cx,1
	mov cs:DIFF1,cx
	inc dx
	mov cs:LEN1,dx
	mov cs:POINTER1,bx
	ret
	endp

GET_NEXT_POINTER2 proc
	mov bx,cs:POINTER2
	mov cx,[bx].X
	mov di,cx
	mov dx,[bx].Y
	add bx,type POINT
	cmp bx,cs:MAX_POINTER
	jna @@L1
	mov bx,cs:MIN_POINTER
@@L1:	sub dx,[bx].Y
	mov cs:DY2,dx
	neg dx
	sub cx,[bx].X
	mov cs:ADD2,-1
	jnb @@L5
	neg cs:ADD2
	neg cx
@@L5:	mov cs:DX2,cx
	cmp cx,dx
	jbe @@L8
	mov dx,cx
	jmp @@L9
@@L8:	mov cx,dx
	neg cx
@@L9:	sar cx,1
	mov cs:DIFF2,cx
	inc dx
	mov cs:LEN2,dx
	mov cs:POINTER2,bx
	ret
	endp

CALC_LINE_1 macro
@@L21:	mov ax,cs:DIFF1
	mov cx,cs:LEN1
	or ax,ax
	js @@L23
@@L22:	dec cx
	jz @@L29
	add si,cs:ADD1
	add ax,cs:DY1
	jns @@L22
	add ax,cs:DX1
	mov cs:DIFF1,ax
	mov cs:LEN1,cx
	or al,1
	jmp @@L210
@@L29:	mov ax,cs:POINTER1
	cmp cs:POINTER2,ax
	jz @@L210
	call GET_NEXT_POINTER1
	jmp @@L21
@@L23:	dec cx
	jz @@L29
	add ax,cs:DX1
	js @@L24	
	add si,cs:ADD1
	add ax,cs:DY1
@@L24:	mov cs:DIFF1,ax
	mov cs:LEN1,cx
	or al,1
@@L210:	endm

CALC_LINE_2 macro
@@L31:	mov ax,cs:DIFF2
	mov cx,cs:LEN2
	or ax,ax
	js @@L33
@@L32:	dec cx
	jz @@L39
	add di,cs:ADD2
	add ax,cs:DY2
	jns @@L32
	add ax,cs:DX2
	mov cs:DIFF2,ax
	mov cs:LEN2,cx
	or al,1
	jmp @@L310
@@L39:	mov ax,cs:POINTER1
	cmp cs:POINTER2,ax
	jz @@L310
	call GET_NEXT_POINTER2
	jmp @@L31
@@L33:	dec cx
	jz @@L39
	add ax,cs:DX2
	js @@L34	
	add di,cs:ADD2
	add ax,cs:DY2
@@L34:	mov cs:DIFF2,ax
	mov cs:LEN2,cx
	or al,1
@@L310:	endm

MIN_POINTER		dw 0
MAX_POINTER		dw 0
POINTER1		dw 0
POINTER2		dw 0
DX1			dw 0
DY1			dw 0
DX2			dw 0
DY2			dw 0
DIFF1			dw 0
DIFF2			dw 0
LEN1			dw 0
LEN2			dw 0
ADD1			dw 0
ADD2			dw 0
LAST_POINTER1		equ 01
LAST_POINTER2		equ 01

GetMinMaxPointer macro
	mov cs:[MIN_POINTER],si		;Zeiger auf ersten Punkt
	dec cx
	shl cx,2	
	add cx,si
	mov cs:[MAX_POINTER],cx		;Zeiger auf letzten Punkt
	endm

SearchHighestPoint macro
	mov ax,-1			;kleinsten Y-Wert suchen -> AX
	mov bx,si
@@L1:	cmp [si].Y,ax
	jnb @@L2
	mov ax,[si].Y
	mov bx,si
@@L2:	add si,type POINT
	cmp si,cx
	jna @@L1
	mov cs:POINTER1,bx
	mov cs:POINTER2,bx
	endm

_FillPolygon proc near
	GetMinMaxPointer
	SearchHighestPoint
	imul bp,ax,BYTES_PER_LINE


	call GET_NEXT_POINTER1
	call GET_NEXT_POINTER2
@@L3:	push si di
	DRAW_ROW
	pop di si
	add bp,BYTES_PER_LINE
	CALC_LINE_1
	CALC_LINE_2
	jnz @@L3
	ret
	endp

	end
