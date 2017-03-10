
#include <avr/interrupt.h>
#include <avr/io.h>

#define TIMER_RESET TCNT1 = 0
#define SAMPLE_SIZE 255

int IRpin = 11;
unsigned int Chunk1[SAMPLE_SIZE];
unsigned int Chunk2[SAMPLE_SIZE];
unsigned int Chunk3[SAMPLE_SIZE];
char dir1[SAMPLE_SIZE];
char dir2[SAMPLE_SIZE];
char dir3[SAMPLE_SIZE];
byte change_count;
byte current_chunk = 0;
long time;
int currtcnt1 = 0;
int prevtcnt1 = 0;

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
  Serial.println("Waiting...");
  change_count = 0;
  while (digitalRead(IRpin) == HIGH) {
  }
  TIMER_RESET;
  Chunk1[change_count] = TCNT1;
  dir1[1] = '0';
  dir2[1] = '0';
  dir3[1] = '0';

  while (change_count < SAMPLE_SIZE && current_chunk <= 3) {
    currtcnt1 = TCNT1;
    //   Serial.println(currtcnt1 *4);
    //   Serial.println(prevtcnt1 *4);

    if (currtcnt1 - prevtcnt1 > 2000) {
      Serial.println((currtcnt1 - prevtcnt1));
      change_count = 1;
      current_chunk++;
    }
    //
    if (current_chunk == 1) {
      if (dir1[change_count - 1] == '0') {
        prevtcnt1 = TCNT1;
        while (digitalRead(IRpin) == LOW) {
        }
        Chunk1[change_count] = TCNT1 - prevtcnt1;
        dir1[change_count++] = '1';
      } else {
        prevtcnt1 = TCNT1;
        while (digitalRead(IRpin) == HIGH) {
        }
        Chunk1[change_count] = TCNT1 - prevtcnt1;
        dir1[change_count++] = '0';
      }
    }

    if (current_chunk == 2) {
      if (dir2[change_count - 1] == '0') {
        prevtcnt1 = TCNT1;
        while (digitalRead(IRpin) == LOW) {
        }
        Chunk2[change_count] = TCNT1 - prevtcnt1;
        dir2[change_count++] = '1';
      } else {
        prevtcnt1 = TCNT1;
        while (digitalRead(IRpin) == HIGH) {
        }
        Chunk2[change_count] = TCNT1 - prevtcnt1;
        dir2[change_count++] = '0';
      }
    }

    if (current_chunk == 3) {
      if (dir3[change_count - 1] == '0') {
        prevtcnt1 = TCNT1;
        while (digitalRead(IRpin) == LOW) {
        }
        Chunk3[change_count] = TCNT1 - prevtcnt1;
        dir3[change_count++] = '1';
      } else {
        prevtcnt1 = TCNT1;
        while (digitalRead(IRpin) == HIGH) {
        }
        Chunk3[change_count] = TCNT1 - prevtcnt1;
        dir3[change_count++] = '0';
      }
    }
  }
  Serial.println("Bit stream detected!");
  change_count = 0;
  time = (long)Chunk1[change_count];
  Serial.print(time);
  Serial.print("\t");
  Serial.println(dir1[change_count++]);
  while (change_count < SAMPLE_SIZE) {
    time = (long)Chunk1[change_count];
    Serial.print(time);
    Serial.print("\t");
    Serial.println(dir1[change_count++]);
  }

  change_count = 0;
  time = (long)Chunk2[change_count] * 4;
  Serial.print(time);
  Serial.print("\t");
  Serial.println(dir2[change_count++]);
  while (change_count < SAMPLE_SIZE) {
    time = (long)Chunk2[change_count] * 4;
    Serial.print(time);
    Serial.print("\t");
    Serial.println(dir2[change_count++]);
  }

  change_count = 0;
  time = (long)Chunk3[change_count] * 4;
  Serial.print(time);
  Serial.print("\t");
  Serial.println(dir3[change_count++]);
  while (change_count < SAMPLE_SIZE) {
    time = (long)Chunk3[change_count] * 4;
    Serial.print(time);
    Serial.print("\t");
    Serial.println(dir3[change_count++]);
  }
  Serial.println("Bit stream end!");
  delay(2000);
}
