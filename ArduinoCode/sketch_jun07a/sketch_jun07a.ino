/******************************************************************************
   SparkFun_VL6180X, https://github.com/sparkfun/SparkFun_ToF_Range_Finder-VL6180_Arduino_Library

   Multiple VL6180 solution from https://learn.sparkfun.com/tutorials/vl6180-hookup-guide/discuss
   Add _i2caddress = new_address; to in Sparkfun_VL6180X.cpp in here:
   … VL6180x_setRegister(VL6180X_I2C_SLAVE_DEVICE_ADDRESS, new_address); i2caddress = new_address;
   return VL6180x_getRegister(VL6180X_I2C_SLAVE_DEVICE_ADDRESS); }

   Add one Euro filter to reduce jitter, http://cristal.univ-lille.fr/~casiez/1euro/
 ******************************************************************************/
 
#include <SF1eFilter.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

// Each distance sensor uses one SF1eFilter structure
SF1eFilter filter1;
SF1eFilter filter2;
SF1eFilter filter3;
SF1eFilter filter4;
//SF1eFilter filter5;

#include <SparkFun_VL6180X.h>
#include <Wire.h>
#include <math.h>
#include <Servo.h>


#define VL6180X_ADDRESS 0x29
#define NEWVL6180X_ADDRESS1 0x30
#define NEWVL6180X_ADDRESS2 0x32
#define NEWVL6180X_ADDRESS3 0x34
#define NEWVL6180X_ADDRESS4 0x36
//#define NEWVL6180X_ADDRESS5 0x38

int enable1 = A0;
int enable2 = A1;
int enable3 = A2;
int enable4 = A3;
//int enable5 = A4;

VL6180xIdentification identification;
VL6180x sensor1(VL6180X_ADDRESS);
VL6180x sensor2(VL6180X_ADDRESS);
VL6180x sensor3(VL6180X_ADDRESS);
VL6180x sensor4(VL6180X_ADDRESS);
VL6180x sensor5(VL6180X_ADDRESS);


Servo servoMilk;
Servo servoOrange;
Servo servoMeat;
Servo servoBroccoli;
Servo servoFish;

int servoStartDegree = 0;
float maxWeight = 1000;

// The full length of each servo
float servoMilkFullLength = 13.4;
float servoOrangeFullLength = 12.3;
float servoMeatFullLength = 12.3;
float servoBrocolliFullLength = 12.3;
float servoFishFullLength = 11.9;

// The length of per degree
float servoMilkLengthPerDegree = 0.087;
float servoOrangeLengthPerDegree = 0.072;
float servoMeatLengthPerDegree = 0.072;
float servoBrocolliLengthPerDegree = 0.072;
float servoFishLengthPerDegree = 0.06;

// The max degree of each servo
float servoMilkMaxDegree = 160;
float servoOrangeMaxDegree = 170;
float servoMeatMaxDegree = 170;
float servoBrocolliMaxDegree = 170;
float servoFishMaxDegree = 180;

// The received weight data
float milkWeight = 0.00;
float orangeWeight = 0.00;
float meatWeight = 0.00;
float brocolliWeight = 0.00;
float fishWeight = 0.00;

// The calculated degree
float milkDegree = 0.00;
float orangeDegree = 0.00;
float meatDegree = 0.00;
float brocolliDegree = 0.00;
float fishDegree = 0.00;


// Define touch sensor pins
int milkTouchPin = 2;
int orangeTouchPin = 3;
int meatTouchPin = 4;
int brocolliTouchPin = 5;
int fishTouchPin = 6;

// Touch sensor value
int milkTouchValue = 0;
int orangeTouchValue = 0;
int meatTouchValue = 0;
int brocolliTouchValue = 0;
int fishTouchValue = 0;

// Touch sensor status
boolean milkSelected = false;
boolean orangeSelected = false;
boolean meatSelected = false;
boolean brocolliSelected = false;
boolean fishSelected = false;


// Distance
float milkRackDistance = 0.00;
float orangeRackDistance = 0.00;
float meatRackDistance = 0.00;
float broccoliRackDistance = 0.00;
float fishRackDistance = 0.00;


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
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin();
  delay(100);


  // Servos
  servoMilk.attach(8);
  servoOrange.attach(9);
  servoMeat.attach(10);
  servoBroccoli.attach(11);
  servoFish.attach(12);

  servoMilk.write(servoStartDegree);
  servoOrange.write(servoStartDegree);
  servoMeat.write(servoStartDegree);
  servoBroccoli.write(servoStartDegree);
  servoFish.write(servoStartDegree);


  // Touch sensors
  pinMode(milkTouchPin, INPUT);
  pinMode(orangeTouchPin, INPUT);
  pinMode(meatTouchPin, INPUT);
  pinMode(brocolliTouchPin, INPUT);
  pinMode(fishTouchPin, INPUT);


  // Distance sensors
  pinMode(enable1, OUTPUT);
  pinMode(enable2, OUTPUT);
  pinMode(enable3, OUTPUT);
  pinMode(enable4, OUTPUT);
  //  pinMode(enable5, OUTPUT);

  // Enable sensor 1 pin, disable other pins
  digitalWrite(enable1, HIGH);
  digitalWrite(enable2, LOW);
  digitalWrite(enable3, LOW);
  digitalWrite(enable4, LOW);
  sensor1.getIdentification(&identification); // Retrieve manufacture info from device memory
  printIdentification(&identification); // Helper function to print all the Module information

  if (sensor1.VL6180xInit() != 0) {
    Serial.println("SENSOR 1 FAILED TO INITALIZE"); //Initialize device and check for errors
  };
  sensor1.VL6180xDefautSettings(); //Load default settings to get started.
  delay(1000); // delay 1s
  sensor1.changeAddress(VL6180X_ADDRESS, NEWVL6180X_ADDRESS1);

  // Enable sensor 2 pin, disable other pins
  digitalWrite(enable2, HIGH);
  sensor2.getIdentification(&identification);
  if (sensor2.VL6180xInit() != 0) {
    Serial.println("SENSOR 2 FAILED TO INITALIZE"); //Initialize device and check for errors
  };
  sensor2.changeAddress(VL6180X_ADDRESS, NEWVL6180X_ADDRESS2);

  // Enable sensor 3 pin, disable other pins
  digitalWrite(enable3, HIGH);
  sensor3.getIdentification(&identification);
  if (sensor3.VL6180xInit() != 0) {
    Serial.println("SENSOR 3 FAILED TO INITALIZE"); //Initialize device and check for errors
  };
  sensor3.changeAddress(VL6180X_ADDRESS, NEWVL6180X_ADDRESS3);

  // Enable sensor 4 pin, disable other pins
  digitalWrite(enable4, HIGH);
  sensor4.getIdentification(&identification);
  if (sensor4.VL6180xInit() != 0) {
    Serial.println("SENSOR 4 FAILED TO INITALIZE"); //Initialize device and check for errors
  };
  sensor4.changeAddress(VL6180X_ADDRESS, NEWVL6180X_ADDRESS4);

  // The presettings of each fliter
  srand((unsigned int)time(NULL));
  filter1.config.frequency = 120;
  filter1.config.cutoffSlope = 1;
  filter1.config.derivativeCutoffFrequency = 1;
  filter1.config.minCutoffFrequency = 1;
  SF1eFilterInit(&filter1);

  filter2.config.frequency = 120;
  filter2.config.cutoffSlope = 1;
  filter2.config.derivativeCutoffFrequency = 1;
  filter2.config.minCutoffFrequency = 1;
  SF1eFilterInit(&filter2);

  filter3.config.frequency = 120;
  filter3.config.cutoffSlope = 1;
  filter3.config.derivativeCutoffFrequency = 1;
  filter3.config.minCutoffFrequency = 1;
  SF1eFilterInit(&filter3);

  filter4.config.frequency = 120;
  filter4.config.cutoffSlope = 1;
  filter4.config.derivativeCutoffFrequency = 1;
  filter4.config.minCutoffFrequency = 1;
  SF1eFilterInit(&filter4);

  //  filter5.config.frequency = 120;
  //  filter5.config.cutoffSlope = 1;
  //  filter5.config.derivativeCutoffFrequency = 1;
  //  filter5.config.minCutoffFrequency = 1;
  //  SF1eFilterInit(&filter5);
  //

}

void loop()
{
  // Receive milk weight datarotate servo
  milkDegree = calDegree(milkWeight, servoMilkFullLength, servoMilkLengthPerDegree);
  servoMilk.write(milkDegree);

  // Receive orange weight data, rotate servo
  orangeDegree = calDegree(orangeWeight, servoOrangeFullLength, servoOrangeLengthPerDegree);
  servoOrange.write(orangeDegree);

  // Receive meat weight data, rotate servo
  meatDegree = calDegree(meatWeight, servoMeatFullLength, servoMeatLengthPerDegree);
  servoMeat.write(meatDegree);

  // Receive brocolli weight data, rotate servo
  brocolliDegree = calDegree(brocolliWeight, servoBrocolliFullLength, servoBrocolliLengthPerDegree);
  servoBroccoli.write(brocolliDegree);

  // Receive fish weight data, rotate servo
  fishDegree = calDegree(fishWeight, servoFishFullLength, servoFishLengthPerDegree);
  servoFish.write(fishDegree);


  // Read milk touch sensor pins value and set status
  milkTouchValue = digitalRead(milkTouchPin);
  if (checkTouchStatus(milkTouchValue)) {
    milkSelected = checkSelectedStatus(milkSelected);
    //emit
//    //Test
//    Serial.print(milkSelected);
//    servoMilk.write(100);
//    delay(1000);
//    servoMilk.detach();
  }

  // Read orange touch sensor pins value and set status
  orangeTouchValue = digitalRead(orangeTouchPin);
  if (checkTouchStatus(orangeTouchValue)) {
    orangeSelected = checkSelectedStatus(orangeSelected);
    //emit
  }

  // Read meat touch sensor pins value and set status
  meatTouchValue = digitalRead(meatTouchPin);
  if (checkTouchStatus(meatTouchValue)) {
    meatSelected = checkSelectedStatus(meatSelected);
    //emit
  }

  // Read broccoli touch sensor pins value and set status
  brocolliTouchValue = digitalRead(brocolliTouchPin);
  if (checkTouchStatus(brocolliTouchValue)) {
    brocolliSelected = checkSelectedStatus(brocolliSelected);
    //emit
  }

  // Read fish touch sensor pins value and set status
  fishTouchValue = digitalRead(fishTouchPin);
  if (checkTouchStatus(fishTouchValue)) {
    fishSelected = checkSelectedStatus(fishSelected);
    // emit
  }


  //  Read distance data
  //  Get Sensor Distance and filtered by One Euro filter
  milkRackDistance = SF1eFiltered1(sensor1.getDistance());
//  Serial.print("1 ");
//  Serial.println(milkRackDistance);
//  Serial.println();
  delay(100);
  
  orangeRackDistance = SF1eFiltered2(sensor2.getDistance());
  delay(100);

  meatRackDistance = SF1eFiltered3(sensor3.getDistance());
  delay(100);

  broccoliRackDistance = SF1eFiltered4(sensor4.getDistance());
  delay(100);

//  fishRackDistance = SF1eFiltered5(sensor5.getDistance())

}


// Calculate the degree based on
// received weight data, the full length and the length per degree of the servo
float calDegree(float weight, float fullLength, float lenghPerDegree) {
  float degree = round((weight / maxWeight * fullLength) / lenghPerDegree + 0.5);
  return degree;
}

// Check if touch sensor is touched
boolean checkTouchStatus(int value) {
  boolean touched;
  if (value == 1)
  {
    touched = true;
  } else {
    touched = false;
  }
  return touched;
}

// Check the status is selected or deselected
boolean checkSelectedStatus(boolean selected) {
  if (selected == true) {
    selected = false;
  } else {
    selected = true;
  }
  return selected;
}

void printIdentification(struct VL6180xIdentification *temp) {
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
  //  Serial.print(signal);
  //  Serial.print(" ");
  //  Serial.print(noisy);
  //  Serial.print(" ");
  //  Serial.print(filtered);
  //  Serial.print(" ");
  return filtered;
}

float SF1eFiltered3(float readData)
{
  float signal = readData;
  float randnum = (float)rand() * 1.f / RAND_MAX;
  float noisy = signal + (randnum - 0.5f) / 5.f;
  float filtered = SF1eFilterDo(&filter3, noisy);
  //  Serial.print(signal);
  //  Serial.print(" ");
  //  Serial.print(noisy);
  //  Serial.print(" ");
  //  Serial.print(filtered);
  //  Serial.print(" ");
  return filtered;
}

float SF1eFiltered4(float readData)
{
  float signal = readData;
  float randnum = (float)rand() * 1.f / RAND_MAX;
  float noisy = signal + (randnum - 0.5f) / 5.f;
  float filtered = SF1eFilterDo(&filter4, noisy);
  //  Serial.print(signal);
  //  Serial.print(" ");
  //  Serial.print(noisy);
  //  Serial.print(" ");
  //  Serial.print(filtered);
  //  Serial.print(" ");
  return filtered;
}

//float SF1eFiltered5(float readData)
//{
//  float signal = readData;
//  float randnum = (float)rand() * 1.f / RAND_MAX;
//  float noisy = signal + (randnum - 0.5f) / 5.f;
//  float filtered = SF1eFilterDo(&filter5, noisy);
////  Serial.print(signal);
////  Serial.print(" ");
////  Serial.print(noisy);
////  Serial.print(" ");
////  Serial.print(filtered);
////  Serial.println();
//  return filtered;
//}




