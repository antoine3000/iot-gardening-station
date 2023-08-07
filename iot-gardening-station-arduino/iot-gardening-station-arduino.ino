#include "env.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <AHT20.h>

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

AHT20 AHT;
#define PIN_SOIL D1
#define PIN_PUMP D2

int rawADC;
int moistureThreshold = 50;
long moistureAvr = 0;
long moistureReadingCounter = 0;
long lastNotification = 0;

// node-red commands
String nodeOutputSting = mqtt_topic + String(DEVICE) + "/output";
const char* nodeOutput = nodeOutputSting.c_str();
String nodeTemperatureString = mqtt_topic + String(DEVICE) + "/temperature";
const char* nodeTemperature = nodeTemperatureString.c_str();
String nodeHumidityString = mqtt_topic + String(DEVICE) + "/humidity";
const char* nodeHumidity = nodeHumidityString.c_str();
String nodeMoistureString = mqtt_topic + String(DEVICE) + "/moisture";
const char* nodeMoisture = nodeMoistureString.c_str();

void setup() {
  Serial.begin(115200);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_SOIL, INPUT);
  AHT.begin();
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  if (String(topic) == nodeOutput) {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(PIN_PUMP, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      digitalWrite(PIN_PUMP, LOW);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe(nodeOutput);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");      
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  long now = millis();
  
  if (now - lastMsg > 1000) {
    lastMsg = now;
    
    // Temperature and humidity
    float humi, temp;
    int ret = AHT.getSensor(&humi, &temp);
    humi = humi * 100;

    char tempString[8];
    dtostrf(temp, 1, 2, tempString);
    Serial.println("Temperature: " + String(temp) + "°C");
    client.publish(nodeTemperature, tempString);
    
    char humString[8];
    dtostrf(humi, 1, 2, humString);
    Serial.println("Air humidity: " + String(humi) + "%");
    client.publish(nodeHumidity, humString);

    // Soil moisture
    rawADC = analogRead(PIN_SOIL);
    int moistPct = map(rawADC, 2500, 2000, 0, 100);
    if (moistPct > 100) {
      moistPct = 100;
    } else if (moistPct < 0) {
      moistPct = 0;
    }
    Serial.println("Soil moisture: " + String(moistPct) + "%");
    moistureReadingCounter++;
    moistureAvr = moistureAvr + moistPct;

    char moistPctString[8];
    dtostrf(moistPct, 1, 2, moistPctString);
    client.publish(nodeMoisture, moistPctString);

    Serial.println("Counter: " + String(moistureReadingCounter) + "s");
    Serial.println("");

    // take moisture reading for 2 minutes and calculate average
    if (moistureReadingCounter > 120 ) {
      moistPct = (int) (moistureAvr / moistureReadingCounter);
      Serial.println("Soil moisture average: " + String(moistPct) + "%");
      Serial.println("");
      if (moistPct < moistureThreshold ) {
        digitalWrite(PIN_PUMP, HIGH);
        delay(5000);
        digitalWrite(PIN_PUMP, LOW);
      }
      moistureReadingCounter = 0;
      moistureAvr = 0;
    }

  }
}