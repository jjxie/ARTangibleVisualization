// 1 euro filter, http://cristal.univ-lille.fr/~casiez/1euro/
#include <SF1eFilter.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

SF1eFilter filter;

#include <HX711.h>
HX711 scale1;
HX711 scale2;
HX711 scale3;
HX711 scale4;
//HX711 scale5;
float calibration_factor1 = 2190; //1,1930    2,430    3,415    4,2295
float calibration_factor2 = 430;
float calibration_factor3 = 415;
float calibration_factor4 = 2295;
//float calibration_factor5 = 2010;
float preUnits1 = 0.0;
float preUnits2 = 0.0;
float preUnits3 = 0.0;
float preUnits4 = 0.0;
//float preUnits5 = 0.0;
float units1;
float units2;
float units3;
float units4;
//float units5;



// one filter code
SFLowPassFilter *SFLowPassFilterCreate()
{
  SFLowPassFilter *filter = (SFLowPassFilter *)malloc(sizeof(SFLowPassFilter));
  SFLowPassFilterInit(filter);
  return filter;
}

SFLowPassFilter *SFLowPassFilterDestroy(SFLowPassFilter *filter)
{
  free(filter);
  return NULL;
}

void SFLowPassFilterInit(SFLowPassFilter *filter)
{
  filter->usedBefore = 0;
  filter->hatxprev = 0;
  filter->xprev = 0;
}

float SFLowPassFilterDo(SFLowPassFilter *filter, float x, float alpha) {
  if(!filter->usedBefore) {
    filter->usedBefore = 1;
    filter->hatxprev = x;
  }
  float hatx = alpha * x + (1.f - alpha) * filter->hatxprev;
  filter->xprev = x;
  filter->hatxprev = hatx;
  return hatx;
}

SF1eFilter *SF1eFilterCreate(float frequency, float minCutoffFrequency, float cutoffSlope, float derivativeCutoffFrequency)
{
  SF1eFilterConfiguration config;
  config.frequency = frequency;
  config.minCutoffFrequency = minCutoffFrequency;
  config.cutoffSlope = cutoffSlope;
  config.derivativeCutoffFrequency = derivativeCutoffFrequency;
  return SF1eFilterCreateWithConfig(config);
}

SF1eFilter *SF1eFilterCreateWithConfig(SF1eFilterConfiguration config)
{
  SF1eFilter *filter = (SF1eFilter *)malloc(sizeof(SF1eFilter));
  filter->config = config;
  return filter;
}

SF1eFilter *SF1eFilterDestroy(SF1eFilter *filter)
{
  free(filter);
  return NULL;
}

void SF1eFilterInit(SF1eFilter *filter)
{
  filter->frequency = filter->config.frequency;
  filter->lastTime = 0;
  SFLowPassFilterInit(&(filter->xfilt));
  SFLowPassFilterInit(&(filter->dxfilt));
}

float SF1eFilterDo(SF1eFilter *filter, float x)
{
  float dx = 0.f;
  
  if(filter->lastTime == 0 && filter->frequency != filter->config.frequency) {
    filter->frequency = filter->config.frequency;
  }
  
  if(filter->xfilt.usedBefore) {
    dx = (x - filter->xfilt.xprev) * filter->frequency;
  }
  
  float edx = SFLowPassFilterDo(&(filter->dxfilt), dx, SF1eFilterAlpha(filter, filter->config.derivativeCutoffFrequency));
  float cutoff = filter->config.minCutoffFrequency + filter->config.cutoffSlope * fabsf(edx);
  return SFLowPassFilterDo(&(filter->xfilt), x, SF1eFilterAlpha(filter, cutoff));
}

float SF1eFilterDoAtTime(SF1eFilter *filter, float x, double timestamp)
{
  if(filter->lastTime != 0) {
    filter->frequency = 1.0f / (timestamp - filter->lastTime);
  }
  filter->lastTime = timestamp;
  float fx = SF1eFilterDo(filter, x);
  return fx;
}

float SF1eFilterAlpha(SF1eFilter *filter, float cutoff)
{
  float tau = 1.0f / (2.f * M_PI * cutoff);
  float te = 1.0f / filter->frequency;
  return 1.0f / (1.0f + tau / te);
}




void setup() {
  Serial.begin(9600);
  
  // parameter "gain" is ommited; the default value 128 is used by the library
  // HX711.DOUT  - pin #A1, FIRST parameter
  // HX711.PD_SCK - pin #A0, SECOND parameter
  // Wemos uses D4,D3, Arduino uses 4,3
  
  scale1.begin(D1,D2); //D1,D2  5,4
  scale2.begin(D3,D4); //D3,D4  0,2
  scale3.begin(D5,D6);  // D5,D6  14,12   
  scale4.begin(D7,D8);  // D7,D8  13,15

//  // Arduino board
//  scale1.begin(2,3);
//  scale2.begin(4,5); 
//  scale3.begin(6,7);  
//  scale4.begin(8,9); 

  scale1.set_scale(calibration_factor1);
  scale1.tare();
  delay(500);
  units1 = scale1.get_units(), 1;
  units1 = initializeScaleData(units1); 
  Serial.print("A");
  Serial.println(units1);
  
  scale2.set_scale(calibration_factor2);
  scale2.tare();
  delay(500);
  units2 = scale2.get_units(), 1;
  units2 = initializeScaleData(units2); 
  Serial.print("B");
  Serial.println(units2);
 
  
  scale3.set_scale(calibration_factor3);
  scale3.tare();
  delay(500);
  units3 = scale3.get_units(), 1;
  units3 = initializeScaleData(units3); 
  Serial.print("C");
  Serial.println(units3);
  
  
  scale4.set_scale(calibration_factor4);
  scale4.tare();
  delay(500);
  units4 = scale4.get_units(), 1;
  units4 = initializeScaleData(units4); 
  Serial.print("D");
  Serial.println(units4);

  srand((unsigned int)time(NULL));
  filter.config.frequency = 10;
  filter.config.cutoffSlope = 1;
  filter.config.derivativeCutoffFrequency = 1;
  filter.config.minCutoffFrequency = 1;
  SF1eFilterInit(&filter);
  
}

void loop() {
  // Data from scale number 1
  // Delay reading before and after placing an object for 1 second, waiting for a stable of the number
  
  delay(1000);
  // Data from scale number 1
  units1 = scale1.get_units(5), 1;
  units1 = initializeScaleData(units1);
  units1 = initializeScaleData(SF1eFiltered(units1));
  if(weightChange(preUnits1, units1)){
     preUnits1 = units1;
     Serial.print("A");
     Serial.println(units1);
  }
  
  delay(1000);
  // Data from scale number 2 
  units2 = scale2.get_units(5), 1;
  units2 = initializeScaleData(units2);
  units2 = initializeScaleData(SF1eFiltered(units2));
  if(weightChange(preUnits2, units2)){
     preUnits2 = units2;
     Serial.print("B");
     Serial.println(units2);
  }
  
  delay(1000);
  // Data from scale number 3
  units3 = scale3.get_units(5), 1;
  units3 = initializeScaleData(units3);
  units3 = initializeScaleData(SF1eFiltered(units3));
  if(weightChange(preUnits3, units3)){
     preUnits3 = units3;
     Serial.print("C");
     Serial.println(units3);
  }
  
  delay(1000);
  // Data from scale number 4
  units4 = scale4.get_units(5), 1;
  units4 = initializeScaleData(units4);
  units4 = initializeScaleData(SF1eFiltered(units4));
  if(weightChange(preUnits4, units4)){
     preUnits4 = units4;
     Serial.print("D");
     Serial.println(units4);
  }
}

// Check if the reading weight data < 0, then set units to 0.00
// else return reading data
float initializeScaleData(float readData){
  float result;
  if (readData < 0) {
    result = 0.00;
  }else {
    result = readData;
  }
  return result;
}

// If the absolute value of weight changes > 3, then return true, weight changed
boolean weightChange(float preWeight, float readWeight){
  if(abs(readWeight - preWeight) > 3 ){
    return true;
  } else {
    return false;
  }
}

// 1 Euro filter, IN initializeScaleData(scale1.get_units(), 1), OUT filtered
float SF1eFiltered(float readData)
{
  float signal = readData;
  float randnum = (float)rand() * 1.f / RAND_MAX;
  float noisy = signal + (randnum - 0.5f) / 5.f;
  float filtered = SF1eFilterDo(&filter, noisy);
//  Serial.print(signal);
//  Serial.print(" ");
//  Serial.print(noisy);
//  Serial.print(" ");
//  Serial.print(filtered);
//  Serial.println();
  return filtered;
}

