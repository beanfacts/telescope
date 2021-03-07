#include <Arduino.h>
const char InPinKeyboard = 13;
const char OutPinKeyboard = 12;
const char InPinMouse = 10;
const char OutPinMouse = 11;
bool state = false;
void setup() {
    pinMode(InPinKeyboard,INPUT);
    pinMode(OutPinKeyboard,OUTPUT);
    pinMode(InPinMouse,INPUT);
    pinMode(OutPinMouse,OUTPUT);

}

void loop() {
    // temporary code to test the arduino
  if(digitalRead(InPinKeyboard)){
    delay(100);
    state = !state;
    digitalWrite(OutPinKeyboard,state);
  }
  if(digitalRead(InPinMouse)){
    delay(80);
    state = !state;
    digitalWrite(OutPinMouse,state);
  }

}
