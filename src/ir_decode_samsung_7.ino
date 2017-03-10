#include <Arduino.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#define TIMER_RESET TCNT1 = 0
#define SAMPLE_SIZE 150
#define OFFSETS 500
#define TIMEOUT_CYCLES 100000

int IRpin = 11;
unsigned int TimerValue[3][SAMPLE_SIZE];
unsigned long Timings[OFFSETS];
char direction[3][SAMPLE_SIZE];
byte change_count;
long time;
unsigned long curr_tcnt1 = 0;
unsigned long prev_tcnt1 = 0;
unsigned long ellapsed = 0;
int chunk = 0;
bool first_bit = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Analyze IR Remote");
  TCCR1A = 0x00; // COM1A1=0, COM1A0=0 => Disconnect Pin OC1 from Timer/Counter
                 // 1 -- PWM11=0,PWM10=0 => PWM Operation disabled
  // ICNC1=0 => Capture Noise Canceler disabled -- ICES1=0 => Input Capture Edge
  // Select (not used) -- CTC1=0 => Clear Timer/Counter 1 on Compare/Match
  // CS12=0 CS11=1 CS10=1 => Set prescaler to clock/64
  TCCR1B = 0x03; // 16MHz clock with prescaler means TCNT1 increments every 4uS
  // ICIE1=0 => Timer/Counter 1, Input Capture Interrupt Enable -- OCIE1A=0 =>
  // Output Compare A Match Interrupt Enable -- OCIE1B=0 => Output Compare B
  // Match Interrupt Enable
  // TOIE1=0 => Timer 1 Overflow Interrupt Enable
  TIMSK1 = 0x00;
  pinMode(IRpin, INPUT);
}

void loop() {
  // reset everything
  change_count = 0;
  chunk = 0;
  memset(TimerValue, 0, sizeof(TimerValue));

  Serial.println("Waiting2...");

  TIMER_RESET;
  TimerValue[chunk][change_count] = TCNT1;
  ellapsed = 0;
  int init_count = change_count++;
  int chunk_index;

  // direction[0][init_count] = '0';
  // direction[1][init_count] = '0';
  // direction[2][init_count] = '0';

  unsigned int offset_index = 0;
  unsigned long start = micros();
  byte OldState = HIGH;
  byte NewState;
  unsigned long StartTime, DeltaTime, CurrentTime, PrevTime;
  bool Finished = false;
  // while (chunk < 4) {

  while (true) {

    // hold on HIGH
    while (digitalRead(IRpin) == HIGH && offset_index == 0) {
      // Serial.print("\nMicrosB4\t");
      // Serial.print(micros());
      StartTime = micros(); // start the counter
      OldState = LOW;
    }

    while (OldState == (NewState = digitalRead(IRpin))) {
      CurrentTime = micros();
      // Serial.print(micros());
      // if ((DeltaTime = (CurrentTime = micros()) - StartTime) > 100000) {
      if (CurrentTime - StartTime > 500000) {
        // Serial.print("\ntimeout");
        // Serial.print(CurrentTime - PrevTime);
        Finished = true;
        break;
      }
    }
    // Serial.print(" ");
    // Serial.print(DeltaTime);
    Timings[offset_index] = CurrentTime - PrevTime;

    PrevTime = CurrentTime;

    if (Finished)
      break;
    offset_index++;
    OldState = NewState;
  }

  Serial.print("\nMicrosDiff: ");
  Serial.print(micros() - StartTime);

  Serial.print("\nBit stream detected!\n");

  byte word[21] = {0};
  unsigned int bit_index, byte_index, shifter;
  bit_index = 0;
  byte_index = 0;
  for (size_t i = 0; i < OFFSETS; i++) {

    // if ((Timings[i] > 2500 && Timings[i] < 10000) ||
    //     (Timings[i + 1] > 2500 && Timings[i + 1] < 10000)) {
    //   Serial.print("\nhdr: \n");
    //   i + 2;
    // }

    if (bit_index == 8) {
      bit_index = 0;
      byte_index++;
    }
    // byte_index = ((bit_index + 7) / 8) - 1;
    shifter = 7 - bit_index;

    if (Timings[i] > 8000                        // start headers
        || Timings[i] + Timings[i + 1] > 2250) { // mid headers
      // Serial.print("\nhdr: \n");
      bit_index = 0;
      // detect mid headers
      if (Timings[i + 2] > 2000 && Timings[i + 3] > 7000) {
        // Serial.print("\nmhdr: \n");
      }
    }

    // 7+1*8 = 64 => (114+1/2) => 57
    // 64 - 57 = 7

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
    //
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
  for (int i = 0; i < sizeof(word); i++) {
    if (i == 0 || i == 7 || i == 14) {
      Serial.print("\nbyte: 0x");
    }
    Serial.print(word[i], HEX);
    Serial.print(" ");
  }

  Serial.print("\nBit stream end!");
  delay(2000);
}
