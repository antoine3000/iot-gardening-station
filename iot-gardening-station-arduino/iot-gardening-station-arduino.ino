#include <Wire.h>
#include <AHT20.h>

<<<<<<< HEAD
#define DEVICE 1

const char* ssid = env_ssid;
const char* password = env_password;
const char* mqtt_server = env_mqtt_server;
const char* mqtt_topic = env_mqtt_topic;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

=======
>>>>>>> e31cbde851a33433e93b252c1d75a67db32a44cc
AHT20 AHT;
#define PIN_SOIL D1
#define PIN_PUMP D2

int rawADC;
int moistureThreshold = 50; // Moisture level needed for the plant
long moistureAvr = 0;
long moistureReadingCounter = 0;
bool graphMode = false;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_SOIL, INPUT);
  AHT.begin();
}

void loop() {
  // long now = millis();
  // lastMsg = now;
    
  // Air Temperature and humidity
  float humi, temp;
  int ret = AHT.getSensor(&humi, &temp);
  humi = humi * 100;
  if (graphMode == false) {
    Serial.println("Temperature: " + String(temp) + "°C");
    Serial.println("Air humidity: " + String(humi) + "%");
  }


  // Soil moisture
  rawADC = analogRead(PIN_SOIL);
  int moistPct = map(rawADC, 2500, 2000, 0, 100);
  if (moistPct > 100) {
    moistPct = 100;
  } else if (moistPct < 0) {
    moistPct = 0;
  }

  if (graphMode == false) {
    Serial.println("Soil moisture: " + String(moistPct) + "%");
  }

  moistureReadingCounter++;
  moistureAvr = moistureAvr + moistPct;

  if (graphMode == false) {
    Serial.println("Counter: " + String(moistureReadingCounter) + "s");
    Serial.println("");
  }


  // take moisture reading for 2 minutes and calculate average
  if (moistureReadingCounter > 120 ) {
    
    moistPct = (int) (moistureAvr / moistureReadingCounter);
    
    if (graphMode == false) {
      Serial.println("Soil moisture average: " + String(moistPct) + "%");
      Serial.println("");
    }
    
    if (moistPct < moistureThreshold ) {
      digitalWrite(PIN_PUMP, HIGH);
      delay(5000);
      digitalWrite(PIN_PUMP, LOW);
    }

    moistureReadingCounter = 0;
    moistureAvr = 0;
  }

  if (graphMode == true) {
    Serial.println(moistPct); // air temperature, air humidity, soil humidity
  }

}
