#include <MIDI.h>
#include <FastLED.h>

MIDI_CREATE_DEFAULT_INSTANCE();

#define DATA_PIN 5
#define NUM_LEDS 16

CRGB leds[NUM_LEDS];

const byte ledPin = 13;    // the number of the LED pin

// Other arduino pins
const byte arduinoComPin1 = 2;
const byte arduinoComPin2 = 3;
const byte arduinoComPin3 = 4;

const byte bpm = 120;
const int quarterNoteDuration = 60000 / bpm;
const int halfNoteDuration = 2 * quarterNoteDuration;
const int timePer16th = quarterNoteDuration / 4;
const int measureTime = 4 * quarterNoteDuration;
const int twoMeasureTime = 2 * measureTime; // Color changes every 2 measures (8 beats)
const int chordLoopTime = 4 * measureTime;

//pot pin
const int potPin = A1;

const byte KICK_NOTE = 36;   // C1
const byte SNARE_NOTE = 38;  // D1
const byte HIHAT_NOTE = 42;  // F#1 (Closed Hi-Hat)

const byte CLAP_NOTE = 39;
const byte TOM_NOTE = 45;

const byte COWBELL_NOTE = 56;
const byte RIMCLICK_NOTE = 37;
const byte SHAKER_NOTE = 70;

const byte C_PENTATONIC[5] = {60, 62, 64, 67, 69};

const byte C_MAJOR[2] = {60, 64};
const byte D_MINOR[2] = {62, 65};
const byte E_MINOR[2] = {64, 67};
const byte F_MAJOR[2] = {65, 69};
const byte G_MAJOR[2] = {67, 71};
const byte A_MINOR[2] = {69, 72};

const byte kickPatterns[3][16] = {
  {1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0},  // Steady Kick
  {1,0,0,0, 1,0,1,0, 1,1,0,0, 1,0,0,0},  // Groovy Kick
  {1,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0}   // Sparse Kick
};
const byte snarePatterns[4][16] = {
  {0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},  // Basic Snare
  {0,0,0,0, 1,0,0,0, 0,0,1,0, 1,0,0,0},  // Funky Snare
  {0,1,0,0, 1,0,0,0, 0,1,0,0, 1,0,0,0},  // Ghost Note Snare
  {0,0,0,0, 1,0,0,0, 0,1,1,0, 1,0,0,0}   // Breakbeat Snare
};
const byte hihatPatterns[4][16] = {
  {1,0,1,0, 1,0,1,0, 1,0,1,0, 1,0,1,0},  // Straight 8ths
  {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1},  // 16th Groove
  {1,0,0,1, 1,0,0,1, 1,0,0,1, 1,0,0,1},  // Shuffle
  {1,0,0,1, 0,1,0,1, 1,0,0,1, 0,1,0,1}   // Open Hat Groove
};
const byte clapPatterns[3][16] = {
  {0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},  // Basic Offbeat Clap
  {0,0,0,0, 1,0,0,1, 0,0,0,0, 1,0,1,0},  // Funky Double Clap
  {0,1,0,0, 1,0,0,0, 0,1,0,0, 1,0,0,0}   // Upbeat Groove
};
const byte tomPatterns[3][16] = {
  {0,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,1},  // Basic Tom Groove
  {0,0,0,1, 0,1,0,0, 0,0,1,0, 0,1,0,0},  // Rolling Toms
  {1,0,0,1, 0,1,0,1, 1,0,0,1, 0,1,0,1}   // Tribal Groove
};
const byte cowbellPattern[16] = {1,0,0,1, 0,1,0,1, 1,0,0,1, 0,1,0,1};
const byte rimclickPattern[16] = {0,1,0,0, 1,0,0,0, 0,1,0,0, 1,0,0,0};
const byte shakerPattern[16] = {1,1,0,1, 1,1,0,1, 1,1,0,1, 1,1,0,1};

const byte chordPatterns[4][3][8] = {{
  // Bounce between base
  {0, 0, 1, -1, 0, 0, 1, -1}, // C
  {0, 0, 1, -1, 0, 0, 1, -1}, // E
  {0, 0, 1, -1, 0, 0, 1, -1} // G
},
{ // Normal arpeggio
  {1, -1, 0, 0, 1, -1, 0, 0}, // C
  {-1, 1, -1, 1, -1, 1, -1, 1}, // E
  {0, 0, 1, -1, 0, 0, 1, -1} // G
},
{
  {1, -1, 0, 0, 1, -1, 0, 0}, // C
  {0, 0, 1, -1, 0, 0, 1, -1}, // E
  {-1, 1, -1, 1, -1, 1, -1, 1} // G
},
{
  {1, 0, -1, 0, 1, 0, -1, 0}, // C
  {-1, 0, 1, 0, -1, 0, 1, 0}, // E
  {-1, 0, 1, 0, -1, 0, 1, 0} // G
}};

int selectedChordPattern = 0;

int lastDrumInstrument = 0;

byte sfxMelody[9] = { 60, 64, 67, 62, 66, 69, 64, 68, 71 };
byte currentSFXstep = 0;
unsigned long playSFXNoteTime = 0;

byte melody[32];
byte lastNotePlayed;
byte locationsOfAddedNotes[32];

unsigned long currentTime = 0;

int startTime = 0; // The time the beat started playing
unsigned long prevTime = 0;
byte drumLoop[6][16];

int drumActivationLevel = 0;

int chordActivationLevel = 0;
int chordProgression[4][2];
int progression[4] = {0, 0, 0, 0};
int bassPattern = 0;

int potValue = 0; // a value between 0-1023
int oledVolume = 0; // a value between 0-10 
int oldVolume = 0; 
int volume = 0; // a value between 0-127

int sfxNoteDuration = 100;

byte lastArduinoCommand = 0;

// Colors changing every 2 measures (8 beats)
CRGB measureColors[] = {CRGB::Red, CRGB::Blue, CRGB::Green, CRGB::Purple};

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  //pinMode(BUTTON_PIN, INPUT_PULLUP); // Button set as input with pull-up resistor
  startTime = millis();

  // This is probably not neccessary
  Serial.begin(9600);

  MIDI.begin(MIDI_CHANNEL_OMNI); 

  /* Channel Mapping:
  1: Melody
  2: Chords
  3: SFX
  10: Rhythm
  */
  MIDI.sendProgramChange(27, 1); // Set the guitar on channel 1
  MIDI.sendProgramChange(1, 2); // Set a piano on channel 2


  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);

  pinMode(arduinoComPin1, INPUT);
  pinMode(arduinoComPin2, INPUT);
  pinMode(arduinoComPin3, INPUT);

  // volume
  pinMode(potPin, INPUT);

  // Base the random numbers of of a random reading from analog port 0 (DON'T CONNECT ANYTHING TO IT)
  randomSeed(analogRead(0));
}

void loop() {
  currentTime = millis();

  loopLeds();

  readPot();

  loopSFX();

  loopDrums();

  loopChords();

  loopMelody();
  
  // Receive controls from other arduino
  receiveInputs();

  prevTime = currentTime;
}

void receiveInputs() {
  byte arduinoCommand = 0;

  if (digitalRead(arduinoComPin1) == HIGH) {
    arduinoCommand += 1;
  }
  if (digitalRead(arduinoComPin2) == HIGH) {
    arduinoCommand += 2;
  }
  if (digitalRead(arduinoComPin3) == HIGH) {
    arduinoCommand += 4;
  }

  if ((arduinoCommand == 1) && (lastArduinoCommand == 0)) {
    playSFX();
  }
  if ((arduinoCommand == 2) && (lastArduinoCommand == 0)) {
    addChords();
  }
  if ((arduinoCommand == 3) && (lastArduinoCommand == 0)) {
    removeChords();
  }
  if ((arduinoCommand == 4) && (lastArduinoCommand == 0)) {
    addMelody();
  }
  if ((arduinoCommand == 5) && (lastArduinoCommand == 0)) {
    removeMelody();
  }
  if ((arduinoCommand == 6) && (lastArduinoCommand == 0)) {
    addDrums();
  }
  if ((arduinoCommand == 7) && (lastArduinoCommand == 0)) {
    removeDrums();
  }

  lastArduinoCommand = arduinoCommand;
}

void loopLeds() {
  int elapsedTime = (currentTime - startTime) % chordLoopTime;
    
  int currentTwoMeasure = elapsedTime / twoMeasureTime; // Changes every 2 measures (8 beats)
  int timeInTwoMeasure = elapsedTime % twoMeasureTime;
  
  int currentHalfNote = timeInTwoMeasure / halfNoteDuration;
  int timeInHalfNote = timeInTwoMeasure % halfNoteDuration;

  // Smooth pulsing brightness
  uint8_t brightness = sin8(map(timeInHalfNote, 0, halfNoteDuration, 0, 255));

  // Smooth transition between two-measure colors
  CRGB currentColor = measureColors[currentTwoMeasure % 4];
  CRGB nextColor = measureColors[(currentTwoMeasure + 1) % 4];
  
  uint8_t blendFactor = map(timeInTwoMeasure, 0, twoMeasureTime, 0, 255);
  CRGB blendedColor = blend(currentColor, nextColor, blendFactor);

  for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = blendedColor;
      leds[i].nscale8(brightness);
  }
  FastLED.show();
}

void removeMelody() {
  // Repeat 3 times, find the 3 latest added notes and remove them
  for (int i = 0; i < 3; i++) {
    bool found = false;
    int j = 1; // j = 1 so j - 1 is still accessible always
    while (!found) {
      if (locationsOfAddedNotes[j] == 0) {
        // The note before j was last added, so it should be removed from melody, and locations array
        melody[locationsOfAddedNotes[j-1]] = 0;
        locationsOfAddedNotes[j-1] = 0;
        found = true;
      }
      else {
        j++;
      }
    }
  }

}

void addMelody() {
  // Generate random note heigth
  // Put it in a random location (0-15) * 2
  // Look if that location is a rest (0) else, repeat find location and put
  // Maybe repeat a few times

  for (int i = 0; i < 3; i++) {
    //int randomDuration = random(2); // 0 is half note, 1 is quarter, 2 is double eigth // Not used anymore
    int randomNote = C_PENTATONIC[random(5)];

    int randomLocation;
    bool found = false;
    while(!found) {
      //randomLocation = random(16) * 2; // Not used anymore, now just choosing one of 32
      randomLocation = random(32);

      if (melody[randomLocation] == 0) {
        found = true;
      }
    }
  
    melody[randomLocation] = randomNote;

    //Code for the rewind button
    bool found2 = false;
    int j = 0;
    while (!found2) {
      if (locationsOfAddedNotes[j] == 0) {
        locationsOfAddedNotes[j] = randomLocation;
        found2 = true;
      }
      else {
        j++;
      }
    }
  }
}

void loopMelody() {
  int currentEigth = ((currentTime - startTime) % chordLoopTime) / (timePer16th * 2);
  int prevEigth = ((prevTime - startTime) % chordLoopTime) / (timePer16th * 2);

  if (currentEigth != prevEigth && melody[currentEigth] != 0) {
    // Zet vorige note uit
    MIDI.sendNoteOff(lastNotePlayed, 0, 1);
    
    // Play new note
    MIDI.sendNoteOn(melody[currentEigth], volume, 1);
    lastNotePlayed = melody[currentEigth];
  }
}

void loopChords() {

  int currentMeasure = ((currentTime - startTime) % chordLoopTime) / measureTime;
  int prevMeasure = ((prevTime - startTime) % chordLoopTime) / measureTime;

  // Handles simple activation levels (1-5/6)
  if (currentMeasure != prevMeasure && chordProgression[currentMeasure][0] != 0) {
    // Set notes from previous measures off
    if (chordActivationLevel > 0 && chordActivationLevel < 5) {
      MIDI.sendNoteOff(chordProgression[prevMeasure][0], 0, 2); // Root
    }
    if (chordActivationLevel > 1 && chordActivationLevel < 6) {
      MIDI.sendNoteOff(chordProgression[prevMeasure][0] + 7, 0, 2); // 5th
    }
    if (chordActivationLevel > 2 && chordActivationLevel < 6) {
      MIDI.sendNoteOff(chordProgression[prevMeasure][1], 0, 2); // 3rd
    }
    if (chordActivationLevel > 3 && chordActivationLevel < 6) {
      MIDI.sendNoteOff(chordProgression[prevMeasure][0] - 12, 0, 2); // Base
    }

    // Turn notes from now on
    if (chordActivationLevel > 0 && chordActivationLevel < 5) {
      MIDI.sendNoteOn(chordProgression[currentMeasure][0], volume, 2); // Root
    }
    if (chordActivationLevel > 1 && chordActivationLevel < 6) {
      MIDI.sendNoteOn(chordProgression[currentMeasure][0] + 7, volume, 2); // 5th
    }
    if (chordActivationLevel > 2 && chordActivationLevel < 6) {
      MIDI.sendNoteOn(chordProgression[currentMeasure][1], volume, 2); // 3rd
    }
    if (chordActivationLevel > 3 && chordActivationLevel < 6) {
      MIDI.sendNoteOn(chordProgression[currentMeasure][0] - 12, volume, 2); // Base
    }
  }

  // Todo add base pattern at 5
  int currentHalfMeasure = ((currentTime - startTime) % measureTime) / (measureTime / 2);
  int prevHalfMeasure = ((prevTime - startTime) % measureTime) / (measureTime / 2);

  if (currentHalfMeasure != prevHalfMeasure && chordProgression[currentMeasure][0] != 0 && chordActivationLevel > 4) {
    // play bass pattern
    switch (bassPattern) {
      case 1:
        if (currentHalfMeasure == 0) {
          // Off
          MIDI.sendNoteOff(chordProgression[prevMeasure][0] - 12, 0, 2); // Base
          MIDI.sendNoteOff(chordProgression[prevMeasure][0] - 24, 0, 2); // HyperBase

          // On
          MIDI.sendNoteOn(chordProgression[currentMeasure][0] - 12, volume, 2); // Base
          MIDI.sendNoteOn(chordProgression[currentMeasure][0] - 24, volume, 2); // HyperBase
        }
        break;
      case 2:
        if (currentHalfMeasure == 0) {
          MIDI.sendNoteOff(chordProgression[prevMeasure][0] - 12, 0, 2); // Base
          MIDI.sendNoteOn(chordProgression[currentMeasure][0] - 12, volume, 2); // Base
        }
        if (currentHalfMeasure == 1) {
          MIDI.sendNoteOff(chordProgression[currentMeasure][0] - 12, 0, 2); // Base
          MIDI.sendNoteOn(chordProgression[currentMeasure][0] - 12, volume, 2); // Base
        }
        break;
      case 3:
        if (currentHalfMeasure == 0) {
          MIDI.sendNoteOff(chordProgression[prevMeasure][0] - 5, 0, 2); // Base
          MIDI.sendNoteOn(chordProgression[currentMeasure][0] - 12, volume, 2); // Base
        }
        if (currentHalfMeasure == 1) {
          MIDI.sendNoteOff(chordProgression[currentMeasure][0] - 12, 0, 2); // Base
          MIDI.sendNoteOn(chordProgression[currentMeasure][0] - 5, volume, 2); // Base
        }
        break;
    }
  }

  // Todo add chord rhythm at 6
  int currentEigth = ((currentTime - startTime) % measureTime) / (timePer16th * 2);
  int prevEigth = ((prevTime - startTime) % measureTime) / (timePer16th * 2);

  if (currentEigth != prevEigth && chordProgression[currentMeasure][0] != 0 && chordActivationLevel > 5) {
    // If there is a 1, play that note, if there is a -1, stop that note.
    if (chordPatterns[selectedChordPattern][0][currentEigth] == 1) {
      MIDI.sendNoteOn(chordProgression[currentMeasure][0], volume, 2); // Root
    }
    if (chordPatterns[selectedChordPattern][0][currentEigth] == -1) {
      if (currentEigth == 0) {
        MIDI.sendNoteOff(chordProgression[prevMeasure][0], 0, 2); // Root from previous
      }
      else {
        MIDI.sendNoteOff(chordProgression[currentMeasure][0], 0, 2); // Root
      }
    }

    if (chordPatterns[selectedChordPattern][1][currentEigth] == 1) {
      MIDI.sendNoteOn(chordProgression[currentMeasure][1], volume, 2); // Third
    }
    if (chordPatterns[selectedChordPattern][1][currentEigth] == -1) {
      if (currentEigth == 0) {
        MIDI.sendNoteOff(chordProgression[prevMeasure][1], 0, 2); // Third
      }
      else {
        MIDI.sendNoteOff(chordProgression[currentMeasure][1], 0, 2); // Third
      }
    }

    if (chordPatterns[selectedChordPattern][2][currentEigth] == 1) {
      MIDI.sendNoteOn(chordProgression[currentMeasure][0] + 7, volume, 2); // Fifth
    }
    if (chordPatterns[selectedChordPattern][2][currentEigth] == -1) {
      if (currentEigth == 0) {
        MIDI.sendNoteOff(chordProgression[prevMeasure][0] + 7, 0, 2); // Fifth
      }
      else {
        MIDI.sendNoteOff(chordProgression[currentMeasure][0] + 7, 0, 2); // Fifth
      }
    }
  }
}

void removeChords() {
  chordActivationLevel--;
}

void addChords() {
  chordActivationLevel++;

  if (chordActivationLevel == 5) {
    // Generate a bass pattern
    bassPattern = random(1,4);
  }

  if (chordActivationLevel == 6) {
    selectedChordPattern = random(4);
  }

  // Generate a random progression, do not use already used chords
  if (chordActivationLevel == 1) {
    
    // Start with root chord
    int progression[4] = {1, 0, 0, 0};

    chordProgression[0][0] = C_MAJOR[0];
    chordProgression[0][1] = C_MAJOR[1];

    // Build demo progression
    for (int i = 1; i < 4; i++) {
      int randomChord;
      while (true) {
        // Rest of chords are random
        randomChord = random(2, 7);
        bool alreadyUsed = false;

        // Check if already used
        for (int j = 1; j < i; j++) {
          if (progression[j] == randomChord) {
            alreadyUsed = true;
          }
        }

        // We found a chord that has not been used
        if (!alreadyUsed) {
          break;
        }
      }
      progression[i] = randomChord;

      // Put the chord into the actual progression
      switch (randomChord) {
        case 2:
          chordProgression[i][0] = D_MINOR[0];
          chordProgression[i][1] = D_MINOR[1];
          break;
        case 3:
          chordProgression[i][0] = E_MINOR[0];
          chordProgression[i][1] = E_MINOR[1];
          break;
        case 4:
          chordProgression[i][0] = F_MAJOR[0];
          chordProgression[i][1] = F_MAJOR[1];
          break;
        case 5:
          chordProgression[i][0] = G_MAJOR[0];
          chordProgression[i][1] = G_MAJOR[1];
          break;
        case 6:
          chordProgression[i][0] = A_MINOR[0];
          chordProgression[i][1] = A_MINOR[1];
          break;
      }
    }
    

  }
}

void loopSFX() {
    // Play sound effect
  if ((currentTime > playSFXNoteTime) && (playSFXNoteTime != 0)) {
    playSFXNoteTime += sfxNoteDuration;
    MIDI.sendNoteOff(sfxMelody[currentSFXstep], 0, 3);

    if (currentSFXstep < 8) {
      currentSFXstep++;
      MIDI.sendNoteOn(sfxMelody[currentSFXstep], volume, 3);
    }
    else { // Sound effect finished
      playSFXNoteTime = 0;
    }
  }
}

void loopDrums() {
    // Play drum loop
  int current16th = ((currentTime - startTime) % measureTime) / timePer16th;
  int prevFrame16th = ((prevTime - startTime) % measureTime) / timePer16th;

  if (current16th != prevFrame16th) {

    // Kick
    if (drumLoop[0][current16th] == 1) {
      MIDI.sendNoteOn(KICK_NOTE, volume, 10);
    }
    else {
      MIDI.sendNoteOff(KICK_NOTE, 0, 10);
    }

    // Snare
    if (drumLoop[1][current16th] == 1) {
      MIDI.sendNoteOn(SNARE_NOTE, volume, 10);
    }
    else {
      MIDI.sendNoteOff(SNARE_NOTE, 0, 10);
    }

    // Hihat
    if (drumLoop[2][current16th] == 1) {
      MIDI.sendNoteOn(HIHAT_NOTE, volume, 10);
    }
    else {
      MIDI.sendNoteOff(HIHAT_NOTE, 0, 10);
    }

    // Clap
    if (drumLoop[3][current16th] == 1) {
      MIDI.sendNoteOn(CLAP_NOTE, volume, 10);
    }
    else {
      MIDI.sendNoteOff(CLAP_NOTE, 0, 10);
    }

    // Tom 
    if (drumLoop[4][current16th] == 1) {
      MIDI.sendNoteOn(TOM_NOTE, volume, 10);
    }
    else {
      MIDI.sendNoteOff(TOM_NOTE, 0, 10);
    }

    // Misc
    if (drumLoop[5][current16th] == 1) {
      MIDI.sendNoteOn(lastDrumInstrument, volume, 10);
    }
    else {
      MIDI.sendNoteOff(lastDrumInstrument, 0, 10);
    }

  }
}

void playSFX() {
  // Choose random sound effect (maybe instrument range 24-79)?
  int instrument = random(24, 79);
  MIDI.sendProgramChange(instrument, 3);

  // Start sound effect
  MIDI.sendNoteOn(sfxMelody[0], volume, 3);
  playSFXNoteTime = millis() + sfxNoteDuration;
  currentSFXstep = 0;
}

void removeDrums() {
  drumActivationLevel--;
  for (int i = 0; i < 16; i++) {
    drumLoop[drumActivationLevel][i] = 0;
  }
}

void addDrums() {
  // Add kick drum
  if (drumActivationLevel == 0) {
    int randomKick = random(3);
    for (int i = 0; i < 16; i++) {
      drumLoop[0][i] = kickPatterns[randomKick][i];
    }
  } 
  // Add snare
  else if (drumActivationLevel == 1) {
    int randomSnare = random(4);
    for (int i = 0; i < 16; i++) {
      drumLoop[1][i] = snarePatterns[randomSnare][i];
    }
  }
  // Add hihat
  else if (drumActivationLevel == 2) {
    int randomHihat = random(4);
    for (int i = 0; i < 16; i++) {
      drumLoop[2][i] = hihatPatterns[randomHihat][i];
    }
  }
  // Add clap
  else if (drumActivationLevel == 3) {
    int randomClap = random(3);
    for (int i = 0; i < 16; i++) {
      drumLoop[3][i] = clapPatterns[randomClap][i];
    }
  }
  // Add tom
  else if (drumActivationLevel == 4) {
    int randomTom = random(3);
    for (int i = 0; i < 16; i++) {
      drumLoop[4][i] = tomPatterns[randomTom][i];
    }
  }
  // Add misc
  else if (drumActivationLevel == 5) {
    int lastDrum = random(3);

    if (lastDrum == 0) {
      lastDrumInstrument = COWBELL_NOTE;
      for (int i = 0; i < 16; i++) {
        drumLoop[5][i] = cowbellPattern[i];
      }
    }
    else if (lastDrum == 1) {
      lastDrumInstrument = RIMCLICK_NOTE;
      for (int i = 0; i < 16; i++) {
        drumLoop[5][i] = rimclickPattern[i];
      }
    }
    else if (lastDrum == 2) {
      lastDrumInstrument = SHAKER_NOTE;
      for (int i = 0; i < 16; i++) {
        drumLoop[5][i] = shakerPattern[i];
      }
    }
  }
  drumActivationLevel++;
}

void readPot (void) {
  //read the pot value and map the analog value to an volume value between 0-10
  potValue = analogRead(potPin);
  oledVolume = map(potValue, 0, 1023, 0, 10);
  volume = map(potValue, 0, 1023, 0, 127);
}
