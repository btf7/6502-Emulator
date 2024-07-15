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
LDY #0; STA doesn't have indirect - use indirect,y instead

drawpix:
LDA 2; Get colour
STA (0),Y; Draw pixel

; Increment position and colour
INC 2; inc colour
INC 0; inc pos x
LDA 0
AND #%00011111; if 0, inc pos y
BNE drawpix;    otherwise, draw next pixel
LDA 0; inc pos y lo byte
AND #%11000000
CLC
ADC #%01000000
STA 0;       if carry, inc pos y hi byte
BNE drawpix; otherwise, draw next pixel
INC 1; inc pos y high byte
LDA 1
CMP #$e2;    if too big, reset
BNE drawpix; otherwise, draw next pixel
LDA #$e0; reset
STA 1
JMP drawpix