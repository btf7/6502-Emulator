; Listen for key presses and print the hex value to console

; Memory layout:
; FFF8 : 2 byte keyboard input
; ...
; FFFA : Console output

; Define constants
.def byte offsetZero 48; '0'
.def byte offsetA 55; 'A' - 10
.def word input $fff8
.def word output $fffa

.org $8000

; Since X and Y aren't used,
; use them to store 0 and newline
; for small performance gain as
; lda # doesn't have to be called
ldx #0
ldy #10

waitLoop:
    ; Only print if input != $0000
    lda input
    ora input+1
    beq waitLoop

    ; Print

    ; High byte first nibble
    lda input+1
    lsr A
    lsr A
    lsr A
    lsr A
    jsr printNibble

    ; High byte second nibble
    lda input+1
    jsr printNibble

    ; Low byte first nibble
    lda input
    lsr A
    lsr A
    lsr A
    lsr A
    jsr printNibble

    ; Low byte second nibble
    lda input
    jsr printNibble

    ; Reset input
    stx input
    stx input+1

    ; Print \n
    sty output

    jmp waitLoop

; Print low nibble of A
printNibble:
    and #$0f

    ; If nibble - 10 is negative, nibble is 0-9
    cmp #10
    bmi printDecimal

    ; Print A-F
    clc
    adc #offsetA
    sta output
    rts

printDecimal:
    ; Print 0-9
    clc
    adc #offsetZero
    sta output
    rts