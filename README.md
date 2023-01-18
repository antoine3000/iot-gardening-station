# Gardening Station

A smart device connected to a plant, which measures the soil moisture level, the ambient temperature and humidity and based on the data collected it waters the plant to meet its needs. The data is published online and a web interface allows direct interaction with the gardening station.

![](medias/IMG_9961.jpg)
![](medias/IMG_9969.jpg)

## Hardware

### Components

- Seeed Studio XIAO ESP32C3
- Grove Shield for Seeed Studio XIAO
- Watering Unit with Mositure Sensor and Pump
- AHT20 Temperature and Humidity Sensor
- 3D-printed enclosure

![](medias/IMG_9924.jpg)

### Assembly instructions

1. Break the XIAO ESP32 at the divider to reduce its size
2. Attach the XIAO ESP32 to the top of the watering unit with a plastic collar
3. Connect the antenna to the XIAO ESP32 and stick it on the side of the water unit
4. Connect the sensor to the board with a Grove cable to pin 4 and 5
4. Connect the water unit to the board with a Grove cable to pin 1 and 2
5. Tape the sensor to the base of the watering unit using double-sided tape
6. 3D print the case and place it on the unit to cover everything

![](medias/IMG_9941.jpg)
![](medias/IMG_9946.jpg)
![](medias/IMG_9949.jpg)

## Software

### Arduino

Arduino allows us to program our board so that it acts as desired. Create a new arduino project and paste the following code into it. It will most likely be necessary to install the few libraries used.

```C++
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <AHT20.h>

#define DEVICE 1

const char* ssid = "*****";
const char* password = "*****";
const char* mqtt_server = "*****";

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
String nodeOutputSting = "esp32-" + String(DEVICE) + "/output";
const char* nodeOutput = nodeOutputSting.c_str();
String nodeTemperatureString = "esp32-" + String(DEVICE) + "/temperature";
const char* nodeTemperature = nodeTemperatureString.c_str();
String nodeHumidityString = "esp32-" + String(DEVICE) + "/humidity";
const char* nodeHumidity = nodeHumidityString.c_str();
String nodeMoistureString = "esp32-" + String(DEVICE) + "/moisture";
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
```

![](medias/IMG_9975.jpg)

### Node-red

An instance of Node-red allows us to connect our devices to a dashboard. The easiest way is to have Node-red installed on a Raspberry Pi. Or on another computer that can stay on. Import the following code into the Node-red dashboard to configure the dashboard and its connections as desired.

![](screenshot-nodered.png)

```json
[
    {
        "id": "df932c172bbbc3c7",
        "type": "tab",
        "label": "Home",
        "disabled": false,
        "info": "",
        "env": []
    },
    {
        "id": "9e58624.7faaba",
        "type": "mqtt out",
        "z": "df932c172bbbc3c7",
        "name": "",
        "topic": "esp32-1/output",
        "qos": "",
        "retain": "",
        "respTopic": "",
        "contentType": "",
        "userProps": "",
        "correl": "",
        "expiry": "",
        "broker": "10e78a89.5b4fd5",
        "x": 560,
        "y": 640,
        "wires": []
    },
    {
        "id": "abf7079a.653be8",
        "type": "mqtt in",
        "z": "df932c172bbbc3c7",
        "name": "",
        "topic": "esp32-1/temperature",
        "qos": "2",
        "datatype": "auto-detect",
        "broker": "10e78a89.5b4fd5",
        "nl": false,
        "rap": false,
        "inputs": 0,
        "x": 344,
        "y": 487,
        "wires": [
            [
                "cc79021b.9a751",
                "21eae8f8.2971b8",
                "fed106f49d2b92cf"
            ]
        ]
    },
    {
        "id": "cc79021b.9a751",
        "type": "debug",
        "z": "df932c172bbbc3c7",
        "name": "",
        "active": true,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "false",
        "x": 531,
        "y": 454,
        "wires": []
    },
    {
        "id": "4aecba01.78ce64",
        "type": "mqtt in",
        "z": "df932c172bbbc3c7",
        "name": "",
        "topic": "esp32-1/humidity",
        "qos": "2",
        "datatype": "auto-detect",
        "broker": "10e78a89.5b4fd5",
        "nl": false,
        "rap": false,
        "inputs": 0,
        "x": 323,
        "y": 371,
        "wires": [
            [
                "22efa7b7.544a28",
                "77147396edee297b"
            ]
        ]
    },
    {
        "id": "22efa7b7.544a28",
        "type": "debug",
        "z": "df932c172bbbc3c7",
        "name": "",
        "active": true,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "false",
        "x": 520,
        "y": 338,
        "wires": []
    },
    {
        "id": "83cf37cf.c76988",
        "type": "ui_switch",
        "z": "df932c172bbbc3c7",
        "name": "",
        "label": "Water pump",
        "tooltip": "",
        "group": "61285987.c20328",
        "order": 6,
        "width": 0,
        "height": 0,
        "passthru": true,
        "decouple": "false",
        "topic": "",
        "topicType": "str",
        "style": "",
        "onvalue": "on",
        "onvalueType": "str",
        "onicon": "",
        "oncolor": "",
        "offvalue": "off",
        "offvalueType": "str",
        "officon": "",
        "offcolor": "",
        "animate": true,
        "className": "",
        "x": 350,
        "y": 640,
        "wires": [
            [
                "9e58624.7faaba"
            ]
        ]
    },
    {
        "id": "21eae8f8.2971b8",
        "type": "ui_chart",
        "z": "df932c172bbbc3c7",
        "name": "",
        "group": "61285987.c20328",
        "order": 4,
        "width": 0,
        "height": 0,
        "label": "Temperature history",
        "chartType": "line",
        "legend": "false",
        "xformat": "dd HH:mm",
        "interpolate": "linear",
        "nodata": "",
        "dot": false,
        "ymin": "",
        "ymax": "",
        "removeOlder": "4",
        "removeOlderPoints": "",
        "removeOlderUnit": "604800",
        "cutout": 0,
        "useOneColor": false,
        "useUTC": false,
        "colors": [
            "#1f77b4",
            "#aec7e8",
            "#ff7f0e",
            "#2ca02c",
            "#97df89",
            "#d62728",
            "#ff9896",
            "#9467bd",
            "#c5b0d5"
        ],
        "outputs": 1,
        "useDifferentColor": false,
        "className": "",
        "x": 560,
        "y": 520,
        "wires": [
            []
        ]
    },
    {
        "id": "b4d3ab21b536b767",
        "type": "mqtt in",
        "z": "df932c172bbbc3c7",
        "name": "",
        "topic": "esp32-1/moisture",
        "qos": "2",
        "datatype": "auto-detect",
        "broker": "10e78a89.5b4fd5",
        "nl": false,
        "rap": false,
        "inputs": 0,
        "x": 290,
        "y": 180,
        "wires": [
            [
                "747f19c5bd7bdc85",
                "52d261579eeb9287",
                "05e4b3dce079b347"
            ]
        ]
    },
    {
        "id": "747f19c5bd7bdc85",
        "type": "debug",
        "z": "df932c172bbbc3c7",
        "name": "",
        "active": true,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "payload",
        "targetType": "msg",
        "statusVal": "",
        "statusType": "auto",
        "x": 570,
        "y": 140,
        "wires": []
    },
    {
        "id": "fed106f49d2b92cf",
        "type": "ui_text",
        "z": "df932c172bbbc3c7",
        "group": "61285987.c20328",
        "order": 2,
        "width": 0,
        "height": 0,
        "name": "",
        "label": "Temperature",
        "format": "{{value}} °C",
        "layout": "row-spread",
        "className": "",
        "x": 770,
        "y": 500,
        "wires": []
    },
    {
        "id": "77147396edee297b",
        "type": "ui_text",
        "z": "df932c172bbbc3c7",
        "group": "61285987.c20328",
        "order": 3,
        "width": 0,
        "height": 0,
        "name": "",
        "label": "Air humidity",
        "format": "{{value}} %",
        "layout": "row-spread",
        "className": "",
        "x": 750,
        "y": 380,
        "wires": []
    },
    {
        "id": "52d261579eeb9287",
        "type": "ui_text",
        "z": "df932c172bbbc3c7",
        "group": "61285987.c20328",
        "order": 1,
        "width": 0,
        "height": 0,
        "name": "",
        "label": "Soil moisture",
        "format": "{{value}} %",
        "layout": "row-spread",
        "className": "",
        "x": 750,
        "y": 240,
        "wires": []
    },
    {
        "id": "05e4b3dce079b347",
        "type": "ui_chart",
        "z": "df932c172bbbc3c7",
        "name": "",
        "group": "61285987.c20328",
        "order": 5,
        "width": 0,
        "height": 0,
        "label": "Soil moisture history",
        "chartType": "line",
        "legend": "false",
        "xformat": "dd HH:mm",
        "interpolate": "linear",
        "nodata": "",
        "dot": false,
        "ymin": "",
        "ymax": "",
        "removeOlder": "4",
        "removeOlderPoints": "",
        "removeOlderUnit": "604800",
        "cutout": 0,
        "useOneColor": false,
        "useUTC": false,
        "colors": [
            "#1f77b4",
            "#aec7e8",
            "#ff7f0e",
            "#3bd43d",
            "#98df8a",
            "#d62728",
            "#ff9896",
            "#9467bd",
            "#c5b0d5"
        ],
        "outputs": 1,
        "useDifferentColor": false,
        "className": "",
        "x": 540,
        "y": 260,
        "wires": [
            []
        ]
    },
    {
        "id": "41039e722cf4be71",
        "type": "mqtt out",
        "z": "df932c172bbbc3c7",
        "name": "",
        "topic": "esp32-2/output",
        "qos": "",
        "retain": "",
        "respTopic": "",
        "contentType": "",
        "userProps": "",
        "correl": "",
        "expiry": "",
        "broker": "10e78a89.5b4fd5",
        "x": 1460,
        "y": 620,
        "wires": []
    },
    {
        "id": "32cf987e2062cb16",
        "type": "mqtt in",
        "z": "df932c172bbbc3c7",
        "name": "",
        "topic": "esp32-2/temperature",
        "qos": "2",
        "datatype": "auto-detect",
        "broker": "10e78a89.5b4fd5",
        "nl": false,
        "rap": false,
        "inputs": 0,
        "x": 1244,
        "y": 467,
        "wires": [
            [
                "c75cf8d07e342c81",
                "9ab8ea895d73a025",
                "1f5c89723135b74b"
            ]
        ]
    },
    {
        "id": "c75cf8d07e342c81",
        "type": "debug",
        "z": "df932c172bbbc3c7",
        "name": "",
        "active": true,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "false",
        "x": 1431,
        "y": 434,
        "wires": []
    },
    {
        "id": "f2c278f53a626bd2",
        "type": "mqtt in",
        "z": "df932c172bbbc3c7",
        "name": "",
        "topic": "esp32-2/humidity",
        "qos": "2",
        "datatype": "auto-detect",
        "broker": "10e78a89.5b4fd5",
        "nl": false,
        "rap": false,
        "inputs": 0,
        "x": 1223,
        "y": 351,
        "wires": [
            [
                "a710f103559dc5d4",
                "af459fd57d6af5e5"
            ]
        ]
    },
    {
        "id": "a710f103559dc5d4",
        "type": "debug",
        "z": "df932c172bbbc3c7",
        "name": "",
        "active": true,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "false",
        "x": 1420,
        "y": 318,
        "wires": []
    },
    {
        "id": "cd78384089692716",
        "type": "ui_switch",
        "z": "df932c172bbbc3c7",
        "name": "",
        "label": "Water pump",
        "tooltip": "",
        "group": "8358512e0101275b",
        "order": 6,
        "width": 0,
        "height": 0,
        "passthru": true,
        "decouple": "false",
        "topic": "",
        "topicType": "str",
        "style": "",
        "onvalue": "on",
        "onvalueType": "str",
        "onicon": "",
        "oncolor": "",
        "offvalue": "off",
        "offvalueType": "str",
        "officon": "",
        "offcolor": "",
        "animate": true,
        "className": "",
        "x": 1250,
        "y": 620,
        "wires": [
            [
                "41039e722cf4be71"
            ]
        ]
    },
    {
        "id": "9ab8ea895d73a025",
        "type": "ui_chart",
        "z": "df932c172bbbc3c7",
        "name": "",
        "group": "8358512e0101275b",
        "order": 4,
        "width": 0,
        "height": 0,
        "label": "Temperature history",
        "chartType": "line",
        "legend": "false",
        "xformat": "dd HH:mm",
        "interpolate": "linear",
        "nodata": "",
        "dot": false,
        "ymin": "",
        "ymax": "",
        "removeOlder": "4",
        "removeOlderPoints": "",
        "removeOlderUnit": "604800",
        "cutout": 0,
        "useOneColor": false,
        "useUTC": false,
        "colors": [
            "#1f77b4",
            "#aec7e8",
            "#ff7f0e",
            "#2ca02c",
            "#97df89",
            "#d62728",
            "#ff9896",
            "#9467bd",
            "#c5b0d5"
        ],
        "outputs": 1,
        "useDifferentColor": false,
        "className": "",
        "x": 1460,
        "y": 500,
        "wires": [
            []
        ]
    },
    {
        "id": "6e322f98adfb150d",
        "type": "mqtt in",
        "z": "df932c172bbbc3c7",
        "name": "",
        "topic": "esp32-2/moisture",
        "qos": "2",
        "datatype": "auto-detect",
        "broker": "10e78a89.5b4fd5",
        "nl": false,
        "rap": false,
        "inputs": 0,
        "x": 1190,
        "y": 160,
        "wires": [
            [
                "2213c001a2e269d4",
                "fce11778047c4a2b"
            ]
        ]
    },
    {
        "id": "1f5c89723135b74b",
        "type": "ui_text",
        "z": "df932c172bbbc3c7",
        "group": "8358512e0101275b",
        "order": 2,
        "width": 0,
        "height": 0,
        "name": "",
        "label": "Temperature",
        "format": "{{value}} °C",
        "layout": "row-spread",
        "className": "",
        "x": 1670,
        "y": 480,
        "wires": []
    },
    {
        "id": "af459fd57d6af5e5",
        "type": "ui_text",
        "z": "df932c172bbbc3c7",
        "group": "8358512e0101275b",
        "order": 3,
        "width": 0,
        "height": 0,
        "name": "",
        "label": "Air humidity",
        "format": "{{value}} %",
        "layout": "row-spread",
        "className": "",
        "x": 1650,
        "y": 360,
        "wires": []
    },
    {
        "id": "2213c001a2e269d4",
        "type": "ui_text",
        "z": "df932c172bbbc3c7",
        "group": "8358512e0101275b",
        "order": 1,
        "width": 0,
        "height": 0,
        "name": "",
        "label": "Soil moisture",
        "format": "{{value}} %",
        "layout": "row-spread",
        "className": "",
        "x": 1650,
        "y": 220,
        "wires": []
    },
    {
        "id": "fce11778047c4a2b",
        "type": "ui_chart",
        "z": "df932c172bbbc3c7",
        "name": "",
        "group": "8358512e0101275b",
        "order": 5,
        "width": 0,
        "height": 0,
        "label": "Soil moisture history",
        "chartType": "line",
        "legend": "false",
        "xformat": "dd HH:mm",
        "interpolate": "linear",
        "nodata": "",
        "dot": false,
        "ymin": "",
        "ymax": "",
        "removeOlder": "4",
        "removeOlderPoints": "",
        "removeOlderUnit": "604800",
        "cutout": 0,
        "useOneColor": false,
        "useUTC": false,
        "colors": [
            "#1f77b4",
            "#aec7e8",
            "#ff7f0e",
            "#3bd43d",
            "#98df8a",
            "#d62728",
            "#ff9896",
            "#9467bd",
            "#c5b0d5"
        ],
        "outputs": 1,
        "useDifferentColor": false,
        "className": "",
        "x": 1440,
        "y": 240,
        "wires": [
            []
        ]
    },
    {
        "id": "10e78a89.5b4fd5",
        "type": "mqtt-broker",
        "name": "",
        "broker": "localhost",
        "port": "1883",
        "clientid": "",
        "usetls": false,
        "compatmode": true,
        "keepalive": "60",
        "cleansession": true,
        "birthTopic": "",
        "birthQos": "0",
        "birthPayload": "",
        "closeTopic": "",
        "closeQos": "0",
        "closePayload": "",
        "willTopic": "",
        "willQos": "0",
        "willPayload": ""
    },
    {
        "id": "61285987.c20328",
        "type": "ui_group",
        "name": "Plant 01",
        "tab": "e7c46d5e.a1283",
        "order": 1,
        "disp": true,
        "width": "6",
        "collapse": true,
        "className": ""
    },
    {
        "id": "8358512e0101275b",
        "type": "ui_group",
        "name": "Plant 02",
        "tab": "e7c46d5e.a1283",
        "order": 2,
        "disp": true,
        "width": "6",
        "collapse": true,
        "className": ""
    },
    {
        "id": "e7c46d5e.a1283",
        "type": "ui_tab",
        "name": "Gardening Station",
        "icon": "fa-pagelines",
        "disabled": false,
        "hidden": false
    }
]
```

![](medias/IMG_9967.jpg)
