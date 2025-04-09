
#include <Arduino.h>
#include <U8g2lib.h>
#include "HX711.h"
#include <MIDI.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h> // On an arduino UNO: A4(SDA), A5(SCL)
#endif

#define NUM_SCALES 4 

HX711 scale[NUM_SCALES];
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
MIDI_CREATE_DEFAULT_INSTANCE();

//pins for scale 
int8_t dataPins[NUM_SCALES] = {2,4,6,8};
int8_t clockPins[NUM_SCALES] = {3,5,7,9};

//button pins
const int buttonPins[3] = {A1,A2,A3}; 

//pot pin
const int potPin = A0; 

// Output arduino pins
const int arduinoComPin1 = 10;
const int arduinoComPin2 = 11;
const int arduinoComPin3 = 12;

// Oled standard by wire library: A4(SDA), A5(SCL)

//everything for debounce logic 
#define debounceTime 50  // Debounce time in milliseconds
volatile bool buttonState[3] = {0, 0, 0}; // Array for current button states
unsigned long lastDebounceTime[3] = {0, 0, 0}; // Array to store last debounce times
volatile bool lastButtonState[3] = {0, 0, 0}; // array to store last button state

// everything for to look the weight differences
#define weightThreshold 100
volatile int currentWeight[NUM_SCALES] = {0,0,0,0};
volatile int previousWeight[NUM_SCALES] = {0,0,0,0};

//for screen
int genreSelection = 1; 
bool lock = false;
unsigned long volumeTime = 0;
#define maxVolumeTime 800

//volume 
int potValue = 0; // a value between 0-1023
int oledVolume = 0; // a value between 0-10 
int oldVolume = 0; 
const int offset[NUM_SCALES] = { -478319, 654035, 437674, -502097 };
const float scaleSet[NUM_SCALES] = { 430.580902, 429.952209 , 428.897033, 394.222412 };

//delay for weighting
#define weightTime 1000
unsigned long prevWeightTime = 0;

int sfxNoteDuration = 100;

//data transfer 
unsigned long stopSignalTime = 0;
int signalTime = 500;
int signalData = 0;

void setup(void) {
  Serial.begin(9600);
  u8g2.begin();

  for (int i = 0; i < NUM_SCALES; i++) {
    scale[i].begin(dataPins[i], clockPins[i]); // Equivalent to scale1.begin(), scale2.begin(), etc.
    scale[i].set_offset(offset[i]); 
    scale[i].set_scale(scaleSet[i]);
  }

  //setup the pins for the buttons 
  for (int i = 0; i < 3; i++) {
    pinMode(buttonPins[i], INPUT);
  }

  pinMode(potPin, INPUT);
  
  pinMode(arduinoComPin1, OUTPUT);
  pinMode(arduinoComPin2, OUTPUT);
  pinMode(arduinoComPin3, OUTPUT);
}

void loop(void) {
  checkButtons();
  readPot();
  updateDisplay();
  readScales();
}



void checkButtons (void) { 
  //debounce and read for three buttons
  for (int i = 0; i < 3; i++) { 
    int reading = digitalRead(buttonPins[i]);
    
    if (reading != lastButtonState[i]) {
      lastDebounceTime[i] = millis();
    }
    if ((millis() - lastDebounceTime[i]) > debounceTime) {
      if (reading != buttonState[i]) {
        buttonState [i] = reading;
        //check if it is button 1 (up) or button 2 (down) or button 3 (lock) and respond accordingly
        if (buttonState[i] == 1){
          if (i == 1 && genreSelection < 3){
            genreSelection++;
          }
          if (i == 2 && genreSelection > 1){
            genreSelection--;
          }
          if (i == 0){
            lock = !lock;
          }
        }
      }
    }
    lastButtonState[i] = reading;
  }
}

void readPot (void) {
  //read the pot value and map the analog value to an volume value between 0-10
  potValue = analogRead(potPin);
  oledVolume = map(potValue, 0, 1023, 0, 10);
}

void updateDisplay (void) {
  u8g2.clearBuffer();
  u8g2.firstPage();
  //place the cursor for the arrow according to the selected genre 
  do {
    if (genreSelection == 1){
      u8g2.setCursor(0, 25);
    }
    if (genreSelection == 2){
      u8g2.setCursor(0, 45);
    }
    if (genreSelection == 3){
      u8g2.setCursor(0, 65);
    }

    u8g2.setFont(u8g2_font_open_iconic_arrow_2x_t);  // Set an arrow icon font
    u8g2.print("\x0042");  // Print an arrow 

    if (lock == true){
      // Set an locked lock
      u8g2.setFont(u8g2_font_open_iconic_thing_2x_t);  
      u8g2.setCursor(110,65);
      u8g2.print("\x004F");
    }

    displayConstants();
  } while ( u8g2.nextPage() );
}

void displayConstants(void){ 
  //show the three genres on screen
  u8g2.setFont(u8g2_font_fub11_tr);
  u8g2.setCursor(20, 20);
  u8g2.print(F("1. Pop"));

  u8g2.setCursor(20, 40);
  u8g2.print(F("2. Rock"));
  
  u8g2.setCursor(20, 60);
  u8g2.print(F("3. Country"));


  if (oledVolume != oldVolume) {
    volumeTime = millis();  // Store the time when volume was changed
    oldVolume = oledVolume;  // Update old volume to avoid unnecessary updates
  }

  // Only display volume if within the allowed time window
  if (millis() - volumeTime < maxVolumeTime) {
    u8g2.setCursor(98, 20);
    u8g2.print(oledVolume);
    u8g2.setFont(u8g2_font_open_iconic_play_2x_t);  
    u8g2.setCursor(80, 23);
    u8g2.print("\x004F");
  }
}

void readScales(void){
  //check for each scale if their current value is x grams higher or lower than the previous weight
  if (lock == false){ // is the lock on or not?
    if (millis() - prevWeightTime > weightTime) {
      for (int i = 0; i < NUM_SCALES; i++) {
        currentWeight[i] = scale[i].get_units(1); 

        if (currentWeight[i] > previousWeight[i] + weightThreshold) {
          previousWeight[i] = currentWeight[i];
          stopSignalTime = millis() + signalTime; // start the timer for how long the singal is send 

          // i = number of the scale
          if (i == 0) { // add chords - 2
            signalData=2;
          }

          if (i==1) { // add melody - 4
            signalData=4;
          }

          if (i==2) { //add drums - 6
            signalData=6;
          }

          if (i == 3) { // play sfx - 1
            signalData=1;
          }
        }

        if (currentWeight[i] < previousWeight[i] - weightThreshold) {
          previousWeight[i] = currentWeight[i];
          stopSignalTime = millis() + signalTime; // start the timer for how long the singal is send 

          if (i == 0) { // remove chords - 3
            signalData=3;     
          }

          if (i==1) { // remove melody - 5
            signalData=5;
          }
          if (i==2) { // remove drums - 7 
            signalData=7;
          }
        }
      }
      prevWeightTime = millis();
      sentSignal();
    }
  }
}

void sentSignal(){
  // convert the value to bits and sent them for x amount of ms 
  if (millis() < stopSignalTime) {

    if (signalData==1) {
      digitalWrite(arduinoComPin1, HIGH);
      digitalWrite(arduinoComPin2, LOW);
      digitalWrite(arduinoComPin3, LOW);
      Serial.println("1");
    }
    else if (signalData==2) {
      digitalWrite(arduinoComPin1, LOW);
      digitalWrite(arduinoComPin2, HIGH);
      digitalWrite(arduinoComPin3, LOW);
      Serial.println("2");
    }
    else if (signalData==3) {
      digitalWrite(arduinoComPin1, HIGH);
      digitalWrite(arduinoComPin2, HIGH);
      digitalWrite(arduinoComPin3, LOW);
      Serial.println("3");
    }
    else if (signalData==4) {
      digitalWrite(arduinoComPin1, LOW);
      digitalWrite(arduinoComPin2, LOW);
      digitalWrite(arduinoComPin3, HIGH);
      Serial.println("4");
    }
    else if (signalData==5) {
      digitalWrite(arduinoComPin1, HIGH);
      digitalWrite(arduinoComPin2, LOW);
      digitalWrite(arduinoComPin3, HIGH);
      Serial.println("5");
    }
    else if (signalData==6) {
      digitalWrite(arduinoComPin1, LOW);
      digitalWrite(arduinoComPin2, HIGH);
      digitalWrite(arduinoComPin3, HIGH);
      Serial.println("6");
    }
    else if (signalData==7) {
      digitalWrite(arduinoComPin1, HIGH);
      digitalWrite(arduinoComPin2, HIGH);
      digitalWrite(arduinoComPin3, HIGH);
      Serial.println("7");
    }
  }

  if (millis() > stopSignalTime) {
    digitalWrite(arduinoComPin1, LOW);
    digitalWrite(arduinoComPin2, LOW);
    digitalWrite(arduinoComPin3, LOW);
    Serial.println("sending nothing");
  }
}
