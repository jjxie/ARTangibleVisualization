// This code uses 1 euro filter, http://cristal.univ-lille.fr/~casiez/1euro/
#define ESP32
#include <SF1eFilter.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <WiFi.h>
#include <Wire.h>
#include <SocketIoClient.h>
#include "soc/rtc.h"

// The Router that the board connects to
const char* ssid = "TP-LINK_8F96";
const char* password = "49005875";

// socket.io connection
SocketIoClient socket;

// Each servo uses one SF1eFilter structure
SF1eFilter filter1;
SF1eFilter filter2;
SF1eFilter filter3;
SF1eFilter filter4;
//SF1eFilter filter5;

#include <HX711.h>
HX711 scale1;
HX711 scale2;
HX711 scale3;
HX711 scale4;
//HX711 scale5;

//Larger the weight data, smaller the calibration_factor when calibrating the readings
float calibration_factor1 = 2180; //1,2190    2,430    3,415    4,2295
float calibration_factor2 = 428;
float calibration_factor3 = 413;
float calibration_factor4 = 2285;
//float calibration_factor5 = 2010;

float preUnits1 = 0.00;
float preUnits2 = 0.00;
float preUnits3 = 0.00;
float preUnits4 = 0.00;
//float preUnits5 = 0.00;

float units1;
float units2;
float units3;
float units4;
//float units5;

static char unitString1[15];
static char unitString2[15];
static char unitString3[15];
static char unitString4[15];


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
  //ESP32 clock is too fast for hx711, slow it down. Soluntion from this discussion. https://github.com/bogde/HX711/issues/75
  rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
  // parameter "gain" is ommited; the default value 128 is used by the library
  // HX711.DOUT  - pin #A1, FIRST parameter
  // HX711.PD_SCK - pin #A0, SECOND parameter
  // Wemos uses D4,D3, Arduino uses 4,3
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

  scale1.begin(13, 12); //D1,D2  5,4
  scale2.begin(14, 27); //D3,D4  0,2
  scale3.begin(26, 25); // D5,D6  14,12
  scale4.begin(33, 32); // D7,D8  13,15

  scale1.set_scale(calibration_factor1);
  scale1.tare();
  delay(100);
  dtostrf(preUnits1, 4, 2, unitString1);
  socket.emit("milkWeight", unitString1);

  scale2.set_scale(calibration_factor2);
  scale2.tare();
  delay(100);
  dtostrf(preUnits2, 4, 2, unitString2);
  socket.emit("orangeWeight", unitString2);


  scale3.set_scale(calibration_factor3);
  scale3.tare();
  delay(100);
  dtostrf(preUnits3, 4, 2, unitString3);
  socket.emit("meatWeight", unitString3);


  scale4.set_scale(calibration_factor4);
  scale4.tare();
  delay(100);
  dtostrf(preUnits4, 4, 2, unitString4);
  socket.emit("broccoliWeight", unitString4);

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

void loop() {
  socket.loop();
  socket.on("connect", socketConnected);

  // Data from scale number 1
  // If the weight changes more than 3g which is compared to preUnits1
  // Then delay 0.2s, and sequencely read two data, compare the two data
  // If the |second - first| < 1g, then serial print the second, and set preUnits1 = second;
  units1 = scale1.get_units(), 1;
  units1 = initializeScaleData(units1);
  units1 = initializeScaleData(SF1eFiltered1(units1));
//  Serial.print("units1：");
//  Serial.print("  ");
//  Serial.print(units1);
//  Serial.print("    ");
//  Serial.print("preUnits1：");
//  Serial.print("  ");
//  Serial.println(preUnits1);
  if (weightChange(preUnits1, units1)) {
//    Serial.println("More than 3.00  ");
//    Serial.print("preUnits1:  ");
//    Serial.println(preUnits1);
    delay(300);
    int i;
    float readNumber[20];
    float temp1, temp2;
    for (i = 0; i < 20; i++) {
      temp1 = scale1.get_units(), 1;
      temp1 = initializeScaleData(SF1eFiltered1(initializeScaleData(temp1)));
      temp2 = scale1.get_units(), 1;
      temp2 = initializeScaleData(SF1eFiltered1(initializeScaleData(temp2)));
//      Serial.print("temp1: ");
//      Serial.print(temp1);
//      Serial.print("    ");
//      Serial.print("temp2: ");
//      Serial.print(temp2);
      float difference = abs(temp2 - temp1);
//      Serial.print("    ");
//      Serial.println(difference);
      if (abs(temp2 - temp1) < 1.00) {
        Serial.println("Enter < 1 ");
//        Serial.print("A");
//        Serial.println(temp2);
        // Convert to char and emit this data
        dtostrf(temp2, 4, 2, unitString1);
        socket.emit("milkWeight", unitString1);
        delay(10);
        preUnits1 = temp2;
//        Serial.print("Emit  ");
//        Serial.print("  ");
//        Serial.print(unitString1);
//        Serial.print("    ");
//        Serial.print("preUnits1:  ");
//        Serial.println(preUnits1);
        break;
      }
    }
  }

  delay(100);

  // Data from scale number 2
  units2 = scale2.get_units(), 1;
  units2 = initializeScaleData(units2);
  units2 = initializeScaleData(SF1eFiltered2(units2));
  //Serial.println(units2);
  if (weightChange(preUnits2, units2)) {
    delay(300);
    int i;
    float readNumber[20];
    float temp1, temp2;
    for (i = 0; i < 20; i++) {
      temp1 = scale2.get_units(), 1;
      temp1 = initializeScaleData(SF1eFiltered2(initializeScaleData(temp1)));
      temp2 = scale2.get_units(), 1;
      temp2 = initializeScaleData(SF1eFiltered2(initializeScaleData(temp2)));
      if (abs(temp2 - temp1) < 1) {
//        Serial.print("B");
//        Serial.println(temp2);
        dtostrf(temp2, 4, 2, unitString2);
        socket.emit("orangeWeight", unitString2);
        delay(10);
        preUnits2 = temp2;
        break;
      }
    }
  }

  delay(100);
  
  // Data from scale number 3
  units3 = scale3.get_units(), 1;
  units3 = initializeScaleData(units3);
  units3 = initializeScaleData(SF1eFiltered3(units3));
//  Serial.println(units3);
  if (weightChange(preUnits3, units3)) {
    delay(300);
    int i;
    float readNumber[20];
    float temp1, temp2;
    for (i = 0; i < 20; i++) {
      temp1 = scale3.get_units(), 1;
      temp1 = initializeScaleData(SF1eFiltered3(initializeScaleData(temp1)));
      temp2 = scale3.get_units(), 1;
      temp2 = initializeScaleData(SF1eFiltered3(initializeScaleData(temp2)));
      if (abs(temp2 - temp1) < 1) {
//        Serial.print("C");
//        Serial.println(temp2);
        dtostrf(temp2, 4, 2, unitString3);
        socket.emit("meatWeight", unitString3);
        delay(10);
        preUnits3 = temp2;
        break;
      }
    }
  }

  delay(100);
  
  // Data from scale number 4
  units4 = scale4.get_units(), 1;
  units4 = initializeScaleData(units4);
  units4 = initializeScaleData(SF1eFiltered4(units4));
  if (weightChange(preUnits4, units4)) {
    delay(300);
    int i;
    float readNumber[20];
    float temp1, temp2;
    for (i = 0; i < 20; i++) {
      temp1 = scale4.get_units(), 1;
      temp1 = initializeScaleData(SF1eFiltered4(initializeScaleData(temp1)));
      temp2 = scale4.get_units(), 1;
      temp2 = initializeScaleData(SF1eFiltered4(initializeScaleData(temp2)));
      if (abs(temp2 - temp1) < 1) {
//        Serial.print("D");
//        Serial.println(temp2);
        dtostrf(temp2, 4, 2, unitString4);
        socket.emit("broccoliWeight", unitString4);
        delay(10);
        preUnits4 = temp2;
        break;
      }
    }
  }
  
  delay(100);
}

// Check if the reading weight data < 0, then set units to 0.00
// else return reading data
float initializeScaleData(float readData) {
  float result;
  if (readData < 0.00) {
    result = 0.00;
  } else {
    result = readData;
  }
  return result;
}

// If the absolute value of weight changes > 3, then return true, weight changed
boolean weightChange(float preWeight, float readWeight) {
  if (abs(readWeight - preWeight) > 3.00 ) {
    return true;
  } else {
    return false;
  }
}

// 1 Euro filter, IN initializeScaleData(scale1.get_units(), 1), OUT filtered
float SF1eFiltered1(float readData)
{
  float signal = readData;
  float randnum = (float)rand() * 1.f / RAND_MAX;
  float noisy = signal + (randnum - 0.5f) / 5.f;
  float filtered = SF1eFilterDo(&filter1, noisy);
  //  Serial.print(signal);
  //  Serial.print(" ");
  //  Serial.print(noisy);
  //  Serial.print(" ");
  //  Serial.print(filtered);
  //  Serial.print(" ");
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

