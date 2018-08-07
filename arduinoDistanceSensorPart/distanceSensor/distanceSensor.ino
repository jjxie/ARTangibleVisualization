/******************************************************************************
   SparkFun_VL6180X_demo.ino
   Example Sketch for VL6180x time of flight range finder.
   Casey Kuhns @ SparkFun Electronics
   10/29/2014
   https://github.com/sparkfun/SparkFun_ToF_Range_Finder-VL6180_Arduino_Library

   Multiple VL6180 solution from https://learn.sparkfun.com/tutorials/vl6180-hookup-guide/discuss
   Add _i2caddress = new_address; to in Sparkfun_VL6180X.cpp in here:
   … VL6180x_setRegister(VL6180X_I2C_SLAVE_DEVICE_ADDRESS, new_address); i2caddress = new_address;
   return VL6180x_getRegister(VL6180X_I2C_SLAVE_DEVICE_ADDRESS); }

   Add one Euro filter to reduce jitter
 ******************************************************************************/
#include <SF1eFilter.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

SF1eFilter filter1;
SF1eFilter filter2;
SF1eFilter filter3;
SF1eFilter filter4;
SF1eFilter filter5;

#include <Wire.h>

#include <SparkFun_VL6180X.h>

#define VL6180X_ADDRESS 0x29
#define NEWVL6180X_ADDRESS1 0x30
#define NEWVL6180X_ADDRESS2 0x32
#define NEWVL6180X_ADDRESS3 0x34
#define NEWVL6180X_ADDRESS4 0x36
#define NEWVL6180X_ADDRESS5 0x38

//int enable1 = 27;
//int enable2 = 26;
//int enable3 = 25;
//int enable4 = 33;
int enable1 = A0;
int enable2 = A1;
int enable3 = A2;
int enable4 = A3;
int enable5 = 8;

VL6180xIdentification identification1;
VL6180xIdentification identification2;
VL6180xIdentification identification3;
VL6180xIdentification identification4;
VL6180xIdentification identification5;
VL6180x sensor1(VL6180X_ADDRESS);
VL6180x sensor2(VL6180X_ADDRESS);
VL6180x sensor3(VL6180X_ADDRESS);
VL6180x sensor4(VL6180X_ADDRESS);
VL6180x sensor5(VL6180X_ADDRESS);


// The received weight data
float milkWeight = 0.00;
float orangeWeight = 0.00;
float meatWeight = 0.00;
float brocolliWeight = 0.00;
float fishWeight = 0.00;

// Distance sensors
float milkRackDistance = 0.00;
float orangeRackDistance = 0.00;
float meatRackDistance = 0.00;
float broccoliRackDistance = 0.00;
float fishRackDistance = 0.00;

// When the servo rotates full degree, the rack distance from the distance sensor
float milkFullDegreeRackDistance = 16.2;
float orangeFullDegreeRackDistance = 16.2;
float meatFullDegreeRackDistance = 16.6;
float broccoliFullDegreeRackDistance = 16.2;
float fishFullDegreeRackDistance = 16.2;


float milkPreviousRackDistance = 11.50;
float orangePreviousRackDistance = 9.50;
float meatPreviousRackDistance = 9.50;
float broccoliPreviousRackDistance = 7.50;
float fishPreviousRackDistance = 13.00;

// Set relative distance, distance of broccoli as baseline.Same height will have same weight 
float milkRackInitialDistance = 11.50;
float orangeRackInitialDistance = 9.50;
float meatRackInitialDistance = 9.50;
float broccoliRackInitialDistance = 7.50;
float fishRackInitialDistance = 13.00;

float maxWeight = 1000;


// One filter code
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
  if (!filter->usedBefore) {
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

  if (filter->lastTime == 0 && filter->frequency != filter->config.frequency) {
    filter->frequency = filter->config.frequency;
  }

  if (filter->xfilt.usedBefore) {
    dx = (x - filter->xfilt.xprev) * filter->frequency;
  }

  float edx = SFLowPassFilterDo(&(filter->dxfilt), dx, SF1eFilterAlpha(filter, filter->config.derivativeCutoffFrequency));
  float cutoff = filter->config.minCutoffFrequency + filter->config.cutoffSlope * fabsf(edx);
  return SFLowPassFilterDo(&(filter->xfilt), x, SF1eFilterAlpha(filter, cutoff));
}

float SF1eFilterDoAtTime(SF1eFilter *filter, float x, double timestamp)
{
  if (filter->lastTime != 0) {
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

  Serial.begin(115200); //Start Serial at 115200bps
  Wire.begin(); //Start I2C library
  delay(100); // delay .1s

  pinMode(enable1, OUTPUT);
  pinMode(enable2, OUTPUT);
  pinMode(enable3, OUTPUT);
  pinMode(enable4, OUTPUT);
  pinMode(enable5, OUTPUT);

  // Enable sensor 1 pin, disable other pins
  digitalWrite(enable1, HIGH);
  digitalWrite(enable2, LOW);
  digitalWrite(enable3, LOW);
  digitalWrite(enable4, LOW);
  digitalWrite(enable5, LOW);
  sensor1.getIdentification(&identification1); // Retrieve manufacture info from device memory
//  printIdentification(&identification1); // Helper function to print all the Module information
  if (sensor1.VL6180xInit() != 0) {
    Serial.println("SENSOR 1 FAILED TO INITALIZE"); //Initialize device and check for errors
  };
  sensor1.VL6180xDefautSettings(); //Load default settings to get started.
  delay(1000); // delay 1s
  sensor1.changeAddress(VL6180X_ADDRESS, NEWVL6180X_ADDRESS1);

  // Enable sensor 2 pin, disable other pins
  digitalWrite(enable2, HIGH);
  sensor2.getIdentification(&identification2);
//  printIdentification(&identification2);
  if (sensor2.VL6180xInit() != 0) {
    Serial.println("SENSOR 2 FAILED TO INITALIZE"); //Initialize device and check for errors
  };
  sensor2.VL6180xDefautSettings();
  delay(1000); // delay 1s
  sensor2.changeAddress(VL6180X_ADDRESS, NEWVL6180X_ADDRESS2);

  // Enable sensor 3 pin, disable other pins
  digitalWrite(enable3, HIGH);
  sensor3.getIdentification(&identification3);
//  printIdentification(&identification3);
  if (sensor3.VL6180xInit() != 0) {
    Serial.println("SENSOR 3 FAILED TO INITALIZE"); //Initialize device and check for errors
  };
  sensor3.VL6180xDefautSettings();
  delay(1000); // delay 1s
  sensor3.changeAddress(VL6180X_ADDRESS, NEWVL6180X_ADDRESS3);

  // Enable sensor 4 pin, disable other pins
  digitalWrite(enable4, HIGH);
  sensor4.getIdentification(&identification4);
//  printIdentification(&identification4);
  if (sensor4.VL6180xInit() != 0) {
    Serial.println("SENSOR 4 FAILED TO INITALIZE"); //Initialize device and check for errors
  };
  sensor4.VL6180xDefautSettings();
  delay(1000); // delay 1s
  sensor4.changeAddress(VL6180X_ADDRESS, NEWVL6180X_ADDRESS4);

  // Enable sensor 5 pin, disable other pins
  digitalWrite(enable5, HIGH);
  sensor5.getIdentification(&identification5);
//  printIdentification(&identification5);
  if (sensor5.VL6180xInit() != 0) {
    Serial.println("SENSOR 5 FAILED TO INITALIZE"); //Initialize device and check for errors
  };
  sensor5.VL6180xDefautSettings();
  delay(1000); // delay 1s
  sensor5.changeAddress(VL6180X_ADDRESS, NEWVL6180X_ADDRESS5);

  // The presettings of each fliter
  srand((unsigned int)time(NULL));
  filter1.config.frequency = 120;
  filter1.config.cutoffSlope = 1;
  filter1.config.derivativeCutoffFrequency = 1;
  filter1.config.minCutoffFrequency = 1;
  SF1eFilterInit(&filter1);

  srand((unsigned int)time(NULL));
  filter2.config.frequency = 120;
  filter2.config.cutoffSlope = 1;
  filter2.config.derivativeCutoffFrequency = 1;
  filter2.config.minCutoffFrequency = 1;
  SF1eFilterInit(&filter2);

  srand((unsigned int)time(NULL));
  filter3.config.frequency = 120;
  filter3.config.cutoffSlope = 1;
  filter3.config.derivativeCutoffFrequency = 1;
  filter3.config.minCutoffFrequency = 1;
  SF1eFilterInit(&filter3);

  srand((unsigned int)time(NULL));
  filter4.config.frequency = 120;
  filter4.config.cutoffSlope = 1;
  filter4.config.derivativeCutoffFrequency = 1;
  filter4.config.minCutoffFrequency = 1;
  SF1eFilterInit(&filter4);

  srand((unsigned int)time(NULL));
  filter5.config.frequency = 120;
  filter5.config.cutoffSlope = 1;
  filter5.config.derivativeCutoffFrequency = 1;
  filter5.config.minCutoffFrequency = 1;
  SF1eFilterInit(&filter5);

}

void loop() {

  // Read distance data
  // Get Sensor Distance and filtered by One Euro filter
  milkRackDistance = SF1eFiltered1(sensor1.getDistance());
//  Serial.print("1  ");
//  Serial.println(milkRackDistance);
  // If distance changes more than 1.5CM
  if (checkDistance(milkPreviousRackDistance, milkRackDistance)) {
    delay(500);
    int i;
    float readNumber[20];
    float temp1, temp2;
    for (i = 0; i < 20; i++) {
      temp1 = SF1eFiltered1(sensor1.getDistance());
      temp2 = SF1eFiltered1(sensor1.getDistance());
      if (abs(temp2 - temp1) < 5.00) {
        float milkWeight = calWeight (temp2, milkFullDegreeRackDistance, milkRackInitialDistance);
        if (milkWeight < 0 ) {
          milkWeight = 0;
        }
        Serial.print("A");
        Serial.println(milkWeight);
        milkPreviousRackDistance = temp2;
        break;
      }
    }
  }
  
  delay(100);

  // Orange
  orangeRackDistance = SF1eFiltered2(sensor2.getDistance());
//  Serial.print("2  ");
//  Serial.println(orangeRackDistance);
  // If distance changes more than 1.5CM
  if (checkDistance(orangePreviousRackDistance, orangeRackDistance)) {
    delay(500);
    int i;
    float readNumber[20];
    float temp1, temp2;
    for (i = 0; i < 20; i++) {
      temp1 = SF1eFiltered2(sensor2.getDistance());
      temp2 = SF1eFiltered2(sensor2.getDistance());
      if (abs(temp2 - temp1) < 5.00) {
        float orangeWeight = calWeight (orangeRackDistance, orangeFullDegreeRackDistance, orangeRackInitialDistance);
        if (orangeWeight < 0 ) {
          orangeWeight = 0;
        }
        Serial.print("B");
        Serial.println(orangeWeight);
        orangePreviousRackDistance = temp2;
        break;
      }
    }
  }
  
  delay(100);

  // Meat
  meatRackDistance = SF1eFiltered3(sensor3.getDistance());
//  Serial.print("3  ");
//  Serial.println(meatRackDistance);
  // If distance changes more than 1.5CM
  if (checkDistance(meatPreviousRackDistance, meatRackDistance)) {
    delay(500);
    int i;
    float readNumber[20];
    float temp1, temp2;
    for (i = 0; i < 20; i++) {
      temp1 = SF1eFiltered3(sensor3.getDistance());
      temp2 = SF1eFiltered3(sensor3.getDistance());
      if (abs(temp2 - temp1) < 5.00) {
        float meatWeight = calWeight (meatRackDistance, meatFullDegreeRackDistance, meatRackInitialDistance);
        if (meatWeight < 0) {
          meatWeight = 0;
        }
        Serial.print("C");
        Serial.println(meatWeight);
        meatPreviousRackDistance = temp2;
        break;
      }
    }
  }
  
  delay(100);

  // Broccoli
  broccoliRackDistance = SF1eFiltered4(sensor4.getDistance());
//  Serial.print("4  ");
//  Serial.println(broccoliRackDistance);
  // If distance changes more than 1.5CM
  if (checkDistance(broccoliPreviousRackDistance, broccoliRackDistance)) {
    delay(500);
    int i;
    float readNumber[20];
    float temp1, temp2;
    for (i = 0; i < 20; i++) {
      temp1 = SF1eFiltered4(sensor4.getDistance());
      temp2 = SF1eFiltered4(sensor4.getDistance());
      if (abs(temp2 - temp1) < 5.00) {
        float broccoliWeight = calWeight (broccoliRackDistance, broccoliFullDegreeRackDistance, broccoliRackInitialDistance);
        if (broccoliWeight < 0) {
          broccoliWeight = 0;
        }
        Serial.print("D");
        Serial.println(broccoliWeight);
        broccoliPreviousRackDistance = temp2;
        break;
      }
    }
  }
  
  delay(100);

  // Fish
  fishRackDistance = SF1eFiltered5(sensor5.getDistance());
//  Serial.print("5  ");
//  Serial.println(fishRackDistance);
  // If distance changes more than 1.5CM
  if (checkDistance(fishPreviousRackDistance, fishRackDistance)) {
    delay(500);
    int i;
    float readNumber[20];
    float temp1, temp2;
    for (i = 0; i < 20; i++) {
      temp1 = SF1eFiltered5(sensor5.getDistance());
      temp2 = SF1eFiltered5(sensor5.getDistance());
      if (abs(temp2 - temp1) < 5.00) {
        float fishWeight = calWeight (fishRackDistance, fishFullDegreeRackDistance, fishRackInitialDistance);
        if (fishWeight < 0) {
          fishWeight = 0;
        }
        Serial.print("E");
        Serial.println(fishWeight);
        fishPreviousRackDistance = temp2;
        break;
      }
    }
  }
  
  delay(100);

}

void printIdentification(struct VL6180xIdentification * temp) {
  Serial.print("Model ID = ");
  Serial.println(temp->idModel);

  Serial.print("Model Rev = ");
  Serial.print(temp->idModelRevMajor);
  Serial.print(".");
  Serial.println(temp->idModelRevMinor);

  Serial.print("Module Rev = ");
  Serial.print(temp->idModuleRevMajor);
  Serial.print(".");
  Serial.println(temp->idModuleRevMinor);

  Serial.print("Manufacture Date = ");
  Serial.print((temp->idDate >> 3) & 0x001F);
  Serial.print("/");
  Serial.print((temp->idDate >> 8) & 0x000F);
  Serial.print("/1");
  Serial.print((temp->idDate >> 12) & 0x000F);
  Serial.print(" Phase: ");
  Serial.println(temp->idDate & 0x0007);

  Serial.print("Manufacture Time (s)= ");
  Serial.println(temp->idTime * 2);
  Serial.println();
  Serial.println();
}

// 1 Euro filter, IN initializeScaleData(scale1.get_units(), 1), OUT filtered
float SF1eFiltered1(float readData)
{
  float signal = readData;
  float randnum = (float)rand() * 1.f / RAND_MAX;
  float noisy = signal + (randnum - 0.5f) / 5.f;
  float filtered = SF1eFilterDo(&filter1, noisy);
  //    Serial.print(signal);
  //    Serial.print(" ");
  //    Serial.print(noisy);
  //    Serial.print(" ");
  //    Serial.print(filtered);
  //    Serial.print(" ");
  return filtered;
}

float SF1eFiltered2(float readData)
{
  float signal = readData;
  float randnum = (float)rand() * 1.f / RAND_MAX;
  float noisy = signal + (randnum - 0.5f) / 5.f;
  float filtered = SF1eFilterDo(&filter2, noisy);
  return filtered;
}

float SF1eFiltered3(float readData)
{
  float signal = readData;
  float randnum = (float)rand() * 1.f / RAND_MAX;
  float noisy = signal + (randnum - 0.5f) / 5.f;
  float filtered = SF1eFilterDo(&filter3, noisy);
  return filtered;
}

float SF1eFiltered4(float readData)
{
  float signal = readData;
  float randnum = (float)rand() * 1.f / RAND_MAX;
  float noisy = signal + (randnum - 0.5f) / 5.f;
  float filtered = SF1eFilterDo(&filter4, noisy);
  return filtered;
}

float SF1eFiltered5(float readData)
{
  float signal = readData;
  float randnum = (float)rand() * 1.f / RAND_MAX;
  float noisy = signal + (randnum - 0.5f) / 5.f;
  float filtered = SF1eFilterDo(&filter5, noisy);
  return filtered;
}

// Calculate the weight based on the distance of the bottom part of the rack from the distance sensor
float calWeight(float readingdistance, float fullLength, float intialDistance) {
  // Transform the data from MM to CM
  readingdistance = (readingdistance - intialDistance) / 10.00;
  float weight = round((readingdistance / fullLength) * maxWeight + 0.5);
  return weight;
}

// Check if the reading distance is more than 1.5CM than the previous distance data
boolean checkDistance(float previousDistance, float readingDistance) {
  if (abs(readingDistance - previousDistance) > 15.00) {
    return true;
  }
  else {
    return false;
  }
}


