updated - final SCET GENSET CODE 
 
#include <WiFi.h>
#include <PubSubClient.h>
#include <PZEM004Tv30.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
 
// WiFi and MQTT Credentials
#define WIFI_SSID "NETGEARE51"
#define WIFI_PASSWORD "youngbird986"
#define THINGSBOARD_SERVER "thingsboard.cloud"
#define THINGSBOARD_TOKEN "TUJKKtrqTxAEB8m2jOIs"
 
// MQTT & WiFi Client Setup
WiFiClient espClient;
PubSubClient client(espClient);
 
// Sensor Pins
#define PROXIMITY_SENSOR_PIN 5
#define MQ7_PIN 33
#define ONE_WIRE_BUS 2
 
// Initialize Sensors
PZEM004Tv30 pzem(Serial2, 17, 16);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);
Adafruit_MPU6050 mpu;
 
// RPM Variables
volatile int pulseCount = 0;
float rpm = 0;
unsigned long lastMillis = 0;
 
// Debounce Variables for Noise Filtering
volatile unsigned long lastInterruptTime = 0;
 
// Interrupt Service Routine (ISR) for RPM Calculation
void IRAM_ATTR countPulse() {
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > 20) { // Ignore pulses within 20ms
        pulseCount++;
    }
    lastInterruptTime = interruptTime;
}
 
// WiFi Connection
void connectWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi Connected!");
}
 
// MQTT Connection
void connectMQTT() {
    client.setServer(THINGSBOARD_SERVER, 1883);
    while (!client.connected()) {
        Serial.print("Connecting to ThingsBoard MQTT...");
        if (client.connect("ESP32_Client", THINGSBOARD_TOKEN, NULL)) {
            Serial.println("Connected!");
        } else {
            Serial.print("Failed, rc=");
            Serial.println(client.state());
            delay(5000);
        }
    }
}
 
void setup() {
    Serial.begin(115200);
   
    // Connect WiFi & MQTT
    connectWiFi();
    connectMQTT();
 
    // Configure Proximity Sensor Pin with Internal Pull-down
    pinMode(PROXIMITY_SENSOR_PIN, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(PROXIMITY_SENSOR_PIN), countPulse, RISING);
   
    // Initialize Sensors
    tempSensor.begin();
    mpu.begin();
}
 
void loop() {
    // Reconnect MQTT if disconnected
    if (!client.connected()) connectMQTT();
    client.loop();
   
    // Calculate RPM Every 1 Second
    unsigned long currentMillis = millis();
    if (currentMillis - lastMillis >= 1000) {
        rpm = (pulseCount * 60); // Convert pulses to RPM
        pulseCount = 0;
        lastMillis = currentMillis;
    }
 
    // Read MQ-7 Sensor
    int mq7Value = analogRead(MQ7_PIN);
    float mq7Voltage = mq7Value * (3.3 / 4095.0);
   
    // Read Temperature Sensor
    tempSensor.requestTemperatures();
    float temperature = tempSensor.getTempCByIndex(0);
   
    // Read MPU6050 Data
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
   
    // Read PZEM Data
    float voltage = pzem.voltage();
    float current = pzem.current();
    float power = pzem.power();
    float energy = pzem.energy();
    float frequency = pzem.frequency();
    float powerFactor = pzem.pf();
   
    // Send Data to ThingsBoard
    String payload = "{";
    payload += "\"rpm\":" + String(rpm) + ",";
    payload += "\"mq7_voltage\":" + String(mq7Voltage) + ",";
    payload += "\"temperature\":" + String(temperature) + ",";
    payload += "\"accel_x\":" + String(a.acceleration.x) + ",";
    payload += "\"accel_y\":" + String(a.acceleration.y) + ",";
    payload += "\"accel_z\":" + String(a.acceleration.z) + ",";
    payload += "\"voltage\":" + String(voltage) + ",";
    payload += "\"current\":" + String(current) + ",";
    payload += "\"power\":" + String(power) + ",";
    payload += "\"energy\":" + String(energy) + ",";
    payload += "\"frequency\":" + String(frequency) + ",";
    payload += "\"power_factor\":" + String(powerFactor);
    payload += "}";
   
    client.publish("v1/devices/me/telemetry", payload.c_str());
    Serial.println(payload);
   
    delay(2000);
}
 