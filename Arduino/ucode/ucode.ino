#define SHIFT_DATA 2
#define SHIFT_CLOCK 3
#define EEPROM_WE 4  // active low
#define EEPROM_OE 13 // active low
#define EEPROM_DATA_0 5
#define EEPROM_WRITE_DELAY 2

// instruction set
// 00 ADD: Add to accumulator
// 04 SUB: Subtract from accumulator
// 10 RSB: Subtract accumulator from
// 14 SHL: Shift left into accumulator
// 20 CMP: Compare with accumulator and set flags
// 24 LDA: Load into accumulator
// 30 STA: Store accumulator
// 34 OUT: Output
// 40 JMP: Jump
// 44 JPC: Jump if carry flag set
// 50 JPN: Jump if negative flag set
// 54 JMS: Jump to subroutine
// 60 NOP: No operation
// 64 SWP: Swap with accumulator
// 70 ---: Illegal
// 74 HLT: Halt execution

// addressing modes
// 0 current page
// 1 zero page
// 2 immediate
// 3 accumulator

// ucode signals
// BANK 0
#define U_ARO 0x00000001ul // A register out
#define U_SUO 0x00000002ul // Sum out
#define U_PHO 0x00000004ul // PC high out
#define U_PLO 0x00000008ul // PC low out
#define U_IRO 0x00000010ul // I register out
#define U_HLT 0x00000020ul // halt
#define U_RV0 0x00000040ul // reserved
#define U_RV1 0x00000080ul // reserved

// BANK 1
#define U_MRI 0x00000100ul // M register in
#define U_MEI 0x00000200ul // RAM in
#define U_MEO 0x00000400ul // RAM out
#define U_SUB 0x00000800ul // subtract
#define U_OUT 0x00001000ul // output
#define U_ARI 0x00002000ul // A register in
#define U_BRI 0x00004000ul // B register in
#define U_JMP 0x00008000ul // jump (PC in)

// BANK 2
#define U_JPC 0x00010000ul // jump if carry (PC in AND C flag)
#define U_JPN 0x00020000ul // jump if negative (PC in AND N flag)
#define U_FRI 0x00040000ul // F register in
#define U_PCE 0x00080000ul // PC enable (increment)
#define U_IRI 0x00100000ul // I register in
#define U_RV2 0x00200000ul // reserved
#define U_RV3 0x00400000ul // reserved
#define U_END 0x00800000ul // next instruction

// pseudo signals
#define U_PCO (U_PHO | U_PLO) // PC out
#define U_ZEO 0               // zero bus

#define U_FE0 (U_PCO | U_MRI)          // fetch 0: M <- PC
#define U_FE1 (U_MEO | U_IRI | U_PCE)  // fetch 1: I <- RAM[M]
#define U_CUP (U_PHO | U_IRO | U_MRI)  // current-page addressing:  M <- HIGH(PC)|LOW(I)
#define U_ZEP (U_IRO | U_MRI)          // zero-page addressing:     M <- 0|LOW(I)
#define U_AC0 (U_IRO | U_BRI)          // accumulator addressing 0: B <- LOW(I)
#define U_AC1 (U_SUO | U_MRI)          // accumulator addressing 1: M <- A + B

PROGMEM static const unsigned long isa[64][8] = {
      { // 000: ADDc
      U_FE0,
      U_FE1,
      U_CUP,
      U_MEO | U_BRI,
      U_SUO | U_ARI | U_FRI,
      U_END
   }, { // 001: ADDz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_MEO | U_BRI,
      U_SUO | U_ARI | U_FRI,
      U_END
   }, { // 002: ADDi
      U_FE0,
      U_FE1,
      U_IRO | U_BRI,
      U_SUO | U_ARI | U_FRI,
      U_END
   }, { // 003: ADDa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_MEO | U_BRI,
      U_SUO | U_ARI | U_FRI,
      U_END
   }, { // 004: SUBc
      U_FE0,
      U_FE1,
      U_CUP,
      U_MEO | U_BRI,
      U_SUO | U_SUB | U_ARI | U_FRI,
      U_END
   }, { // 005: SUBz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_MEO | U_BRI,
      U_SUO | U_SUB | U_ARI | U_FRI,
      U_END
   }, { // 006: SUBi
      U_FE0,
      U_FE1,
      U_IRO | U_BRI,
      U_SUO | U_SUB | U_ARI | U_FRI,
      U_END
   }, { // 007: SUBa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_ARO | U_BRI,
      U_MEO | U_ARI,
      U_SUO | U_SUB | U_ARI | U_FRI,
      U_END
   }, { // 010: RSBc
      U_FE0,
      U_FE1,
      U_CUP,
      U_ARO | U_BRI,
      U_MEO | U_ARI,
      U_SUO | U_SUB | U_ARI | U_FRI,
      U_END
   }, { // 011: RSBz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_ARO | U_BRI,
      U_MEO | U_ARI,
      U_SUO | U_SUB | U_ARI | U_FRI,
      U_END
   }, { // 012: RSBi
      U_FE0,
      U_FE1,
      U_ARO | U_BRI,
      U_IRO | U_ARI,
      U_SUO | U_SUB | U_ARI | U_FRI,
      U_END
   }, { // 013: RSBa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_ARO | U_BRI,
      U_MEO | U_ARI,
      U_SUO | U_SUB | U_ARI | U_FRI,
      U_END
   }, { // 014: SHLc
      U_FE0,
      U_FE1,
      U_CUP,
      U_MEO | U_ARI | U_BRI,
      U_SUO | U_ARI | U_FRI,
      U_END
   }, { // 015: SHLz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_MEO | U_ARI | U_BRI,
      U_SUO | U_ARI | U_FRI,
      U_END
   }, { // 016: SHLi
      U_FE0,
      U_FE1,
      U_IRO | U_ARI | U_BRI,
      U_SUO | U_ARI | U_FRI,
      U_END
   }, { // 017: SHLa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_MEO | U_ARI | U_BRI,
      U_SUO | U_ARI | U_FRI,
      U_END
   }, { // 020: CMPc
      U_FE0,
      U_FE1,
      U_CUP,
      U_MEO | U_BRI,
      U_SUB | U_FRI,
      U_END
   }, { // 021: CMPz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_MEO | U_BRI,
      U_SUB | U_FRI,
      U_END
   }, { // 022: CMPi
      U_FE0,
      U_FE1,
      U_IRO | U_BRI,
      U_SUB | U_FRI,
      U_END
   }, { // 023: CMPa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_MEO | U_BRI,
      U_SUB | U_FRI,
      U_END
   }, { // 024: LDAc
      U_FE0,
      U_FE1,
      U_CUP,
      U_MEO | U_ARI,
      U_END
   }, { // 025: LDAz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_MEO | U_ARI,
      U_END
   }, { // 026: LDAi
      U_FE0,
      U_FE1,
      U_IRO | U_ARI,
      U_END
   }, { // 027: LDAa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_MEO, U_ARI,
      U_END,
   }, { // 030: STAc
      U_FE0,
      U_FE1,
      U_CUP,
      U_ARO | U_MEI,
      U_END
   }, { // 031: STAz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_ARO | U_MEI,
      U_END
   }, { // 032: STAi
      U_FE0,
      U_FE1,
      U_CUP,
      U_ARO | U_MEI,
      U_END,
   }, { // 033: STAa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_ARO | U_MEI,
      U_END
   }, { // 034: OUTc
      U_FE0,
      U_FE1,
      U_CUP,
      U_MEO | U_OUT,
      U_END
   }, { // 035: OUTz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_MEO | U_OUT,
      U_END
   }, { // 036: OUTi
      U_FE0,
      U_FE1,
      U_IRO | U_OUT,
      U_END
   }, { // 037: OUTa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_MEO | U_OUT,
      U_END
   }, { // 040: JMPc
      U_FE0,
      U_FE1,
      U_CUP,
      U_MEO | U_JMP,
      U_END
   }, { // 041: JMPz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_MEO | U_JMP,
      U_END
   }, { // 042: JMPi
      U_FE0,
      U_FE1,
      U_IRO | U_PHO | U_JMP,
      U_END
   }, { // 043: JMPa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_MEO | U_JMP,
      U_END
   }, { // 044: JPCc
      U_FE0,
      U_FE1,
      U_CUP,
      U_MEO | U_JPC,
      U_END
   }, { // 045: JPCz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_MEO | U_JPC,
      U_END
   }, { // 046: JPCi
      U_FE0,
      U_FE1,
      U_IRO | U_PHO | U_JPC,
      U_END
   }, { // 047: JPCa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_MEO | U_JPC,
      U_END
   }, { // 050: JPNc
      U_FE0,
      U_FE1,
      U_CUP,
      U_MEO | U_JPN,
      U_END
   }, { // 051: JPNz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_MEO | U_JPN,
      U_END
   }, { // 052: JPNi
      U_FE0,
      U_FE1,
      U_IRO | U_PHO | U_JPN,
      U_END
   }, { // 053: JPNa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_MEO | U_JPN,
      U_END
   }, { // 050: JMSc
      U_FE0,
      U_FE1,
      U_CUP,
      U_MEO | U_MRI,
      U_PCO | U_MEI,
      U_MRO | U_PCI,
      U_PCE,
      U_END
   }, { // 051: JMSz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_MEO | U_MRI,
      U_PCO | U_MEI,
      U_MRO | U_PCI,
      U_PCE,
      U_END
   }, { // 052: JMSi
      U_FE0,
      U_FE1,
      U_IRO | U_PHO | U_MRI,
      U_PCO | U_MEI,
      U_MRO | U_PCI,
      U_PCE,
      U_END
   }, { // 053: JMSa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_MEO | U_MRI,
      U_PCO | U_MEI,
      U_MRO | U_PCI,
      U_PCE,
   }, { // 060: NOPc
      U_FE0,
      U_FE1,
      U_CUP,
      U_END
   }, { // 061: NOPz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_END
   }, { // 062: NOPi
      U_FE0,
      U_FE1,
      U_END
   }, { // 063: NOPa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_END
   }, { // 064: SWPc
      U_FE0,
      U_FE1,
      U_CUP,
      U_MEO | U_BRI,
      U_ARO | U_MEI,
      U_ZEO | U_ARI,
      U_SUO | U_ARI,
      U_END
   }, { // 065: SWPz
      U_FE0,
      U_FE1,
      U_ZEP,
      U_MEO | U_BRI,
      U_ARO | U_MEI,
      U_ZEO | U_ARI,
      U_SUO | U_ARI,
      U_END
   }, { // 066: SWPi
      U_FE0,
      U_FE1,
      U_IRO | U_ARI,
      U_END
   }, { // 067: SWPa
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_MEO | U_BRI,
      U_ARO | U_MEI,
      U_ZEO | U_ARI,
      U_SUO | U_ARI,
   }, { // 070: ---c
      U_FE0,
      U_FE1,
      U_CUP,
      U_END
   }, { // 071: ---z
      U_FE0,
      U_FE1,
      U_ZEP,
      U_END
   }, { // 072: ---i
      U_FE0,
      U_FE1,
      U_END
   }, { // 073: ---a
      U_FE0,
      U_FE1,
      U_AC0,
      U_AC1,
      U_END
   }, { // 074: HLTc
      U_FE0,
      U_FE1,
      U_HLT
   }, { // 075: HLTz
      U_FE0,
      U_FE1,
      U_HLT
   }, { // 076: HLTi
      U_FE0,
      U_FE1,
      U_HLT
   }, { // 077: HLTa
      U_FE0,
      U_FE1,
      U_HLT
   },
};

static unsigned char getOutputByte(unsigned short address)
{
   unsigned int bank, inst, cycle;
   unsigned long control;
   unsigned char result;

   if (address > 2048) {
      return 0;
   }

   bank = address >> 9;
   cycle = (address >> 6) & 7;
   inst = address & 077;

   control = pgm_read_dword_near(&isa[inst][cycle]);
   result = (unsigned char)(control >> (bank << 3));

   return result;
}

static void writeEepromAddress(unsigned int address, int dataPin, int clockPin)
{
   shiftOut(dataPin, clockPin, MSBFIRST, (uint8_t)(address >> 8));
   shiftOut(dataPin, clockPin, MSBFIRST, (uint8_t)address);
}

static void writeEepromByte(unsigned char data, int dataPin0, int enablePin)
{
   for (int i = dataPin0; i < dataPin0 + 8; ++i, data >>= 1) {
      digitalWrite(i, data & 1);
   }

   digitalWrite(enablePin, LOW);
   digitalWrite(enablePin, HIGH);
}

static void dumpEeprom(unsigned int begin, unsigned int end)
{
   digitalWrite(EEPROM_WE, HIGH);

   for (int i = EEPROM_DATA_0; i < EEPROM_DATA_0 + 8; ++i) {
      pinMode(i, INPUT);
   }
   digitalWrite(EEPROM_OE, LOW);

   for (unsigned int i = begin; i < end; ++i) {
      unsigned char data = 0;

      writeEepromAddress(i, SHIFT_DATA, SHIFT_CLOCK);

      for (int k = EEPROM_DATA_0 + 8; k-- > EEPROM_DATA_0; ) {
         data = (data << 1) | digitalRead(k);
      }

      Serial.write("0123456789abcdef"[data >> 4]);
      Serial.write("0123456789abcdef"[data & 15]);
      Serial.write(((i + 1) & 15) ? ' ' : '\n');
   }
}

void setup()
{
   Serial.begin(9600);
   Serial.write("Programming");

   digitalWrite(EEPROM_OE, HIGH);
   pinMode(EEPROM_OE, OUTPUT);
   digitalWrite(EEPROM_WE, HIGH);
   pinMode(EEPROM_WE, OUTPUT);
   for (int i = EEPROM_DATA_0; i < EEPROM_DATA_0 + 8; ++i) {
      pinMode(i, OUTPUT);
   }

   digitalWrite(SHIFT_DATA, LOW);
   digitalWrite(SHIFT_CLOCK, LOW);
   pinMode(SHIFT_DATA, OUTPUT);
   pinMode(SHIFT_CLOCK, OUTPUT);

   for (unsigned int i = 0; i < 4; ++i) {
      for (unsigned int j = 0; j < 512; ++j) {
         unsigned char data;
         unsigned short address;

         address = (i << 9) | j;
         writeEepromAddress(address, SHIFT_DATA, SHIFT_CLOCK);
         delay(EEPROM_WRITE_DELAY);

         data = getOutputByte(address);
         writeEepromByte(data, EEPROM_DATA_0, EEPROM_WE);
      }

      Serial.write('.');
   }

   for (unsigned int i = 2048; i < 8192; ++i) {
      writeEepromAddress(i, SHIFT_DATA, SHIFT_CLOCK);
      delay(EEPROM_WRITE_DELAY);
      writeEepromByte(0, EEPROM_DATA_0, EEPROM_WE);
      if (((i + 1) & 511) == 0) {
         Serial.write('.');
      }
   }

   delay(EEPROM_WRITE_DELAY * 10);
   Serial.write("\nFinished\n");

   dumpEeprom(0, 512);

   Serial.end();
}

void loop() {
   // put your main code here, to run repeatedly:

}
