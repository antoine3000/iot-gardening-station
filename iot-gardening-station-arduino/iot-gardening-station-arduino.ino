#include <Wire.h>
#include <AHT20.h>

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
