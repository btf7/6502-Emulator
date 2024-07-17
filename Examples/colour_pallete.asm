.ORG $8000

; Memory layout:
; $00 : LO pos (YY0XXXXX)
; $01 : HI pos (1110000Y)
; $02 : Colour

; Setup
LDA #0
STA 0; LO pos
STA 2; Colour
LDA #$e0
STA 1; HI pos
LDY #0; STA doesn't have indirect - use indirect,Y instead

drawpix:
LDA 2; Get colour
STA (0),Y; Draw pixel

; Increment position and colour
INC 2; Inc colour
INC 0; Inc pos x
LDA 0;          Check if we've drawn the whole row
AND #%00011111; If 0, inc pos y
BNE drawpix;    Otherwise, draw next pixel

; Inc pos y LO byte
LDA 0
AND #%11000000
CLC
ADC #%01000000
STA 0;       If 0, inc pos y HI byte
BNE drawpix; Otherwise, draw next pixel

; Inc pos y HI byte
LDA 1
EOR #1; As only 1 bit is used here, just toggle it on/off
STA 1
JMP drawpix