; first.asm
; ---------
; Display the sum of two numbers, then halt.

        org @0100

        lda x
        add y
        sta sum
        out sum
        hlt

x:      wrd 28
y:      wrd 14
sum:    wrd
