#include <SparkFun_VL6180X.h>
#include <Wire.h>
#include <math.h>
#include <Servo.h>


#define VL6180X_ADDRESS 0x29

VL6180xIdentification identification;
VL6180x sensor(VL6180X_ADDRESS);

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


void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  Wire.begin();
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
  pinMode(brocolliTouchPin, INPUT); 
  pinMode(fishTouchPin, INPUT); 

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
  if(checkTouchStatus(milkTouchValue)){
    milkSelected = checkSelectedStatus(milkSelected);
    //emit
    //Test
    Serial.print(milkSelected);
    servoMilk.write(100);
    delay(1000);
    servoMilk.detach();
  }

  // Read orange touch sensor pins value and set status
  orangeTouchValue = digitalRead(orangeTouchPin);
  if(checkTouchStatus(orangeTouchValue)){
    orangeSelected = checkSelectedStatus(orangeSelected);
    //emit
  }

  // Read meat touch sensor pins value and set status
  meatTouchValue = digitalRead(meatTouchPin);
  if(checkTouchStatus(meatTouchValue)){
    meatSelected = checkSelectedStatus(meatSelected);
    //emit
  }

  // Read broccoli touch sensor pins value and set status
  brocolliTouchValue = digitalRead(brocolliTouchPin);
  if(checkTouchStatus(brocolliTouchValue)){
    brocolliSelected = checkSelectedStatus(brocolliSelected);
    //emit
  }

  // Read fish touch sensor pins value and set status
  fishTouchValue = digitalRead(fishTouchPin);
  if(checkTouchStatus(fishTouchValue)){
    fishSelected = checkSelectedStatus(fishSelected);
    // emit
  }


  // Read distance sensor 
  milkRackDistance =  sensor.getDistance();

}


// Calculate the degree based on 
// received weight data, the full length and the length per degree of the servo
float calDegree(float weight, float fullLength, float lenghPerDegree){
  float degree = round((weight/maxWeight * fullLength) / lenghPerDegree + 0.5);
  return degree;
}

// Check if touch sensor is touched
boolean checkTouchStatus(int value){
  boolean touched;
  if(value == 1)
  {
    touched = true;
  }else{
    touched = false;
  }
  return touched;
}

// Check the status is selected or deselected
boolean checkSelectedStatus(boolean selected){
  if(selected == true){
    selected = false;
  }else{
    selected = true;
  }
  return selected;
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



