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
#include <Wire.h>
#include <math.h>
#include <ESP32_Servo.h>
#include <WiFi.h>

// The Router that the board connects to
const char* ssid = "TP-LINK_8F96";
const char* password = "49005875";

// socket.io connection
SocketIoClient socket;


// Published values for SG90 servos; adjust if needed
int minUs = 500;
int maxUs = 2400;

Servo servoMilk;
Servo servoOrange;
Servo servoMeat;
Servo servoBroccoli;
Servo servoFish;

// Define servo pins
int milkServoPin = 23;
int orangeServoPin = 18;
int meatServoPin = 27;
int broccoliServoPin = 32;
int fishServoPin= 12;

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
int milkTouchPin = 19;
int orangeTouchPin = 17;
int meatTouchPin = 35;
int broccoliTouchPin = 33;
int fishTouchPin= 13;

// Selection status
boolean milkSelected = false;
boolean orangeSelected = false;
boolean meatSelected = false;
boolean broccoliSelected = false;
boolean fishSelected = false;

static unsigned long lastMillis = 0;


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
  // Mac Network--> WiFi--> Advanced--> TCP/IP 192.168.0.101
  socket.beginSSL("192.168.0.101", 3000);
  delay(100);

  socket.on("connect", socketConnected);

  // Servos
  servoMilk.attach(milkServoPin, minUs, maxUs);
  servoOrange.attach(orangeServoPin, minUs, maxUs);
  servoMeat.attach(meatServoPin, minUs, maxUs);
  servoBroccoli.attach(broccoliServoPin, minUs, maxUs);
  servoFish.attach(fishServoPin,minUs, maxUs);

  servoMilk.write(servoStartDegree);
  servoOrange.write(servoStartDegree);
  servoMeat.write(servoStartDegree);
  servoBroccoli.write(servoStartDegree);
  servoFish.write(servoStartDegree);


  // Touch sensors
  pinMode(milkTouchPin, INPUT);
  pinMode(orangeTouchPin, INPUT);
  pinMode(meatTouchPin, INPUT);
  pinMode(broccoliTouchPin, INPUT);
  pinMode(fishTouchPin, INPUT);

  delay(100);
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
  socket.on("scaleDataFish", setFishServo);

  socket.on("waitServoFinishReset", setFishServoAndReset);

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
    boolean fishTouch = digitalRead(fishTouchPin);
    if (fishTouch != fishSelected) {
      fishSelected = fishTouch;
      if (fishTouch) socket.emit("fish", "\"1\"");
    }

}


// Calculate the rotate degree based on (received weight data, the full length of the servo rack, the length per degree of the servo)
float calDegree(float weight, float fullLength, float lenghPerDegree) {
  float degree = round((weight / maxWeight * fullLength) / lenghPerDegree + 0.5);
  return degree;
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
  delay(3000);
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
  delay(3000);
  servoOrange.detach();
}

// Convenience functions to set servos; these are used as callbacks for the socket.io messages
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
  delay(3000);
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
  delay(3000);
  servoBroccoli.detach();
}

void setFishServo(const char *weight, size_t length) {
  servoFish.attach(fishServoPin, minUs, maxUs);
  // get value from message
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)weight[i];
  }
  int value = data.toInt();

  // calculate value for servo and send it
  float transformedValue = calDegree(fishWeight, servoFishFullLength, servoFishLengthPerDegree);
  Serial.println(transformedValue);
  servoFish.write(transformedValue);
  delay(3000);
  servoFish.detach();
}

void setFishServoAndReset(const char *weight, size_t length) {
  servoFish.attach(fishServoPin, minUs, maxUs);
  // get value from message
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)weight[i];
  }
  int value = data.toInt();

  // calculate value for servo and send it
  float transformedValue = calDegree(fishWeight, servoFishFullLength, servoFishLengthPerDegree);
  Serial.println(transformedValue);
  servoFish.write(transformedValue);
  delay(3000);
  servoFish.detach();
  socket.emit("resetVirutalSceneAgain", "\"1\"");
}
