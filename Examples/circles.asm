; Memory layout:
;
; 0000 : Centre X position
;
; 0001 : Centre Y position
;
; 0002 : Radius
;
; 0003 : X distance from center
;
; 0004 : Y distance from center
;
; 0005 : 2 byte value used in the midpoint algorithm
;      : When positive, the circle is too big and y should decrement
;      : When negative, the circle is too small and y should remain constant
;
; 0007 : 2 byte value where the current pixel's address is stored
;
; 0009 : When drSub is called, subtract this number from dr
;
; 000A : When drawPixel is called, the X register is changed.
;      : Store the old value here and restore it afterwards
;
; E000 : Video output
; |    :
; EFFF :

; Variable pointers
.def byte x 0
.def byte y 1
.def byte r 2
.def byte dx 3
.def byte dy 4
.def byte dr 5
.def byte pixelPos 7
.def byte numToSub 9
.def byte oldX 10

.org $8000

lda #32
sta x
lda #32
sta y
lda #31
sta r
jsr drawCircle

lda #27
sta r
jsr drawCircle

lda #23
sta r
jsr drawCircle

lda #13
sta r
jsr drawCircle

lda #19
sta y
lda #5
sta r
jsr drawCircle

lda #44
sta x
lda #37
sta y
lda #5
sta r
jsr drawCircle

jmp *

; Draw a circle outline using the midpoint algorithm
; Expects 1 <= r <= 32
; Based on the following python code:
;
; dx = 0
; dy = r
; dr = 5 - 4*dy
; while dx <= dy:
;     screen.set_at((x+dx, y+dy), white)
;     screen.set_at((x-dx, y+dy), white)
;     screen.set_at((x+dx, y-dy), white)
;     screen.set_at((x-dx, y-dy), white)
;     screen.set_at((x+dy, y+dx), white)
;     screen.set_at((x-dy, y+dx), white)
;     screen.set_at((x+dy, y-dx), white)
;     screen.set_at((x-dy, y-dx), white)
;
;     dx += 1
;     if dr > 0:
;         dy -= 1
;         dr -= 8*dy
;     dr += 8*dx + 4
; return
drawCircle:
    lda #0
    sta dx

    lda r
    sta dy

    lda #5
    sta dr
    lda #0
    sta dr+1
    lda dy
    asl A
    asl A
    sta numToSub
    jsr drSub

    drawCircleLoop:
        lda dy
        cmp dx
        bcc drawCircleReturn

        jsr drawPixels

        inc dx

        lda dr+1
        bmi decYSkip

        dec dy
        lda dy
        asl A
        asl A
        asl A
        sta numToSub
        jsr drSub

    decYSkip:
        lda dx
        asl A
        asl A
        asl A
        clc
        adc #4
        jsr drAdd

        jmp drawCircleLoop

drawCircleReturn:
    rts

drawPixels:
    ; x+dx, y+dy
    lda x
    clc
    adc dx
    tax
    lda y
    clc
    adc dy
    tay
    jsr drawPixel

    ; x-dx, y+dy
    lda x
    sec
    sbc dx
    tax
    jsr drawPixel

    ; x-dx, y-dy
    lda y
    sec
    sbc dy
    tay
    jsr drawPixel

    ; x+dx, y-dy
    lda x
    clc
    adc dx
    tax
    jsr drawPixel

    ; x+dy, y+dx
    lda x
    clc
    adc dy
    tax
    lda y
    clc
    adc dx
    tay
    jsr drawPixel

    ; x-dy, y+dx
    lda x
    sec
    sbc dy
    tax
    jsr drawPixel

    ; x-dy, y-dx
    lda y
    sec
    sbc dx
    tay
    jsr drawPixel

    ; x+dy, y-dx
    lda x
    clc
    adc dy
    tax
    jsr drawPixel

    rts

drawPixel:
    jsr getPixelPos
    lda #$ff
    stx oldX; Preserve X register
    ldx #0
    sta (pixelPos,X)
    ldx oldX
    rts

; Get the memory address of the pixel at X, Y
; Expects X register = x pos, Y register = y pos
getPixelPos:
    tya
    lsr A
    lsr A
    clc
    adc #$e0
    sta pixelPos+1

    tya
    asl A
    asl A
    asl A
    asl A
    asl A
    asl A
    sta pixelPos

    txa
    and #%00111111
    ora pixelPos
    sta pixelPos

    rts

; dr += A
drAdd:
    clc
    adc dr
    sta dr
    bcc drAddReturn
    inc dr+1
drAddReturn:
    rts

; dr -= numToSub
drSub:
    lda dr
    sec
    sbc numToSub
    sta dr
    bcs drSubReturn
    dec dr+1
drSubReturn:
    rts