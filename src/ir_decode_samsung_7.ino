#include <Arduino.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#define TIMER_RESET TCNT1 = 0
#define SAMPLE_SIZE 150

int IRpin = 11;
unsigned int TimerValue[3][SAMPLE_SIZE];
char direction[3][SAMPLE_SIZE];
byte change_count;
long time;
unsigned long curr_tcnt1 = 0;
unsigned long prev_tcnt1 = 0;
unsigned long ellapsed = 0;
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
  // reset everything
  change_count = 0;
  chunk = 0;
  memset(TimerValue, 0, sizeof(TimerValue));

  Serial.println("Waiting2...");

  while (digitalRead(IRpin) == HIGH) {
  }
  TIMER_RESET;
  TimerValue[chunk][change_count] = TCNT1;
  ellapsed = 0;
  int init_count = change_count++;
  int chunk_index;
  direction[0][init_count] = '0';
  direction[1][init_count] = '0';
  direction[2][init_count] = '0';

  while (chunk < 3) {
    //    while (change_count < SAMPLE_SIZE) {
    curr_tcnt1 = TCNT1;

    if (prev_tcnt1 > curr_tcnt1) {
      ellapsed = ellapsed + (65535 - prev_tcnt1) + curr_tcnt1; // rollover
    } else {
      //      Serial.print("\n<");
      ellapsed = ellapsed + (curr_tcnt1 - ellapsed);
    }

    //    if (ellapsed > 100000) {
    //      break;
    //    }

    if (curr_tcnt1 - prev_tcnt1 > 2000 && chunk <= 3) {
      chunk++;
      chunk_index = chunk - 1;
      change_count = init_count;
      Serial.println(chunk);
      //      Serial.println(chunk_index);
    }

    if (direction[chunk_index][change_count - 1] == '0') {
      prev_tcnt1 = TCNT1;
      while (digitalRead(IRpin) == LOW) {
        if (ellapsed + TCNT1 > 75000) {
          chunk++;
          //          break;
        }
        //        if (ellapsed + TCNT1 > 100000) {
        //          Serial.println("\ntimeout");
        //          Serial.println(ellapsed + TCNT1 );
        //          chunk = 4;
        //          break;
        //      }
      }
      TimerValue[chunk_index][change_count] = TCNT1 - prev_tcnt1;
      direction[chunk_index][change_count++] = '1';
    } else {
      prev_tcnt1 = TCNT1;
      while (digitalRead(IRpin) == HIGH) {
        if (ellapsed + TCNT1 > 75000) {
          chunk++;
          //          break;
        }
        //        if (ellapsed + TCNT1 > 100000) {
        //          Serial.println("\ntimeout");
        //          Serial.println(ellapsed + TCNT1 );
        //          chunk = 4;
        //          break;
        //      }
      }
      TimerValue[chunk_index][change_count] = TCNT1 - prev_tcnt1;
      direction[chunk_index][change_count++] = '0';
      //      }
    }
  }

  Serial.println("Bit stream detected!");
  chunk = 0;
  // WORKS
  byte cycle[7] = {0};
  int cycletime;
  int shifter;
  int byte_index = 0;
  change_count = 0;
  while (chunk < 3) {
    time = (long)TimerValue[chunk][change_count] * 4;
    //    time = (long) TimerValue[chunk][change_count];
    //    Serial.print(time);
    //    Serial.print("\t");
    //    Serial.println(direction[chunk][change_count++]);

    while (change_count < SAMPLE_SIZE) {
      Serial.print("\n cc:");
      Serial.print(change_count);
      cycletime =
          TimerValue[chunk][change_count] + TimerValue[chunk][change_count + 1];
      if (TimerValue[chunk][change_count] > 100 &&
          TimerValue[chunk][change_count] < 500) { // check for invalid timings
        byte_index = ((change_count + 14) / 16) - 1;
        shifter = (byte_index + 1) * 8 - (change_count / 2);
        //        shifter = 8 - ((change_count - 1) / 2);
        //        Serial.print("\t byte_index: ");
        //        Serial.print(byte_index);
        if (cycletime < 300) {
          Serial.print("\t decoded: ");
          Serial.print("1");
          cycle[byte_index] = cycle[byte_index] | (0x1 << shifter);
        } else {
          Serial.print("\t decoded: ");
          Serial.print("0");
          cycle[byte_index] = cycle[byte_index] | (0x0 << shifter);
        }
        Serial.print("\t shifted: ");
        Serial.print(shifter);
      }

      //            Serial.print("\t");
      //      Serial.print("0x");
      //      Serial.print(cycle[byte_index], HEX);
      Serial.print("\t");
      Serial.print(cycletime);
      //      Serial.print("\t");
      //            Serial.print((long) TimerValue[chunk][change_count]);
      //            Serial.print(" : ");
      //            Serial.print((long) TimerValue[chunk][change_count - 1]);
      //            Serial.println("\t");
      change_count++;
      //      Serial.print(direction[chunk][change_count]);
      change_count++; // odd check
    }
    Serial.print("\nFinal byte: 0x");
    for (int i = 0; i < sizeof(cycle); i++) {
      Serial.print(cycle[i], HEX);
      Serial.print(" ");
      cycle[i] = (char)0;
    }

    chunk++;
    change_count = 0;
  }

  Serial.println("\nBit stream end!");
  delay(2000);
}
