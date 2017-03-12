#include <Arduino.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#define OFFSETS 500

int IRpin = 11;
unsigned long Timings[OFFSETS];

void setup() {
  Serial.begin(115200);
  Serial.println("Analyze IR Remote");
  pinMode(IRpin, INPUT);
}

void loop() {
  Serial.println("Waiting2...");

  unsigned int offset_index = 0;
  byte OldState = HIGH; // hi logic while ir receiver sleeps
  byte NewState;
  unsigned long StartTime, CurrentTime, PrevTime;
  bool Finished = false;
  while (true) {

    // hold first on HIGH
    while (digitalRead(IRpin) == HIGH && offset_index == 0) {
      // Serial.print("\nMicrosB4\t");
      // Serial.print(micros());
      StartTime = micros(); // start the counter
      OldState = LOW;
    }

    while (OldState == (NewState = digitalRead(IRpin))) {
      CurrentTime = micros();
      if (CurrentTime - StartTime > 500000) {
        Finished = true;
        break;
      }
    }

    Timings[offset_index] = CurrentTime - PrevTime; // write the delta t to
                                                    // array

    PrevTime = CurrentTime;

    if (Finished)
      break;
    offset_index++;
    OldState = NewState;
  }

  Serial.print("\nMicros Diff: ");
  Serial.print(micros() - StartTime);

  Serial.print("\nBit stream detected!\n");

  byte word[21] = {0}; // samsung AC sends 3 chunks of 7 bytes each
  unsigned int bit_index, byte_index, shifter;
  bit_index = 0;
  byte_index = 0;
  for (size_t i = 0; i < OFFSETS; i++) {
    if (bit_index == 8) {
      bit_index = 0;
      byte_index++;
    }
    shifter = 7 - bit_index;

    if (Timings[i] > 8000 ||
        Timings[i] + Timings[i + 1] > 2250) { // start headers
      // Serial.print("\nhdr: \n");
      bit_index = 0;
      // detect mid headers
      if (Timings[i + 2] > 2000 && Timings[i + 3] > 7000) {
        // Serial.print("\nmidhdr: \n");
      }
    }

    // tons of serial debug stuff
    // Serial.print("\n");
    // Serial.print(i);
    // Serial.print("i= ");
    // Serial.print(Timings[i]);
    // Serial.print("+");
    // Serial.print(Timings[i + 1]);
    // Serial.print("us");
    // Serial.print(byte_index);
    // Serial.print("<<");
    // Serial.print(shifter);
    // Serial.print(": \t bit_index:");
    // Serial.print(bit_index);
    // Serial.print("\t bit: ");

    if (Timings[i] + Timings[i + 1] < 2500 &&
        Timings[i] + Timings[i + 1] > 1200) {
      // Serial.print("0");
      word[byte_index] = word[byte_index] | (0x0 << shifter);
      bit_index++;
    }
    if (Timings[i] + Timings[i + 1] > 800 &&
        Timings[i] + Timings[i + 1] < 1200) {
      // Serial.print("1");
      word[byte_index] = word[byte_index] | (0x1 << shifter);
      bit_index++;
    }
    i++;
  };
  for (size_t i = 0; i < sizeof(word); i++) {
    if (i == 0 || i == 7 || i == 14) {
      Serial.print("\nbyte: 0x");
    }
    Serial.print(word[i], HEX);
    Serial.print(" ");
  }

  Serial.print("\nBit stream end!");
  delay(2000);
}
