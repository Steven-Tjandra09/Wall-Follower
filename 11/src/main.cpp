#include <WiFi.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include <ArduinoJson.h> 
#include <math.h>

const int IN1 = 25; 
const int IN2 = 26; 
const int ENA = 33; 

const int IN3 = 27; 
const int IN4 = 14; 
const int ENB = 32;

const int TRIG_FRONT = 17;
const int ECHO_FRONT = 16;
const int TRIG_REAR = 19;
const int ECHO_REAR = 18;

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

const float SENSOR_ANGLE_DEG = 17.5; 

const char* ssid = "ESP32_WallFollower";         
const char* password = "password123!";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebSocketsServer webSocket = WebSocketsServer(81); 

float frontDistance = 0;
float sideDistance = 0;
float heading = 0;
float distUS_A = 0;
float distUS_B = 0;

unsigned long previousMillis = 0;
const long interval = 100;

bool isAutonomous = false;
bool loxReady = false;

const float TARGET_DISTANCE = 15.0; 
const float TOLERANCE = 2.0; 

void drive_control(int status) {
  if (status == 1) { 
    digitalWrite(IN1, HIGH); 
    digitalWrite(IN2, LOW); 
    analogWrite(ENA, 180); 
  } 
  else if (status == 2) { 
    digitalWrite(IN1, LOW); 
    digitalWrite(IN2, HIGH); 
    analogWrite(ENA, 180); 
  } 
  else { 
    digitalWrite(IN1, LOW); 
    digitalWrite(IN2, LOW); 
    analogWrite(ENA, 0); 
  } 
}

void steer_control(int status) {
  if (status == 1) { 
    digitalWrite(IN3, HIGH); 
    digitalWrite(IN4, LOW); 
    analogWrite(ENB, 255); 
  }
  else if (status == 2) { 
    digitalWrite(IN3, LOW); 
    digitalWrite(IN4, HIGH); 
    analogWrite(ENB, 255); 
  }
  else { 
    digitalWrite(IN3, LOW); 
    digitalWrite(IN4, LOW); 
    analogWrite(ENB, 0);
  }
}

float readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return 0;
  return 0.017 * duration; 
}

void autonomousWallFollower() {
  if (frontDistance > 0 && frontDistance < 20.0) {
    drive_control(2);
    steer_control(2);  // was 1 (left), now 2 (right)
    return;
  }
  
  drive_control(1); 

  if (sideDistance < (TARGET_DISTANCE - TOLERANCE)) {
    steer_control(2);  // was 1 (left), now 2 (right)
  }
  else if (sideDistance > (TARGET_DISTANCE + TOLERANCE)) {
    steer_control(1);  // was 2 (right), now 1 (left)
  }
  else if (heading > 2.0) {
    steer_control(2);  // was 1 (left), now 2 (right)
  }
  else if (heading < -2.0) {
    steer_control(1);  // was 2 (right), now 1 (left)
  }
  else {
    steer_control(0);
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      if (doc.containsKey("auto")) {
        isAutonomous = (doc["auto"] == 1);
        if (!isAutonomous) {
          drive_control(0);
          steer_control(0);
        }
      }
      
      if (!isAutonomous) {
        if (doc.containsKey("drive")) drive_control(doc["drive"]);
        if (doc.containsKey("steer")) steer_control(doc["steer"]);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(IN1, OUTPUT); 
  pinMode(IN2, OUTPUT); 
  pinMode(ENA, OUTPUT);

  pinMode(IN3, OUTPUT); 
  pinMode(IN4, OUTPUT); 
  pinMode(ENB, OUTPUT);

  pinMode(TRIG_FRONT, OUTPUT); 
  pinMode(ECHO_FRONT, INPUT);
  pinMode(TRIG_REAR, OUTPUT); 
  pinMode(ECHO_REAR, INPUT);
  
  Wire.begin(22, 21);  // SDA = GPIO21, SCL = GPIO22
  loxReady = lox.begin();
  if (!loxReady) {
    Serial.println("WARNING: VL53L0X not found, continuing without it.");
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password, 6);
  delay(2000);

  Serial.print("AP started. IP: ");
  Serial.println(WiFi.softAPIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    if (loxReady) {
      VL53L0X_RangingMeasurementData_t measure;
      lox.rangingTest(&measure, false);
      if (measure.RangeStatus != 4) frontDistance = measure.RangeMilliMeter / 10.0;
    }
    
    float rawDistUS_A = readUltrasonic(TRIG_FRONT, ECHO_FRONT);
    float rawDistUS_B = readUltrasonic(TRIG_REAR, ECHO_REAR);
    float angleRad = SENSOR_ANGLE_DEG * PI / 180.0;
    
    distUS_A = rawDistUS_A * cos(angleRad);
    distUS_B = rawDistUS_B * cos(angleRad);
    
    sideDistance = 0.5 * (distUS_A + distUS_B);
    heading = distUS_A - distUS_B;

    if (isAutonomous) {
      autonomousWallFollower();
    }
    
    StaticJsonDocument<200> doc;
    doc["front"] = frontDistance;
    doc["usFront"] = distUS_A;
    doc["usRear"] = distUS_B;
    doc["isAuto"] = isAutonomous ? 1 : 0;
    
    char jsonString[200];
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);
  }
}
