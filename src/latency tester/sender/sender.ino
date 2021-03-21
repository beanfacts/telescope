//#include <Keyboard.h>
//#include <Mouse.h>
#include <Arduino.h>


const char KeyboardReceivePin = 2; // pin used to interrupt the timer
const char MouseReceivePin = 3;
const char ButtonPin = 9;
unsigned long MouseCurrentTime, MouseElapsedTime, MouseStartTime, KeyboardCurrentTime, KeyboardElapsedTime, KeyboardStartTime;
void setup() {
    // start serial at the highest possible baud rate
    Serial.begin(2000000);

    // initialise the keyboard and Mouse emulator
    // Keyboard.begin();
    // Mouse.begin();

    // define input pins
    pinMode(ButtonPin, INPUT_PULLUP);
    pinMode(KeyboardReceivePin, INPUT_PULLUP);
    pinMode(MouseReceivePin, INPUT_PULLUP);

    // attach the interupt to mouse signal and keyboard signal
    attachInterrupt(digitalPinToInterrupt(KeyboardReceivePin), get_time_keyboard, RISING);
    attachInterrupt(digitalPinToInterrupt(MouseReceivePin), get_time_mouse, RISING);

    // temp output for reaction
    pinMode(10,OUTPUT);
    pinMode(11,OUTPUT);

}

void loop() {
    if (!digitalRead(ButtonPin)){
        //delay to stop repeated calls
        delay(500);
        Serial.println("Start Keyboard Latency Test");
        start_test_keyboard();
        // delay between test case
        delay(100);
        Serial.println("Start Mouse Latency Test");
        start_test_mouse();
    }
}

// Keyboard Test Case
void start_test_keyboard(){
    KeyboardStartTime = millis();
    // Keyboard.write("A");
    // temp
    digitalWrite(10,HIGH);
}

void get_time_keyboard(){
    KeyboardCurrentTime = millis();
    KeyboardElapsedTime = KeyboardCurrentTime - KeyboardStartTime;
    Serial.print("Time Taken: ");
    Serial.print(KeyboardElapsedTime);
    Serial.println("ms");
    // temp
    digitalWrite(10,LOW);
}

// Mouse Test Case
void start_test_mouse(){
    MouseStartTime = millis();
    // Mouse.click()
    // temp
    digitalWrite(11,HIGH);
}

void get_time_mouse(){
    MouseCurrentTime = millis();
    MouseElapsedTime = MouseCurrentTime - MouseStartTime;
    Serial.print("Time Taken: ");
    Serial.print(MouseElapsedTime);
    Serial.println("ms");
    // temp
    digitalWrite(11,LOW);
}
