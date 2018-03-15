#define SHIFT_DATA 2
#define SHIFT_CLOCK 3
#define EEPROM_WE 4  // active low
#define EEPROM_OE 13 // active low
#define EEPROM_DATA_0 5

static unsigned char getOutputByte(unsigned short address)
{
   static const unsigned char digits[16] = { // common anode
      0x88, 0xeb, 0x4c, 0x49, 0x2b, 0x19, 0x18, 0xcb,
      0x08, 0x09, 0x0a, 0x38, 0x9c, 0x68, 0x1c, 0x1e
   };
   static const unsigned char off = 0xff; // common anode

   unsigned char data;
   unsigned int digit = (address >> 12) & 3;

   if (address > 077777) {
      return off;
   }

   if (address < 040000) {
      //octal
      if (digit == 0) {
         data = digits[address & 7];          // digit 0
      } else if (digit == 1) {
         data = digits[(address >> 3) & 7];   // digit 1
      } else if (digit == 2) {
         data = digits[(address >> 6) & 7];   // digit 2
      } else {
         data = digits[(address >> 9) & 7];   // digit 3
      }
   } else {
      unsigned char sign = 0;
      unsigned int value = (unsigned int)(((int)address << 4) >> 4);

      if (value & 0x8000) {
         // negative
         value = ~value + 1;
         sign = 0x08;
      }

      // signed decimal
      if (digit == 0) {
         data = digits[value % 10] ^ sign;   // digit 0
      } else if (digit == 1) {
         data = value >= 10 ? digits[(value / 10) % 10] : off;   // digit 1
      } else if (digit == 2) {
         data = value >= 100 ? digits[(value / 100) % 10] : off;  // digit 2
      } else {
         data = value >= 1000 ? digits[(value / 1000) % 10] : off; // digit 3
      }
   }

   return data;
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

   for (int i = 0; i < 8; ++i) {
      pinMode(EEPROM_DATA_0 + i, INPUT);
   }
   digitalWrite(EEPROM_OE, LOW);

   for (unsigned int i = begin; i < end; ++i) {
      unsigned char data = 0;

      writeEepromAddress(i, SHIFT_DATA, SHIFT_CLOCK);

      for (int k = 0; k < 8; ++k) {
         data |= (digitalRead(EEPROM_DATA_0 + k) == HIGH ? 1 : 0) << k;
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
   for (int i = 0; i < 8; ++i) {
      pinMode(EEPROM_DATA_0 + i, OUTPUT);
   }

   digitalWrite(SHIFT_DATA, LOW);
   digitalWrite(SHIFT_CLOCK, LOW);
   pinMode(SHIFT_DATA, OUTPUT);
   pinMode(SHIFT_CLOCK, OUTPUT);

   for (unsigned int i = 0; i < 8; ++i) {
      for (unsigned int j = 0; j < 4096; ++j) {
         unsigned char data;
         unsigned short address;

         address = (i << 12) | j;
         writeEepromAddress(address, SHIFT_DATA, SHIFT_CLOCK);
         delay(10);

         data = getOutputByte(address);
         writeEepromByte(data, EEPROM_DATA_0, EEPROM_WE);
      }

      Serial.write('.');
   }

   delay(10);
   Serial.write("\nFinished\n");

   dumpEeprom(0, 4096);
   Serial.end();
}

void loop() {
   // put your main code here, to run repeatedly:

}
