/******************************************************************************
  This code controls the nutrient part.
  Five servos represent five different nutrients.
  
 ******************************************************************************/
#define ESP32
#include <SocketIoClient.h>
#include <ESP32_Servo.h>
#include <WiFi.h>
#include <Wire.h>

// The Router that the board connects to
const char* ssid = "TP-LINK_8F96";
const char* password = "49005875";

int servoPin1 = 32;
int servoPin2 = 33;
int servoPin3 = 25;
int servoPin4 = 12;
int servoPin5 = 13;

// socket.io connection
SocketIoClient socket;

// Published values for SG90 servos; adjust if needed
int minUs = 500;
int maxUs = 2400;

Servo servoCalcium;
Servo servoCalories;
Servo servoFat;
Servo servoProtein;
Servo servoVitaminC;

float maxCalcium = 1500.00;
float maxCalories = 3000.00;
float maxFat = 105.00;
float maxProtein = 69.00;
float maxVitaminC = 112.50;

int servoStartDegree = 180;


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
  socket.beginSSL("192.168.0.101", 3000);
  delay(100);

  socket.on("connect", socketConnected);

  // Servos
  servoCalcium.attach(servoPin1, minUs, maxUs);
  servoCalories.attach(servoPin2, minUs, maxUs);
  servoFat.attach(servoPin3, minUs, maxUs);
  servoProtein.attach(servoPin4, minUs, maxUs); 
  servoVitaminC.attach(servoPin5, minUs, maxUs);

}

void loop() {
  // put your main code here, to run repeatedly:
  socket.loop();
  socket.on("connect", socketConnected);

  // Receive new nutrients when weight changes
  socket.on("calciumChanges", setCalcium);
  socket.on("caloriesChanges", setCalories);
  socket.on("fatChanges", setFat);
  socket.on("proteinChanges", setProtein);
  socket.on("vitaminCChanges", setVitaminC);

}


float calDegree(float weight, float maxValue) {
  float degree = round((weight / maxValue) * 180);
  return degree;
}

// Socket IO functions
void socketConnected(const char *message, size_t length) {
  Serial.println("Board connects to server now!");
}

// Convenience functions to set servos; these are used as callbacks for the socket.io messages
void setCalcium(const char *weight, size_t length) {
  servoCalcium.attach(servoPin1, minUs, maxUs);
  // get value from message
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)weight[i];
  }
  int value = data.toInt();

  // calculate value for servo and send it
  float transformedValue = calDegree(value, maxCalcium);
  Serial.println(transformedValue);
  servoCalcium.write(transformedValue); 
  delay(3000);
  servoCalcium.detach();
}

void setCalories(const char *weight, size_t length) {
  servoCalories.attach(servoPin2, minUs, maxUs);
  // get value from message
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)weight[i];
  }
  int value = data.toInt();

  // calculate value for servo and send it
  float transformedValue = calDegree(value, maxCalories);
  Serial.println(transformedValue);
  servoCalories.write(transformedValue); 
  delay(3000);
  servoCalories.detach();
}

void setFat(const char *weight, size_t length) {
  servoFat.attach(servoPin3, minUs, maxUs);
  // get value from message
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)weight[i];
  }
  int value = data.toInt();

  // calculate value for servo and send it
  float transformedValue = calDegree(value, maxFat);
  Serial.println(transformedValue);
  servoFat.write(transformedValue); 
  delay(3000);
  servoFat.detach();
}

void setProtein(const char *weight, size_t length) {
  servoProtein.attach(servoPin4, minUs, maxUs);
  // get value from message
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)weight[i];
  }
  int value = data.toInt();

  // calculate value for servo and send it
  float transformedValue = calDegree(value, maxProtein);
  Serial.println(transformedValue);
  servoProtein.write(transformedValue); 
  delay(3000);
  servoProtein.detach();
}

void setVitaminC(const char *weight, size_t length) {
  servoVitaminC.attach(servoPin5, minUs, maxUs);
  // get value from message
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)weight[i];
  }
  int value = data.toInt();

  // calculate value for servo and send it
  float transformedValue = calDegree(value, maxVitaminC);
  Serial.println(transformedValue);
  servoVitaminC.write(transformedValue); 
  delay(3000);
  servoVitaminC.detach();
}
