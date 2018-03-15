Architecture
------------
The ASP-8 (for "Almost as Simple as Possible") is a 12-bit computer built on
breadboards from 4000-series integrated circuits. The architecture consists of:

* Up to 4 kilowords of RAM (1k in current implementation)
* One accumulator register (A)
* A flags register with two fields: Carry, and Negative.
* A 12-bit ALU with 15 instructions, including conditional branches.

Instructions
------------
Each ASP-8 instruction word consists of three parts:
    A 4-bit operation,
    a 2-bit addressing mode,
and a 6-bit argument.

The operand for each instruction is formed from the addressing mode and
argument of the instruction word.

The operation field selects one of the following 15 instructions:

0: ADD
    Add: A <- A + [operand], Flags()
1: SUB
    Subtract: A <- A - [operand], Flags()
2: RSB
    Reverse subtract: A <- [operand] - A, Flags()
3: SHL
    Shift left: A <- [operand] << 1, Flags()
4: CMP
    Compare: [operand] - A, Flags()
5: LDA
    Load accumulator: A <- [operand]
6: STA
    Store accumulator: [operand] <- A
7: OUT
    Output: Display <- [operand]
8: JMP
    Jump: PC <- [operand]
9: JPC
    Jump if carry: if (Carry) PC <- [operand]
10: JPN
    Jump if negative: if (Negative) PC <- [operand]
11: JMS
    Jump to subroutine (see below)
12: NOP
    No operation
13: SWP
    Swap accumulator with memory: A <-> [operand]
15: HLT
    Halt execution

(operation 14 is reserved and functions as NOP.)

Assembler Instructions
----------------------
There are two pseudo-instructions that are not implemented by the computer,
but are instead instructions to the assembler. They are:

* WRD
   Write a word of memory with the given value (or 0 if no value specified)
* ORG
   Set the memory address of the next instruction

Addressing Modes
----------------
The addressing mode field selects one of the following addressing modes
("|" refers to concatenation. HIGH(x) and LOW(x) refer to the upper and
lower 6 bits of the word x, respectively):

0: Current page
    operand <- Mem[HIGH(PC)|argument]
1: Zero page
    operand <- Mem[argument]
2: Immediate
    operand <- argument
3: Accumulator
    operand <- Mem[A + argument]
4: Direct (jump instructions)
    PC <- HIGH(PC)|argument
5: Indirect (jump instructions)
    PC <- MEM[HIGH(PC)|argument]
6: Indirect zero page (jump instructions)
    PC <- MEM[argument]

The assembler will select the appropriate addressing mode based on the
instruction type and format of the argument given (see "Arguments" below).

Arguments
---------
Instruction arguments take one of the following forms:
* Immediate value: #
   examples:

    LDA #05    ; load the value 5 into A
    JPC #01    ; jump to word 1 in the current page if the carry flag is set
    ADD #x     ; add the address of the variable x to A

* Absolute address:
      An absolute address must either be in page 0, or in the same page as the
   current instruction.
   examples:

    LDA @1455   ; load the value in memory address 1455 octal
    STA $13     ; load the accumulator in memory address 13 hexadecimal
    ADD x       ; add the contents of location x to A

 Indirect address:
   A jump instruction can reference a memory location with contains the
   address to jump to.
   examples:

    JMP [@02]   ; jump to location stored in memory address 2
    JPC [x]     ; jump to address stored in the variable x if carry is set

* Direct address:
   Specifies a location in either the current page or page 0 to jump to
   examples:

    JMP loop    ; jump to location "loop" if it is reachable

* Accumulator:
   The contents of the accumulator plus a given offset are used to calculate an
   address that contains the data for the instruction.
   examples:

    LDA A+#03   ; A <- Mem[A + 3]
      
For immediate and absolute arguments, values starting with @ are in octal,
and values starting with $ are hexadecimal.

Jump To Subroutine
------------------
The JMS instruction stores the return address at the specified location,
then begins execution from the following word. Thus, a subroutine should
have a word reserved at the beginning to hold this return address. An
indirect jump instruction can be used to return to the calling routine.
For example, a subroutine can be written in assembly language as follows:

    myproc: WRD             ; reserve return address
            ...             ; procedure statements follow
            JMP [myproc]    ; return to caller

Memory Pages
------------
The ASP-8 is divided into 64 pages of 64 bytes each. An instruction can
reference memory in either page zero (zero page addressing) or in the same
page as the instruction (current page addressing). The assembler will select
the appropriate addressing mode for each instruction based on its argument.

Data in pages other than the current and zero page can be referenced via a
two-step loading process. First, by loading the address into A via a pointer in
either the current or zero page, then an accumulator-based load to load the
data at that address. For example:

         ORG @0100 ; set current page to 01, making page @77 unavailable
    ptr: WRD @7700 ; the address we are interested in
         ...
         LDA ptr   ; load the value in ptr (@7700) into A
         LDA A+#04 ; load the data at location @7704 into A

The Assembler
------------
Build the assembler with:

    rustc asm.rs

Run it with, for example:

    asm samples/first.asm

The assembler will produce a listing file showing the memory words used by the
program and what values they should be set to (in octal).
