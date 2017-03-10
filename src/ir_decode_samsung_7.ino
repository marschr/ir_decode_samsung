
#include <avr/interrupt.h>
#include <avr/io.h>

#define TIMER_RESET TCNT1 = 0
#define SAMPLE_SIZE 128

int IRpin = 11;
unsigned int TimerValue[3][SAMPLE_SIZE];
char direction[3][SAMPLE_SIZE];
byte change_count;
long time;
long curr_tcnt1 = 0;
long prev_tcnt1 = 0;
int chunk = 0;

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
  TimerValue[chunk][change_count] = TCNT1;
  int init_count = change_count++;
  int chunk_index;
  direction[0][init_count] = '0';
  direction[1][init_count] = '0';
  direction[2][init_count] = '0';

  while (chunk <= 3) {
    //    while (change_count < SAMPLE_SIZE) {
    curr_tcnt1 = TCNT1;

    if (curr_tcnt1 - prev_tcnt1 > 2000) {
      Serial.println(curr_tcnt1 - prev_tcnt1);

      chunk++;
      chunk_index = chunk - 1;
      change_count = init_count;
      Serial.println(chunk);
      Serial.println(chunk_index);
    }

    if (direction[chunk_index][change_count - 1] == '0') {
      prev_tcnt1 = TCNT1;
      while (digitalRead(IRpin) == LOW) {
      }
      TimerValue[chunk_index][change_count] = TCNT1 - prev_tcnt1;
      direction[chunk_index][change_count++] = '1';
    } else {
      prev_tcnt1 = TCNT1;
      while (digitalRead(IRpin) == HIGH) {
      }
      TimerValue[chunk_index][change_count] = TCNT1 - prev_tcnt1;
      direction[chunk_index][change_count++] = '0';
      //      }
    }
  }

  Serial.println("Bit stream detected!");
  chunk = 0;

  while (chunk < 3) {
    time = (long)TimerValue[chunk][change_count] * 4;
    Serial.print(time);
    Serial.print("\t");
    Serial.println(direction[chunk][change_count++]);
    while (change_count < SAMPLE_SIZE) {
      time = (long)TimerValue[chunk][change_count] * 4;
      //   Serial.print(time);
      //   Serial.print("\t");
      //   Serial.println(direction[change_count-1]);
      Serial.print(time);
      Serial.print("\t");
      Serial.println(direction[chunk][change_count++]);
    }
    chunk++;
    change_count = 0;
  }

  Serial.println("Bit stream end!");
  delay(2000);
}
