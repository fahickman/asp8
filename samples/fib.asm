; fib.asm
; -------
; Display the Fibonacci numbers < 2**11, repeatedly

x:      wrd             ; scratch 
y:      wrd             ; scratch

        org @0100

start:  lda #01         ; A <- 1
        sta x           ; x <- 1
        sta y           ; y <- 1

loop:   out y           ; display y
        add x           ; A <- x + y
        jpn start       ; if x + y < 0 restart
        swp y           ; y <-> A
        swp x           ; x <-> A
        lda y           ; A <- y
        jmp loop        ; next number
