/******************************************************************************
   SparkFun_VL6180X, https://github.com/sparkfun/SparkFun_ToF_Range_Finder-VL6180_Arduino_Library

   Multiple VL6180 solution from https://learn.sparkfun.com/tutorials/vl6180-hookup-guide/discuss
   Add _i2caddress = new_address; to in Sparkfun_VL6180X.cpp in here:
   … VL6180x_setRegister(VL6180X_I2C_SLAVE_DEVICE_ADDRESS, new_address); i2caddress = new_address;
   return VL6180x_getRegister(VL6180X_I2C_SLAVE_DEVICE_ADDRESS); }

   Add one Euro filter to reduce distance reading jitter, http://cristal.univ-lille.fr/~casiez/1euro/

   SocketIoClient, have to comment // hexdump(payload, length); in the SocketIoClient.cpp file to avoid error.
   I2C: SDA 21  SCL 22

   Secure SSL connection, Socket.beginSSL("192.168.0.100", 3000);
 ******************************************************************************/
#define ESP32
#include <SocketIoClient.h>

#include <SF1eFilter.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <SparkFun_VL6180X.h>
#include <Wire.h>
#include <math.h>
#include <ESP32_Servo.h>
#include <WiFi.h>

// The Router that the board connects to
const char* ssid = "TP-LINK_8F96";
const char* password = "49005875";

// socket.io connection
SocketIoClient socket;

// Each distance sensor uses one SF1eFilter structure
SF1eFilter filter1;
SF1eFilter filter2;
SF1eFilter filter3;
SF1eFilter filter4;
//SF1eFilter filter5;

#define VL6180X_ADDRESS 0x29
#define NEWVL6180X_ADDRESS1 0x30
#define NEWVL6180X_ADDRESS2 0x32
#define NEWVL6180X_ADDRESS3 0x34
#define NEWVL6180X_ADDRESS4 0x36
//#define NEWVL6180X_ADDRESS5 0x38

int enable1 = 23;  //AO-A4 Arduino, ESP32 23
int enable2 = 17;
int enable3 = 18;
int enable4 = 19;
//int enable5 = A4;

VL6180xIdentification identification;
VL6180x sensor1(VL6180X_ADDRESS);
VL6180x sensor2(VL6180X_ADDRESS);
VL6180x sensor3(VL6180X_ADDRESS);
VL6180x sensor4(VL6180X_ADDRESS);
//VL6180x sensor5(VL6180X_ADDRESS);

// Published values for SG90 servos; adjust if needed
int minUs = 500;
int maxUs = 2400;

Servo servoMilk;
Servo servoOrange;
Servo servoMeat;
Servo servoBroccoli;
//Servo servoFish;

// Define servo pins
int milkServoPin = 12;
int orangeServoPin = 13;
int meatServoPin = 15;
int broccoliServoPin = 16;
//int fishServoPin = 32;

int servoStartDegree = 0;
float maxWeight = 1000;

// The full length of each servo
float servoMilkFullLength = 13.4;
float servoOrangeFullLength = 12.3;
float servoMeatFullLength = 12.3;
float servoBrocolliFullLength = 12.3;
//float servoFishFullLength = 11.9;

// The length of per degree
float servoMilkLengthPerDegree = 0.087;
float servoOrangeLengthPerDegree = 0.072;
float servoMeatLengthPerDegree = 0.072;
float servoBrocolliLengthPerDegree = 0.072;
//float servoFishLengthPerDegree = 0.06;

// The max degree of each servo
float servoMilkMaxDegree = 160;
float servoOrangeMaxDegree = 170;
float servoMeatMaxDegree = 170;
float servoBrocolliMaxDegree = 170;
//float servoFishMaxDegree = 180;

// The received weight data
float milkWeight = 0.00;
float orangeWeight = 0.00;
float meatWeight = 0.00;
float brocolliWeight = 0.00;
//float fishWeight = 0.00;

// The calculated degree
float milkDegree = 0.00;
float orangeDegree = 0.00;
float meatDegree = 0.00;
float brocolliDegree = 0.00;
//float fishDegree = 0.00;


// Define touch sensor pins
int milkTouchPin = 33;
int orangeTouchPin = 25;
int meatTouchPin = 26;
int broccoliTouchPin = 27;
//int fishTouchPin = 14;

// Selection status
boolean milkSelected = false;
boolean orangeSelected = false;
boolean meatSelected = false;
boolean broccoliSelected = false;
//boolean fishSelected = false;


// Distance
float milkRackDistance = 0.00;
float orangeRackDistance = 0.00;
float meatRackDistance = 0.00;
float broccoliRackDistance = 0.00;
//float fishRackDistance = 0.00;

static unsigned long lastMillis = 0;

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

  // connect to wifi rounter in the environment
  Serial.print("Connecting to WiFi network ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("Connected to the WiFi network");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Begin a secure socket, find your computer WiFi TCP/IP Address
  // Mac Network--> WiFi--> Advanced--> TCP/IP 192.168.0.100
  socket.beginSSL("192.168.0.100", 3000);
  delay(100);

  socket.on("connect", socketConnected);

  // Servos
  servoMilk.attach(milkServoPin, minUs, maxUs);
  servoOrange.attach(orangeServoPin, minUs, maxUs);
  servoMeat.attach(meatServoPin, minUs, maxUs);
  servoBroccoli.attach(broccoliServoPin, minUs, maxUs);
  //  servoFish.attach(fishServoPin);

  servoMilk.write(servoStartDegree);
  servoOrange.write(servoStartDegree);
  servoMeat.write(servoStartDegree);
  servoBroccoli.write(servoStartDegree);
  //  servoFish.write(servoStartDegree);


  // Touch sensors
  pinMode(milkTouchPin, INPUT);
  pinMode(orangeTouchPin, INPUT);
  pinMode(meatTouchPin, INPUT);
  pinMode(broccoliTouchPin, INPUT);
  //  pinMode(fishTouchPin, INPUT);


  // Distance sensors
  pinMode(enable1, OUTPUT);
  pinMode(enable2, OUTPUT);
  pinMode(enable3, OUTPUT);
  pinMode(enable4, OUTPUT);
  //    pinMode(enable5, OUTPUT);

  // Enable sensor 1 pin, disable other pins
  // Distance sensor 1, initialization and change address
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

  // Enable distance sensor 2 pin, initialization and change address
  digitalWrite(enable2, HIGH);
  sensor2.getIdentification(&identification);
  if (sensor2.VL6180xInit() != 0) {
    Serial.println("SENSOR 2 FAILED TO INITALIZE"); //Initialize device and check for errors
  };
  sensor2.VL6180xDefautSettings(); //Load default settings to get started.
  delay(1000); // delay 1s
  sensor2.changeAddress(VL6180X_ADDRESS, NEWVL6180X_ADDRESS2);

  // Enable distance sensor 3 pin, initialization and change address
  digitalWrite(enable3, HIGH);
  sensor3.getIdentification(&identification);
  if (sensor3.VL6180xInit() != 0) {
    Serial.println("SENSOR 3 FAILED TO INITALIZE"); //Initialize device and check for errors
  };
  sensor3.VL6180xDefautSettings(); //Load default settings to get started.
  delay(1000); // delay 1s
  sensor3.changeAddress(VL6180X_ADDRESS, NEWVL6180X_ADDRESS3);

  // Enable sensor 4 pin, initialization and change address
  digitalWrite(enable4, HIGH);
  sensor4.getIdentification(&identification);
  if (sensor4.VL6180xInit() != 0) {
    Serial.println("SENSOR 4 FAILED TO INITALIZE"); //Initialize device and check for errors
  };
  sensor4.VL6180xDefautSettings(); //Load default settings to get started.
  delay(1000); // delay 1s
  sensor4.changeAddress(VL6180X_ADDRESS, NEWVL6180X_ADDRESS4);

  // The presettings of each fliter for distance sensor reading to reduce jitter
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
  socket.loop();
  socket.on("connect", socketConnected);

  // Receive milk weight data and rotate servo
  socket.on("scaleDataMilk", setMilkServo);

  // Receive orange weight data, rotate servo
  socket.on("scaleDataOrange", setOrangeServo);

  // Receive meat weight data, rotate servo
  socket.on("scaleDataMeat", setMeatServo);

  // Receive broccoli weight data, rotate servo
  socket.on("scaleDataBroccoli", setBroccoliServo);

  // Receive fish weight data, rotate servo
  //  socket.on("scaleDataFish", setFishServo);


  // Read milk touch sensor pins, emit 1 when touch the sensor
  boolean milkTouch = digitalRead(milkTouchPin);
  if (milkTouch != milkSelected) {
    milkSelected = milkTouch;
    if (milkTouch) socket.emit("milk", "\"1\"");
  }

  // Read orange touch sensor pins value and set status
  boolean orangeTouch = digitalRead(orangeTouchPin);
  if (orangeTouch != orangeSelected) {
    orangeSelected = orangeTouch;
    if (orangeTouch) socket.emit("orange", "\"1\"");
  }

  // Read meat touch sensor pins value and set status
  boolean meatTouch = digitalRead(meatTouchPin);
  if (meatTouch != meatSelected) {
    meatSelected = meatTouch;
    if (meatTouch) socket.emit("meat", "\"1\"");
  }

  // Read broccoli touch sensor pins value and set status
  boolean broccoliTouch = digitalRead(broccoliTouchPin);
  if (broccoliTouch != broccoliSelected) {
    broccoliSelected = broccoliTouch;
    if (broccoliTouch) socket.emit("broccoli", "\"1\"");
  }

  // Read fish touch sensor pins value and set status
  //  boolean fishTouch = digitalRead(fishTouchPin);
  //  if (fishTouch != fishSelected) {
  //    fishSelected = fishTouch;
  //    if (fishTouch) socket.emit("fish", "\"1\"");
  //  }


  // Read distance data
  // Get Sensor Distance and filtered by One Euro filter
  milkRackDistance = SF1eFiltered1(sensor1.getDistance());
  Serial.print("1 ");
  Serial.println(milkRackDistance);
  delay(100);

  orangeRackDistance = SF1eFiltered2(sensor2.getDistance());
  Serial.print("2 ");
  Serial.println(orangeRackDistance);
  delay(100);

  meatRackDistance = SF1eFiltered3(sensor3.getDistance());
  Serial.print("3 ");
  Serial.println(meatRackDistance);
  delay(100);

  broccoliRackDistance = SF1eFiltered4(sensor4.getDistance());
  Serial.print("4 ");
  Serial.println(broccoliRackDistance);
  delay(100);

  //  fishRackDistance = SF1eFiltered5(sensor5.getDistance())

}


// Calculate the rotate degree based on (received weight data, the full length of the servo rack, the length per degree of the servo)
float calDegree(float weight, float fullLength, float lenghPerDegree) {
  float degree = round((weight / maxWeight * fullLength) / lenghPerDegree + 0.5);
  return degree;
}

// Distance sensor indentification
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

// 1 Euro filter, IN initializeScaleData(scale1.get_units(), 1), OUT filtered data
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


// Socket IO functions
void socketConnected(const char *message, size_t length) {
  Serial.println("Board connects to server now!");
}

// Convenience functions to set servos; these are used as callbacks for the socket.io messages
void setMilkServo(const char *weight, size_t length) {
  servoMilk.attach(milkServoPin, minUs, maxUs);
  // get value from message
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)weight[i];
  }
  int value = data.toInt();

  // calculate value for servo and send it
  float transformedValue = calDegree(value, servoMilkFullLength, servoMilkLengthPerDegree);
  //  attachAndSetServo(servoMilk,milkServoPin,transformedValue);
  Serial.print(value);
  Serial.print("  ");
  Serial.println(transformedValue);
  servoMilk.write(transformedValue);
  delay(200);
  servoMilk.detach();
}

void setOrangeServo(const char *weight, size_t length) {
  servoOrange.attach(orangeServoPin, minUs, maxUs);
  // get value from message
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)weight[i];
  }
  int value = data.toInt();

  // calculate value for servo and send it
  float transformedValue = calDegree(value, servoOrangeFullLength, servoOrangeLengthPerDegree);
  //  attachAndSetServo(servoMilk,milkServoPin,transformedValue);
  Serial.print(value);
  Serial.print("  ");
  Serial.println(transformedValue);
  servoOrange.write(transformedValue);
  delay(200);
  servoOrange.detach();
}

void setMeatServo(const char *weight, size_t length) {
  servoMeat.attach(meatServoPin, minUs, maxUs);
  // get value from message
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)weight[i];
  }
  int value = data.toInt();

  // calculate value for servo and send it
  float transformedValue = calDegree(value, servoMeatFullLength, servoMeatLengthPerDegree);
  Serial.print(value);
  Serial.print("  ");
  Serial.println(transformedValue);
  servoMeat.write(transformedValue);
  delay(200);
  servoMeat.detach();
}

void setBroccoliServo(const char *weight, size_t length) {
  servoBroccoli.attach(broccoliServoPin, minUs, maxUs);
  // get value from message
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)weight[i];
  }
  int value = data.toInt();

  // calculate value for servo and send it
  float transformedValue = calDegree(value, servoBrocolliFullLength, servoBrocolliLengthPerDegree);
  Serial.print(value);
  Serial.print("  ");
  Serial.println(transformedValue);
  servoBroccoli.write(transformedValue);
  delay(200);
  servoBroccoli.detach();
}

//void setFishServo(const char *weight, size_t length) {
//  servoFish.attach(fishServoPin);
//  // get value from message
//  String data;
//  for (int i = 0; i < length; i++) {
//    data += (char)weight[i];
//  }
//  int value = data.toInt();
//
//  // calculate value for servo and send it
//  float transformedValue = calDegree(fishWeight, servoFishFullLength, servoFishLengthPerDegree);
//  Serial.println(transformedValue);
//  servoFish.write(transformedValue);
//  delay(100);
//  servoFish.detach();
//}




