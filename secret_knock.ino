#include <stdio.h>
#include <stdlib.h>
#include "structs.h"

#define NSAMPLES 10
#define DEBUG false

/* User Config Block */

float secretKey[]  = {1, 1, .5, .5, .5, 1, .5, 1, .5, .5, .5, .5, 1, 1, .5, .5, .5, 1, .5, .5, .5, 1};  // Bad Romance 
//float secretKey[]  = {1, 1.5, .5, 1, 1, 1.5, .5};                 // Auld Lang Syne
//float secretKey[]  = {1, .5, .5, 1, 2, 1};                        // Shave and a haircut
// float secretKey[]  = {1, 2, 2, 1};                               // Terminator
// float secretKey[]  = {1, 1, .5, 1.5, 1, 1, 1, 1.5};              // Supermodel
// float secretKey[] = {1,1,1,1,.25,.25,.25,1.25,.25,.25,.25 };     // Stayin' alive
// float secretKey[] = {1,1,1,.666,.333,1,.666,.333};               // Imperial march

int scoreThreshold = 1.0;
int holdDoorOpenMs = 750;

int audioPin = 4;    // Analog input pin for doorbell signal
int unlockPin = 12;  // Digital output wired to transistor over doorbell
int ledPin = 13;     // LED output pin for debugging
/* End User Config Block */

int secretKeySize = sizeof(secretKey)/sizeof(*secretKey) + 1;
int samples[NSAMPLES];

CircularBuffer buffer;

int avg(){
  float ret = 0;
  for (int i=0; i<NSAMPLES; i = i+1){
    ret = ret + samples[i];
  }
  return (int)(ret/NSAMPLES+.5);
}

void debug(int startTime, int endTime){
  if (DEBUG == false) return;
  endTime = micros();
  int current = avg();
  String s = String(NSAMPLES) + 
             String(" samples in ") + 
             String(endTime - startTime) + 
             String(" microseconds.\n") + 
             String("Average reading: ") + String(current);
 
  Serial.println(s);      
}

int takeSamples(){
  int i = 0;
  int startTime, endTime;
  
  startTime = micros();
  for (i=0; i<NSAMPLES; i = i+1){
    samples[i] = analogRead(audioPin);
  }
  
  debug(startTime, endTime);
  return avg();
}


boolean waitForPress(int timeout){
  int ret = 0;
  unsigned long waited = 0;
  unsigned long waitingFrom = millis();
  
  while(true){
    waited = millis();
    if (timeout > 0 && waited - waitingFrom >= timeout) return false;
    if (takeSamples() < 20){
      break;
    }
  }
  
  while(true){
    waited = millis();
    if (timeout > 0 && waited - waitingFrom >= timeout) return false;
    if (takeSamples() > 100){
      return true;
    }
  }
}

void printBuffer(){

  Serial.println(String("Start: ") + String(buffer.start));
  Serial.println(String("End: ") + String(buffer.end));

  Serial.println("Start: ");

  int i=buffer.start;
  while(true){
    Serial.print(String(i)+String("\t") + String(buffer.elems[i].time) + "\t"  + String(buffer.elems[i].diff) + "\t");
    Serial.println(buffer.elems[i].ratio, 3);
    i = (i + 1)%buffer.size;
    if (i == buffer.start) break;
  }
}


void calculateRatios(){
  
  int thisKnock = (buffer.start+1)%buffer.size;
  
  unsigned long normalizeBy = (
    buffer.elems[thisKnock   ].time -
    buffer.elems[buffer.start].time
  );
  
  while (true) {
    
    int lastKnock = (thisKnock-1)%buffer.size;
    if (lastKnock < 0) lastKnock += buffer.size;
    
    buffer.elems[thisKnock].diff = (float)(
      buffer.elems[thisKnock].time - 
      buffer.elems[lastKnock].time
    );    
    
    buffer.elems[thisKnock].ratio = buffer.elems[thisKnock].diff * 1.0 / normalizeBy ;
    
    if (thisKnock == buffer.start) break;
    thisKnock = (thisKnock + 1)%buffer.size;
  }
  
  return;
}

float calculateScore(){
  float ret = 0;
  int i=(buffer.start+1)%buffer.size;
  for (int j=0;j<secretKeySize-1;j++){
    ret += abs((buffer.elems[i].ratio - secretKey[j]) / (secretKey[j]));
    i = (i + 1)%buffer.size;
  }
  return ret / secretKeySize * 10;
}

void unlockDoor(){
   digitalWrite(unlockPin, HIGH); 
   delay(holdDoorOpenMs);
   digitalWrite(unlockPin, LOW); 
}


void setup() {
  pinMode(unlockPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  cbInit(&buffer, secretKeySize);
}

void loop() { 
  
  waitForPress(0);
  Knock k = {.time = millis(), .diff=0,.ratio=99.9};
  cbWrite(&buffer, &k);
  
  calculateRatios();
  printBuffer();
  
  Serial.print("Score: " );
  float score = calculateScore();
  if (score < scoreThreshold){
    analogWrite(ledPin, 255);
    Serial.print("You win!");
    Serial.println(score,3);
    unlockDoor();
  }else {
    Serial.print("You lose!");
    Serial.println(score,3);
    analogWrite(ledPin, 0);
  }
  
}

void cbInit(CircularBuffer *cb, int size) {
    cb->size  = size; /* don't include empty elem */
    cb->start = 0;
    cb->end   = size-1;
    cb->elems = (Knock *)calloc(cb->size, sizeof(Knock));
}
 
void cbWrite(CircularBuffer *cb, Knock *elem) {
    cb->end = (cb->end + 1) % cb->size;
    if (cb->end == cb->start)
        cb->start = (cb->start + 1) % cb->size; /* full, overwrite */
    cb->elems[cb->end] = *elem;
}
