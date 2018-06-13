#include <SocketIoClient.h>

#include <SparkFun_VL6180X.h>
#include <Wire.h>
#include <math.h>
#include <Servo.h>
#include <esp_wifi.h>


#define VL6180X_ADDRESS 0x29

VL6180xIdentification identification;
VL6180x sensor(VL6180X_ADDRESS);

// socket.io connection
SocketIoClient socket;

// TODO: setup wifi connection


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
float broccoliWeight = 0.00;
float fishWeight = 0.00;

// The calculated degree
float milkDegree = 0.00;
float orangeDegree = 0.00;
float meatDegree = 0.00;
float broccoliDegree = 0.00;
float fishDegree = 0.00;

// The servo pins
int milkServoPin = 8;
int orangeServoPin = 9;
int meatServoPin = 10;
int broccoliServoPin = 11;
int fishServoPin = 12;
 

// Define touch sensor pins
int milkTouchPin = 2;
int orangeTouchPin = 3;
int meatTouchPin = 4;
int broccoliTouchPin = 5;
int fishTouchPin = 6;

// Touch sensor value
int milkTouchValue = 0;
int orangeTouchValue = 0;
int meatTouchValue = 0;
int broccoliTouchValue = 0;
int fishTouchValue = 0;

// Touch sensor status
boolean milkSelected = false;
boolean orangeSelected = false;
boolean meatSelected = false;
boolean broccoliSelected = false;
boolean fishSelected = false;


// Distance
float milkRackDistance = 0.00;
float orangeRackDistance = 0.00;
float meatRackDistance = 0.00;
float broccoliRackDistance = 0.00;
float fishRackDistance = 0.00;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  Wire.begin();
  socket.begin("192.168.0.xxx", 3000);
  delay(100);

  sensor.getIdentification(&identification); // Retrieve manufacture info from device memory
  printIdentification(&identification); // Helper function to print all the Module information

  // Setting Distance sensor 
  if(sensor.VL6180xInit() != 0){
    Serial.println("FAILED TO INITALIZE"); //Initialize device and check for errors
  }; 
  sensor.VL6180xDefautSettings(); //Load default settings to get started.
  delay(1000); // delay 1s

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
  pinMode(broccoliTouchPin, INPUT); 
  pinMode(fishTouchPin, INPUT); 

  // define socket io callbacks
  socket.on("connect", socketConnected);
  socket.on("scaleDataMilk", setMilkServo);
  socket.on("scaleDataOrange", setOrangeServo);
  socket.on("scaleDataMeat", setMeatServo);
  socket.on("scaleDataBroccoli", setBroccoliServo);
  socket.on("scaleDataFish", setFishServo);

}

void loop() 
{ 
  // we need to read the touch sensors and emit a socket.io message if a status changes
  boolean milk = digitalRead(milkTouchPin);
  if (milk != milkSelected) {
    milkSelected = milk;
    socket.emit("Milk selection status", milk);
  }

  //TODO: do the same for the other 4 sensors























  // Read distance sensor 
  milkRackDistance =  sensor.getDistance();

}


// Calculate the degree based on 
// received weight data, the full length and the length per degree of the servo
float calDegree(float weight, float fullLength, float lenghPerDegree){
  float degree = round((weight/maxWeight * fullLength) / lenghPerDegree + 0.5);
  return degree;
}

// Convenience functions to set servos; these are used as callbacks for the socket.io messages
void setMilkServo(const char *weight, size_t length){
  // get value from message
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)payload[i];
  }
  int value = data.toInt();

  // calculate value for servo and send it
  float transformedValue = calDegree(value, servoMilkFullLength, servoMilkLengthPerDegree);
  attachAndSetServo(servoMilk, milkServoPin, transformedValue);
}

//TODO: add functions for orange, meat, broccoli and fish



























void attachAndSetServo(Servo servo, int pin, int value) {
  servo.attach(pin);
  servo.write(value);
  servo.detach;
}


void socketConnected(const char *weight, size_t length){
  // get value from message
  serial.println("Socket.io server connected.");
}



void printIdentification(struct VL6180xIdentification *temp){
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



