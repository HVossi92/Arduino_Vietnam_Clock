
#include <Wire.h>
#include "ds3231.h"

#define BUFF_MAX 128

// Board pins
int latchPin = 5;
int clockPin = 6;
int dataPin = 4;

// Buttons on Clock
int buttonApin = 10; // Turn on
int buttonBpin = 9; // Turn off
int buttonCpin = 8; // Blinking

int numOfRegisters = 3;
byte* registerState;

bool clockFlag = false;

// how often to refresh the info on stdout (ms)
unsigned long prev = 5000, interval = 5000;

void setup() {
  //Initialize array of registries
  registerState = new byte[numOfRegisters];
  for (size_t i = 0; i < numOfRegisters; i++) {
    registerState[i] = 0;
  }

  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  // Set buttons
  pinMode(buttonApin, INPUT_PULLUP);
  pinMode(buttonBpin, INPUT_PULLUP);
  pinMode(buttonCpin, INPUT_PULLUP);

  // Turn off Leds
  for (int i = 0; i < 24; i++) {
    regWrite(i, LOW);
  }

  // Clock Setup
  Serial.begin(9600);
  Wire.begin();
  DS3231_init(DS3231_CONTROL_INTCN);
  DS3231_clear_a1f();
}

void loop() {

  if (digitalRead(buttonApin) == LOW)
  {
    buttonA();
  }

  if (digitalRead(buttonBpin) == LOW)
  {
    buttonB();
  }

  if (digitalRead(buttonCpin) == LOW)
  {
    buttonC();
  }

  if (clockFlag)
  {
    clockLoop();
  }
}

void buttonA() {
  clockFlag = false;

  for (int i = 0; i < 24; i++) {
    regWrite(i, HIGH);
    delay(100);
  }
}

void buttonB() {
  clockFlag = false;

  for (int i = 0; i < 24; i++) {
    regWrite(i, LOW);
    delay(25);
  }
}

void buttonC() {
  for (int i = 0; i < 24; i++)
  {
    regWrite(i, LOW);
  }

  if (!clockFlag)
  {
    clockFlag = true;
  }
  else
  {
    clockFlag = false;
  }
}

void regWrite(int pin, bool state) {
  //Determines register
  int reg = pin / 8;
  //Determines pin for actual register
  int actualPin = pin - (8 * reg);
  //Begin session
  digitalWrite(latchPin, LOW);

  for (int i = 0; i < numOfRegisters; i++) {
    //Get actual states for register
    byte* states = &registerState[i];

    //Update state
    if (i == reg) {
      bitWrite(*states, actualPin, state);
    }

    //Write
    shiftOut(dataPin, clockPin, MSBFIRST, *states);
  }

  //End session
  digitalWrite(latchPin, HIGH);
}

void clockLoop()
{
  char buff[BUFF_MAX];
  unsigned long now = millis();
  struct ts t;
  // once a while show what is going on
  if ((now - prev > interval) && (Serial.available() <= 0)) {
    DS3231_get(&t);

    // display current time
    snprintf(buff, BUFF_MAX, "%02d:%02d:%02d", t.hour, t.min, t.sec);
    Serial.println(buff);

    // display a1 debug info
    DS3231_get_a1(&buff[0], 59);
    Serial.println(buff);

    if (DS3231_triggered_a1()) {
      // INT has been pulled low
      Serial.println(" -> alarm1 has been triggered");
      // clear a1 alarm flag and let INT go into hi-z
      DS3231_clear_a1f();
    }
    prev = now;
  }
  
  // To LEDs hours __________________________________________________________________
  // Erst mit modulo 24 auf 12Std., dann + 12 zur ersten Roten LED
  int hour = (t.hour % 12) + 13;
  for (int i = 13; i < 24; i++) {
    regWrite(i, LOW);
  }
  regWrite(hour, HIGH);  

  // To LEDs minutes
  int minute = ((t.min % 10) + 2);
  for (int i = 2; i < 13; i++) {
    regWrite(i, LOW);
  }
  regWrite(minute, HIGH);
}
