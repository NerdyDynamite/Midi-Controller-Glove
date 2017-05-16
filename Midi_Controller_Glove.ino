#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

#define SMOOTHING_FACTOR 20
#define NUMSENSORS 8
// Global Variables
volatile bool debugMode = false; // set this to true if you want a serial output instead of midi output
int r0 = 0;      //value of select pin at the 4051 (s0)
int r1 = 0;      //value of select pin at the 4051 (s1)
int r2 = 0;      //value of select pin at the 4051 (s2)
int count = 0;   //which output pin (y on multiplexer) we are selecting
const int muxpin = 0; // Mux output to Analog Pin 0

// for the sensor values
volatile int sensorValue[NUMSENSORS];
volatile int lastValue[NUMSENSORS];

// Calibration constants for each sensor
volatile int sensorMin[NUMSENSORS] = {1023,1023,1023,1023,1023,1023,1023,1023};   // Minimum sensor value
volatile int sensorMax[NUMSENSORS] = {0,0,0,0,0,0,0,0};                           // Maximum sensor value

// MIDI related variables
int midiChannels[] = {2,3,4,5,6,7,8,9}; // map a midi channel onto each flex sensor  
volatile bool soundOn[] = {false,false,false,false};   // should have all set to false

// For smoothing the sensor values
volatile int sum;

void setup(){
  // Set digital outputs for the multiplexer
  pinMode(2, OUTPUT);    // s0
  pinMode(3, OUTPUT);    // s1
  pinMode(4, OUTPUT);    // s2

  // set baud rate
  if (debugMode == true) {
    Serial.begin(38400);
  }
  else {
    MIDI.begin();
  }

  delay(1000); // avoids a spike in the accelerometer outputs when the Arduino starts serial communication
  calibrateSensors();
}
void loop () {
  // This for loop reads the inputs to the multiplexer in sequence
  for (count=0; count<NUMSENSORS; count++) {
    // Set output to multiplexer pin depending on count 
    setMuxOutput(count);
    
    sum = 0;
    // read sensor values in sequence
    for (int k=0;k<SMOOTHING_FACTOR;k++) {
      sensorValue[count] = analogRead(muxpin);
      sensorValue[count] = map(sensorValue[count], sensorMin[count], sensorMax[count], -20, 200);
      sensorValue[count] = constrain(sensorValue[count], 0, 108);
      sum = sum + sensorValue[count];
    }

    sensorValue[count] = sum/SMOOTHING_FACTOR;
    
    if (debugMode == false){
      // MIDI code
      for (int j=0;j<4;j++) {
          if (sensorValue[j] > 10) {
            // MIDI.sendNoteOn(sensorValue[j], 127, midiChannels[j]);    // Send a Note (pitch, velo 127 on channel j), where j is the current flex sensor number             
            MIDI.sendControlChange(7,sensorValue[j],midiChannels[j]);
            
            soundOn[j] = true;
          }
          else {
            if (soundOn[j] != false) {        
              for (int i=0;i<2;i++) {
                //turnAllNotesOff(j);
                MIDI.sendControlChange(7,0,midiChannels[j]);
              }
              soundOn[j] = false;
            }
          }
      }

//      if (sensorValue[7] > 3) {
//        MIDI.sendControlChange(7,sensorValue[7],midiChannels[7]); // send the z axis acceleration only
//
//        soundOn[j] = true;
//      }
//      else {
//        // Do nothing
//      }
      
    }
    else {
      Serial.print(sensorValue[count]);

      if (count == 7){
        Serial.println("");
      }
      else {
        Serial.print(", ");
      }  
    }
  }
}

void calibrateSensors() {
  
  while (millis() < 15000)
  {
    int countdown = 15-millis()/1000;
    MIDI.sendControlChange(7,countdown,countdown);
//    Serial.println("Debug mode ON: Calibration for 15 seconds");
    
    for (int i=0;i<NUMSENSORS;i++) {
      setMuxOutput(i);
      
      sensorValue[i] = analogRead(muxpin);    // read sensor connected to analog pin 0
  
      if (sensorValue[i] > sensorMax[i])    // save the maximum sensor value found
      {
        sensorMax[i] = sensorValue[i];
      }
  
      if (sensorValue[i] < sensorMin[i])    // save the minimum sensor value found
      {
        sensorMin[i] = sensorValue[i];
      }
    }
  }
}

void setMuxOutput(int count) {
  // sets the mux output depending on the input count
  r0 = bitRead(count,0);    // use this with arduino 0013 (and newer versions)     
  r1 = bitRead(count,1);    // use this with arduino 0013 (and newer versions)     
  r2 = bitRead(count,2);    // use this with arduino 0013 (and newer versions)    
  digitalWrite(2, r0);
  digitalWrite(3, r1);
  digitalWrite(4, r2);
}

void turnAllNotesOff(int flexSensorNumber) {
  for (int i=0; i<=127; i++) {
    MIDI.sendNoteOff(i, 0, midiChannels[flexSensorNumber]);     // Stop the note
  }
}

