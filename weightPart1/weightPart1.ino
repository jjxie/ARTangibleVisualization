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

#define VL6180X_ADDRESS 0x29
#define NEWVL6180X_ADDRESS1 0x30
#define NEWVL6180X_ADDRESS2 0x32

int enable1 = 23;  //AO-A4 Arduino, ESP32 23, SDA: 21 SCL: 22
int enable2 = 17;

VL6180xIdentification identification;
VL6180x sensor1(VL6180X_ADDRESS);
VL6180x sensor2(VL6180X_ADDRESS);


// Published values for SG90 servos; adjust if needed
int minUs = 500;
int maxUs = 2400;

Servo servoMilk;
Servo servoOrange;


// Define servo pins
int milkServoPin = 12;
int orangeServoPin = 13;

int servoStartDegree = 0;
float maxWeight = 1000;

// The full length of each servo
float servoMilkFullLength = 13.4;
float servoOrangeFullLength = 12.3;


// The length of per degree
float servoMilkLengthPerDegree = 0.087;
float servoOrangeLengthPerDegree = 0.072;

// The max degree of each servo
float servoMilkMaxDegree = 160;
float servoOrangeMaxDegree = 170;

// The received weight data
float milkWeight = 0.00;
float orangeWeight = 0.00;


// The calculated degree
float milkDegree = 0.00;
float orangeDegree = 0.00;


// Define touch sensor pins
int milkTouchPin = 33;
int orangeTouchPin = 25;


// Selection status
boolean milkSelected = false;
boolean orangeSelected = false;


// Distance sensors
float milkRackDistance = 0.00;
float orangeRackDistance = 0.00;


// When the servo rotates full degree, the rack distance from the distance sensor
float milkFullDegreeRackDistance = 13.5;
float orangeFullDegreeRackDistance = 13.5;


float milkPreviousRackDistance = 0.00;
float orangePreviousRackDistance = 0.00;


static char milkWeightByMovingRack[15];
static char orangeWeightByMovingRack[15];


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
  servoMilk.write(servoStartDegree);
  servoOrange.write(servoStartDegree);

  // Touch sensors
  pinMode(milkTouchPin, INPUT);
  pinMode(orangeTouchPin, INPUT);
  delay(100);

  // Distance sensors
  pinMode(enable1, OUTPUT);
  pinMode(enable2, OUTPUT);

  // Enable sensor 1 pin, disable other pins
  // Distance sensor 1, initialization and change address
  digitalWrite(enable1, HIGH);
  digitalWrite(enable2, LOW);
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

}

void loop()
{
  socket.loop();
  socket.on("connect", socketConnected);

  // Receive milk weight data and rotate servo
  socket.on("scaleDataMilk", setMilkServo);
  // Receive orange weight data, rotate servo
  socket.on("scaleDataOrange", setOrangeServo);

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


  // Read distance data
  // Get Sensor Distance and filtered by One Euro filter
  milkRackDistance = SF1eFiltered1(sensor1.getDistance());
  // If distance changes more than 2CM, then socket.emit new weight data, and set this distance data as **PreviousRackDistance
  if(checkDistance(milkPreviousRackDistance, milkRackDistance)){
    float milkWeight = calWeight (milkRackDistance, milkFullDegreeRackDistance);
    dtostrf(milkWeight, 4, 2, milkWeightByMovingRack);
    socket.emit("milkWeightByMovingRack", milkWeightByMovingRack);
    milkPreviousRackDistance = milkRackDistance;
  }
  Serial.print("1 ");
  Serial.println(milkRackDistance);
  delay(100);

  orangeRackDistance = SF1eFiltered2(sensor2.getDistance());
  // If distance changes more than 2CM, then socket.emit new weight data, and set this distance data as **PreviousRackDistance
  if(checkDistance(orangePreviousRackDistance, orangeRackDistance)){
    float orangeWeight = calWeight (orangeRackDistance, orangeFullDegreeRackDistance);
    dtostrf(orangeWeight, 4, 2, orangeWeightByMovingRack);
    socket.emit("orangeWeightByMovingRack", orangeWeightByMovingRack);
    orangePreviousRackDistance = orangeRackDistance;
  }
  Serial.print("2 ");
  Serial.println(orangeRackDistance);
  delay(100);
}



// Calculate the rotate degree based on (received weight data, the full length of the servo rack, the length per degree of the servo)
float calDegree(float weight, float fullLength, float lenghPerDegree) {
  float degree = round((weight / maxWeight * fullLength) / lenghPerDegree + 0.5);
  return degree;
}

// Calculate the weight based on the distance of the bottom part of the rack from the distance sensor 
float calWeight(float readingdistance, float fullLength){
  // Transform the data from MM to CM
  readingdistance = readingdistance/10.00;
  float weight = round((readingdistance / fullLength) * maxWeight + 0.5);
  return weight;
}

// Check if the reading distance is more than 2CM than the previous distance data
boolean checkDistance(float previousDistance, float readingDistance){
  if(abs(readingDistance - previousDistance) > 20.00){
    return true;
  }
  else{
    return false;
  }
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
  delay(2000);
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
  delay(2000);
  servoOrange.detach();
}

