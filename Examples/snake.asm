; As the screen is 64 x 64, there are 4096 possible positions
; These positions are stored in 2 bytes as 0000 YYYY  YYXX XXXX
; However, the 6502 uses little endian, so in memory it's YYXX XXXX  0000 YYYY

; Memory layout:

; 0000 : Head position

; 0002 : Tail position

; 0004 : Apple position

; 0006 : 2 byte pointer for indexing into dirTable or vidOut

; 0008 : 2 byte random number

; 000A : Temporary 1 byte variable for storing head direction
;      : This is needed because the head direction is not written until after the tail has been moved,
;      : and moving the tail changes the direction value in Y

; 000B : 2 byte value for snake length
;      : If you play long enough to fill the whole screen, the game will get stuck trying to spawn an apple
;      : To fix this, keep track of snake length, and when it hits 4096, restart the game

; 1000 : 4096 byte table of snake part directions
; |    : Index into this table to get the direction to the previous snake part
; |    : This is used to keep track of where to move tailPosition
; |    : Note that values in this table won't be cleared when the tail moves, only set when the head moves
; 1FFF : This table therefore cannot be used to detect collisions, as old values will persist

; E000 : 4096 byte video output
; |    :
; EFFF :

; FFF8 : 2 byte keyboard input

; FFFB : 1 byte delay output in milliseconds

; Define pointers to the variables
.def byte headPos $00
.def byte tailPos $02
.def byte applePos $04
.def byte pointer $06
.def byte random $08
.def byte tmpHeadDir $0a
.def byte snakeLen $0b
.def word dirTable $1000
.def word vidOut $e000
.def word keyboardIn $fff8
.def word delayOut $fffb

; Define constants
.def byte up 0
.def byte right 1
.def byte down 2
.def byte left 3
.def byte delay 100
.def byte rngFeedback $2c
.def byte moveHitWall 1
.def byte moveSuccess 0

.org $8000

ldx #0; Most instructions don't have indirect addressing - use indirect,X instead
;       X is cleared here as it is never used for anything else
jsr init
lda #delay
sta delayOut

mainLoop:
    jsr tickRNG
    jsr readInput
    jsr moveSnake
    lda #delay
    sta delayOut
    jmp mainLoop

init:
    ; Snake should start in middle going right

    ; Starting snake head is at 32,32
    lda #%00100000
    sta headPos
    lda #%00001000
    sta headPos+1

    ; Starting snake tail is at 30,32
    lda #%00011110
    sta tailPos
    lda #%00001000
    sta tailPos+1

    ; Direction table doesn't need cleared
    ; Just correct the values for used tiles
    lda #LO dirTable
    clc
    adc tailPos
    sta pointer
    lda #HI dirTable
    clc
    adc tailPos+1
    sta pointer+1

    lda #right
    ldy #0
    sta (pointer),Y
    iny
    sta (pointer),Y
    iny
    sta (pointer),Y

    ; Clear video out table
    lda #LO vidOut
    sta pointer
    lda #HI vidOut
    sta pointer+1

    clearVidOut:
        lda #0
        sta (pointer,X)

        inc pointer
        bne clearVidOut
        inc pointer+1
        lda pointer+1
        cmp #HI vidOut+$10
        bne clearVidOut

    ; Draw snake
    lda #LO vidOut
    clc
    adc tailPos
    sta pointer
    lda #HI vidOut
    clc
    adc tailPos+1
    sta pointer+1

    lda #$ff
    ldy #0
    sta (pointer),Y
    iny
    sta (pointer),Y
    iny
    sta (pointer),Y

    ; Spawn an apple
    jsr spawnApple

    ; Set snake length variable
    lda #3
    sta snakeLen

    rts

spawnApple:
    jsr tickRNG

    ; Set apple position
    lda random
    sta applePos
    lda random+1
    and #$0f
    sta applePos+1

    ; Check if the space is clear
    lda #LO vidOut
    clc
    adc applePos
    sta pointer
    lda #HI vidOut
    clc
    adc applePos+1
    sta pointer+1

    ; If it's not black, try again
    ; This stops it from spawning in the snake or in the same place again
    lda (pointer,X)
    bne spawnApple

    ; Draw apple
    lda #%11100000
    sta (pointer,X)

    rts

; RNG algorithm is based on https://www.analog.com/en/resources/design-notes/random-number-generation-using-lfsr.html
; If feedback value is 2 bytes, there are 2048 possible feedback values for a 2 byte random number
; Since feedback is only 1 byte for better performance, there are only 6 possible values:
; 2c, 38, 3e, 52, bc, and d6
; Here, 2c was chosen
tickRNG:
    ; 0 will produce another 0
    ; XOR with feedback beforehand if it's 0
    lda random
    bne rngSkipZeroCheck
    lda random+1
    bne rngSkipZeroCheck

    eor #rngFeedback
    sta random
rngSkipZeroCheck:
    ; Rotate left
    asl random
    rol random+1
    bcc rngSkipCarry
    lda random
    adc #0
    ; If it rotated, XOR with feedback
    eor #rngFeedback
    sta random
rngSkipCarry:
    rts

readInput:
    ; W = 0057
    ; A = 0041
    ; S = 0053
    ; D = 0044

    ; As all keys have 0 for high byte, skip if high byte != 0
    lda keyboardIn+1
    bne readInputEnd

    ; Get head direction address
    lda #LO dirTable
    clc
    adc headPos
    sta pointer
    lda #HI dirTable
    clc
    adc headPos+1
    sta pointer+1

    lda keyboardIn

    cmp #$57
    beq readInputW
    cmp #$41
    beq readInputA
    cmp #$53
    beq readInputS
    cmp #$44
    beq readInputD

readInputEnd:
    lda #0
    sta keyboardIn
    sta keyboardIn+1
    rts

readInputW:
    ; Can't be going down
    lda (pointer,X)
    cmp #down
    beq readInputEnd

    lda #up
    sta (pointer,X)
    jmp readInputEnd

readInputA:
    ; Can't be going right
    lda (pointer,X)
    cmp #right
    beq readInputEnd

    lda #left
    sta (pointer,X)
    jmp readInputEnd

readInputS:
    ; Can't be going up
    lda (pointer,X)
    cmp #up
    beq readInputEnd

    lda #down
    sta (pointer,X)
    jmp readInputEnd

readInputD:
    ; Can't be going left
    lda (pointer,X)
    cmp #left
    beq readInputEnd

    lda #right
    sta (pointer,X)
    jmp readInputEnd

moveSnake:
    ; Move head then tail
    ; Must move head first as if you hit an apple you must skip the tail move

    ; Get head direction address
    lda #LO dirTable
    clc
    adc headPos
    sta pointer
    lda #HI dirTable
    clc
    adc headPos+1
    sta pointer+1

    ; Get direction to new head
    lda (pointer,X)
    tay; Store in Y

    ; Head direction cannot be written yet as if the head is moving to where the tail currently is,
    ; the tail would then move in the wrong direction
    ; Store in temporary variable as Y will be overwritten with tail direction
    sta tmpHeadDir

    ; Get head position
    lda headPos
    sta pointer
    lda headPos+1
    sta pointer+1

    cpy #up
    bne skipMoveHeadUp
    jsr movePositionUp
    jmp moveHead
skipMoveHeadUp:
    cpy #right
    bne skipMoveHeadRight
    jsr movePositionRight
    jmp moveHead
skipMoveHeadRight:
    cpy #down
    bne skipMoveHeadDown
    jsr movePositionDown
    jmp moveHead
skipMoveHeadDown:
    ; Left is the only direction left
    jsr movePositionLeft

moveHead:
    ; Check A to see if we hit a wall
    cmp #moveSuccess
    ; Cannot use "bne snakeDied" as branch distance is too far
    beq headDidntHitWall
    jmp snakeDied

headDidntHitWall:
    ; Pointer now has new head position
    lda pointer
    sta headPos
    lda pointer+1
    sta headPos+1

    ; Head cannot be drawn yet as we have to detect collisions based on the colour of the pixel

    ; Check for apple
    lda headPos
    cmp applePos
    bne killTail
    lda headPos+1
    cmp applePos+1
    bne killTail

    ; We hit the apple

    ; First check if the snake has filled the whole map
    lda snakeLen
    clc
    adc #0
    lda snakeLen+1
    adc #0

    lda snakeLen
    bne snakeNotMaxLen
    lda snakeLen+1
    cmp #$10
    bne snakeNotMaxLen

    ; Snake is max length
    jmp snakeDied

snakeNotMaxLen:
    jsr spawnApple
    jmp skipTail

killTail:
    ; Draw tail black
    lda #LO vidOut
    clc
    adc tailPos
    sta pointer
    lda #HI vidOut
    clc
    adc tailPos+1
    sta pointer+1

    lda #0
    sta (pointer,X)

    ; Get tail direction address
    lda #LO dirTable
    clc
    adc tailPos
    sta pointer
    lda #HI dirTable
    clc
    adc tailPos+1
    sta pointer+1

    ; Get direction to new tail
    lda (pointer,X)
    tay; Store in Y

    ; Get tail position
    lda tailPos
    sta pointer
    lda tailPos+1
    sta pointer+1

    ; Move tail
    cpy #up
    bne skipMoveTailUp
    jsr movePositionUp
    jmp moveTail
skipMoveTailUp:
    cpy #right
    bne skipMoveTailRight
    jsr movePositionRight
    jmp moveTail
skipMoveTailRight:
    cpy #down
    bne skipMoveTailDown
    jsr movePositionDown
    jmp moveTail
skipMoveTailDown:
    ; Left is the only direction left
    jsr movePositionLeft

moveTail:
    ; Pointer now has new tail position
    lda pointer
    sta tailPos
    lda pointer+1
    sta tailPos+1

skipTail:
    ; Check for collisions
    ; Since we have not yet drawn the new head,
    ; if the head position is currently white then we have collided

    ; Get head direction address
    lda #LO dirTable
    clc
    adc headPos
    sta pointer
    lda #HI dirTable
    clc
    adc headPos+1
    sta pointer+1

    ; Set new head direction
    lda tmpHeadDir
    sta (pointer,X)

    ; Get head pixel address
    lda #LO vidOut
    clc
    adc headPos
    sta pointer
    lda #HI vidOut
    clc
    adc headPos+1
    sta pointer+1

    ; Check for collision
    lda (pointer,X)
    cmp #$ff
    bne skipCollision

snakeDied:
    ; We collided
    ; Wait 1 second
    lda #250
    sta delayOut
    sta delayOut
    sta delayOut
    sta delayOut

    ; Reset the game
    jsr init

    ; Theres a chance that buttons were pressed during the delay
    ; Ignore them
    lda #0
    sta keyboardIn
    sta keyboardIn+1

    rts
skipCollision:
    ; Draw head
    lda #$ff
    sta (pointer,X)

    rts

movePositionUp:
    lda pointer
    sec
    sbc #%01000000
    sta pointer

    lda pointer+1
    sbc #0
    cmp #$ff
    beq movePositionKill
    sta pointer+1
    lda #moveSuccess
    rts

movePositionDown:
    lda pointer
    clc
    adc #%01000000
    sta pointer

    lda pointer+1
    adc #0
    cmp #$10
    beq movePositionKill
    sta pointer+1
    lda #moveSuccess
    rts

movePositionRight:
    lda pointer
    and #%00111111
    cmp #%00111111
    beq movePositionKill
    inc pointer
    lda #moveSuccess
    rts

movePositionLeft:
    lda pointer
    and #%00111111
    beq movePositionKill
    dec pointer
    lda #moveSuccess
    rts

movePositionKill:
    lda #moveHitWall
    rts