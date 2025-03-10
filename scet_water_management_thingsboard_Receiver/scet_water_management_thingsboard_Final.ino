// SCET LoRa receiver code
// tank height 7 ft. 
// min water level = 25 cm and max = 190 cm. 

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Pin definitions for LoRa module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 23
#define DIO0 26
#define BAND 866E6

// Pin definitions for OLED display
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST 23
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Pin definition for pump
#define pump 12

// WiFi credentials
const char* ssid = "TP-Link_scet";
const char* password = "Scetngp@2024";

// ThingsBoard credentials
const char* mqtt_server = "demo.thingsboard.io";
const char* access_token = "KQ3Pv8elbKKsdvSjCRxU";

WiFiClient espClient;
PubSubClient client(espClient);

bool flag = 0;
int Tank_Level_cms = 0;

// OLED display setup
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void startOLED() {
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("LORA RECEIVER");
  display.display();
}

void startLoRa() {
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  while (!LoRa.begin(BAND)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("LoRa Initialization OK!");
  display.setCursor(0, 10);
  display.clearDisplay();
  display.print("LoRa Initializing OK!");
  display.display();
  delay(10);
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  int i = 0 ;
  while (WiFi.status() != WL_CONNECTED  || i == 4 ) {
    delay(5000);
    Serial.print(".");
    i++;
  }
  Serial.println("WiFi connected");
}

void reconnectMQTT() {
  int j = 0;
  while (!client.connected() || j == 4) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32_Client", access_token, NULL)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
    j++;
  }
}

void sendToThingsBoard() {
  if (!client.connected()) {
    reconnectMQTT();
  }

  char payload[256];
  int currentRSSI =  LoRa.packetRssi();
  // Calculate and send water percentage
  int Tank_Level_Percentage = map(Tank_Level_cms, 200, 20, 0, 100);
  int lora_rssi = map(currentRSSI, -100, 0, 0, 100);
  snprintf(payload, sizeof(payload), "{\"Tank_Level_Percentage\":%d, \"Pump_Status\":%s, \"Tank_Level_cms\":%d, \"LoRa Strength\":%d}", 
           Tank_Level_Percentage, digitalRead(pump) ? "true" : "false", Tank_Level_cms,lora_rssi);
  client.publish("v1/devices/me/telemetry", payload);
}

void setup() {
  Serial.begin(9600);
  startOLED();
  startLoRa();
  pinMode(pump, OUTPUT);

  connectWiFi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  client.loop();

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String LoRaMessage = "";
    while (LoRa.available()) {
      LoRaMessage += (char)LoRa.read();
    }

    Serial.print("Received packet: ");
    Serial.println(LoRaMessage);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("Received packet:");
    display.setCursor(0, 10);
    display.print(LoRaMessage);
    display.display();

    String message = LoRaMessage;
    int distanceStartIndex = message.indexOf("Distance:") + 10;
    int distanceEndIndex = message.indexOf(" cm", distanceStartIndex);

    if (distanceStartIndex > 0 && distanceEndIndex > distanceStartIndex) {
      String y = message.substring(distanceStartIndex, distanceEndIndex);
      Tank_Level_cms = y.toInt();

      Serial.print("Tank_Level_cms= ");
      Serial.print(Tank_Level_cms);
      Serial.println(" cm");

      if (Tank_Level_cms >= 100 && Tank_Level_cms <= 200 && flag == 1) { 
        digitalWrite(pump, HIGH);
        Serial.println("PUMP ON");
        flag = 0;

      } else if (Tank_Level_cms <= 30 && Tank_Level_cms >= 0 && flag == 0) { 
        digitalWrite(pump, LOW);
        Serial.println("PUMP OFF");
        flag = 1;
      }
      else if (Tank_Level_cms >= 30 && Tank_Level_cms <= 99 && flag == 0) { 
        digitalWrite(pump, LOW);
        Serial.println("PUMP OFF");
        flag = 1;
      }else
      {
        digitalWrite(pump, LOW);
        Serial.println("PUMP OFF");
        flag = 1;
      }
    }

    sendToThingsBoard();
  }
}
