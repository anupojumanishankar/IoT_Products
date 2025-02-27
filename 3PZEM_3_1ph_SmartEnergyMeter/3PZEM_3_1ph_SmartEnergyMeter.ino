//final code without error - Thingsbord & Thingworx 3 Ph - Mani Shankar july 12 2024 
 
 
#include <PZEM004Tv30.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
 
WiFiClient wifiClient;
PubSubClient client(wifiClient);
 
// Use Serial2 for PZEM modules
HardwareSerial pzemSerial(2); // Serial2
#define PZEM_RX_PIN 18 // Example RX pin for Serial2
#define PZEM_TX_PIN 5 // Example TX pin for Serial2
#define NUM_PZEMS 3
 
// LED indicator
#define red 15
#define green 2
#define blue 4
 
PZEM004Tv30 pzems[NUM_PZEMS];
 
const char* mqttServer = "demo.thingsboard.io";
const int mqttPort = 1883;
const char* mqttUsername = "HBJ6Xd5Tw368hRkCKrVb"; // Replace with your ThingsBoard access token
const char* host = "thingworx.scetngp.com";
const int httpsPort = 80;
 
const char Thing[] = "DCS_3PH_ENERGYMETER";
const char Property1[] = "current_1";
const char Property2[] = "current_2";
const char Property3[] = "current_3";
const char Property4[] = "pf";
const char Property5[] = "power";
const char Property6[] = "voltage";
const char Property7[] = "Energy";
 
float currents[NUM_PZEMS] = {0};
 
// const int WIFI_RESET_PIN = 4;  // GPIO pin for resetting WiFi credentials
const int ENERGY_RESET_PIN = 2;  // GPIO pin for resetting PZEM energy readings
const int BUTTON_PIN = 23; // Button pin for resetting WiFi credentials using WiFiManager
 
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 15 * 60 * 1000; // 15 minutes in milliseconds
const float minVoltageChange = 50.0;
const float minPFChange = 0.1;
const float minCurrentChange = 0.75;
 
float prevAvgVoltage = 0.0;
float prevAvgPF = 0.0;
float prevCurrents[NUM_PZEMS] = {0};
 
// Initialize WiFiManager
WiFiManager wm;
 
void setRGBColor(bool redState, bool greenState, bool blueState) {
    digitalWrite(red, redState ? HIGH : LOW);
    digitalWrite(green, greenState ? HIGH : LOW);
    digitalWrite(blue, blueState ? HIGH : LOW);
}
 
void setup() {
    Serial.begin(115200);
 
    // pinMode(WIFI_RESET_PIN, INPUT_PULLUP);
    pinMode(ENERGY_RESET_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Set button pin as input with internal pull-up resistor
 
    pinMode(red, OUTPUT);
    pinMode(green, OUTPUT);
    pinMode(blue, OUTPUT);
 
    WiFi.mode(WIFI_STA);
 
    setRGBColor(true, false, false);
    bool res = wm.autoConnect("Dysmech_3 Phase Energy Meter");
    if (!res) {
        Serial.println("Failed to connect");
    } else {
        Serial.print("Connected to: ");
        Serial.println(WiFi.SSID());
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
 
        setRGBColor(false, false, false);
        delay(500);
        setRGBColor(false, true, false);
    }
 
    // MQTT client setup
    client.setServer(mqttServer, mqttPort);
    // Set callback function for receiving MQTT messages, if needed
    // client.setCallback(callback);
 
    // Initialize PZEM004Tv30 objects using Serial2
    pzemSerial.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
    for (int i = 0; i < NUM_PZEMS; i++) {
        pzems[i] = PZEM004Tv30(pzemSerial, PZEM_RX_PIN, PZEM_TX_PIN, 0x01 + i);
    }
}
 
void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
 
    // if (digitalRead(WIFI_RESET_PIN) == LOW || digitalRead(BUTTON_PIN) == LOW)
    if (digitalRead(BUTTON_PIN) == LOW) {
        // Reset WiFi credentials and restart WiFiManager portal
        Serial.println("Button pressed, resetting WiFi credentials...");
        wm.resetSettings();
        ESP.restart();
    }
 
    if (digitalRead(ENERGY_RESET_PIN) == LOW) {
        // Reset energy readings of all PZEM modules
        for (int i = 0; i < NUM_PZEMS; i++) {
            pzems[i].resetEnergy();
        }
    }
 
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdateTime >= updateInterval) {
        lastUpdateTime = currentMillis;
 
        float totalVoltage = 0.0;
        float totalPower = 0.0;
        float totalEnergy = 0.0;
        float totalPF = 0.0;
 
        for (int i = 0; i < NUM_PZEMS; i++) {
            float voltage = pzems[i].voltage();
            float current = pzems[i].current();
            float power = pzems[i].power();
            float energy = pzems[i].energy();
            float pf = pzems[i].pf();
 
            if (!isnan(voltage) && !isnan(current) && !isnan(power) && !isnan(energy) && !isnan(pf)) {
                totalVoltage += voltage;
                totalPower += power;
                totalEnergy += energy;
                totalPF += pf;
                currents[i] = current;
            }
        }
 
        float avgVoltage = totalVoltage / NUM_PZEMS;
        float avgPower = totalPower / NUM_PZEMS;
        float avgPF = totalPF / NUM_PZEMS;
 
        // Publish data to ThingsBoard via MQTT
        publishData("v1/devices/me/telemetry", "Average_Voltage", avgVoltage);
        publishData("v1/devices/me/telemetry", "Average_Power", avgPower);
        publishData("v1/devices/me/telemetry", "Total_Energy", totalEnergy);
        publishData("v1/devices/me/telemetry", "Average_Power_Factor", avgPF);
        Put(Thing, Property7, totalEnergy);
        Put(Thing, Property6, avgVoltage);
        Put(Thing, Property5, avgPower);
        Put(Thing, Property4, avgPF);
 
        for (int i = 0; i < NUM_PZEMS; i++) {
            String propertyName = "Current_" + String(i + 1);
            publishData("v1/devices/me/telemetry", propertyName.c_str(), currents[i]);
            if (i == 0) {
                Put(Thing, Property1, currents[i]);
            } else if (i == 1) {
                Put(Thing, Property2, currents[i]);
            } else {
                Put(Thing, Property3, currents[i]);
            }
        }
    }
    delay(1000); // Adjust delay as needed
}
 
void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP32Client", mqttUsername, "")) { // Correct call to connect
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" : retrying in 5 seconds");
            delay(5000);
        }
    }
}
 
void publishData(const char* topic, const char* property, float value) {
    StaticJsonDocument<200> doc;
    doc[property] = value;
    char buffer[256];
    size_t n = serializeJson(doc, buffer);
 
    if (client.publish(topic, buffer, n)) {
        Serial.print("Published to ");
        Serial.print(topic);
        Serial.print(", Property: ");
        Serial.print(property);
        Serial.print(", Value: ");
        Serial.println(value);
    } else {
        Serial.println("Failed to publish to ThingsBoard MQTT broker");
    }
}
 
void Put(String ThingName, String ThingProperty, float Value) {
    WiFiClient thingworxClient;
    if (!thingworxClient.connect(host, httpsPort)) {
        Serial.println("connection failed");
        return;
    } else {
        Serial.println("Connected to ThingWorx.");
    }
    String url = "/Thingworx/Things/" + ThingName + "/Properties/" + ThingProperty;
    Serial.print("requesting URL: ");
    Serial.println(url);
 
    String strPUTReqVal = "{\"" + ThingProperty + "\":\"" + Value + "\"}";
    Serial.print("PUT Value: ");
    Serial.println(strPUTReqVal);
    thingworxClient.print(String("PUT ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "appKey: 4ca2847b-70b2-4bbe-b001-bc1eff6695d9" + "\r\n" +
                 "x-thingworx-session: false" + "\r\n" +
                 "Accept: application/json" + "\r\n" +
                 "Connection: close" + "\r\n" +
                 "Content-Type: application/json" + "\r\n" +
                 "Content-Length: " + String(strPUTReqVal.length()) + "\r\n\r\n" +
                 strPUTReqVal + "\r\n\r\n");
 
    while (thingworxClient.connected()) {
        String line = thingworxClient.readStringUntil('\r');
        Serial.print(line);
    }
    thingworxClient.stop();
}
 