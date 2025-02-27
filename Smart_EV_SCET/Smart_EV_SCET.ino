#include <WiFi.h>
#include <PubSubClient.h>
#include <PZEM004Tv30.h>
#include <TinyGPS++.h>
 
// Wi-Fi Credentials
const char* ssid = "";
const char* password = "";
 
// ThingsBoard MQTT Settings
const char* mqtt_server = "thingsboard.cloud"; // Change if using local server
const int mqtt_port = 1883;
const char* access_token = ""; // Replace with ThingsBoard token
 
WiFiClient espClient;
PubSubClient client(espClient);
 
// Define serial ports for different modules
HardwareSerial neoSerial(1); // GPS Module (UART1)
HardwareSerial vajraveghaSerial(2); // Vajravegha Sensor (UART2)
 
// PZEM (UART2 - GPIO 26, 27)
PZEM004Tv30 pzem(Serial2, 26, 27);
 
TinyGPSPlus gps; // GPS Parser
 
void setup() {
    Serial.begin(115200); // Monitor
   
    // Wi-Fi Setup
    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi Connected!");
 
    // MQTT Setup
    client.setServer(mqtt_server, mqtt_port);
    reconnect();
 
    // Initialize Serial Communication for Sensors
    neoSerial.begin(9600, SERIAL_8N1, 16, 17); // GPS on UART1
    vajraveghaSerial.begin(9600, SERIAL_8N1, 4, 2); // Vajravegha Sensor on UART2
}
 
void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
 
    // Read PZEM-004T Data
    float voltage = pzem.voltage();
    float current = pzem.current();
    float power = pzem.power();
    float energy = pzem.energy();
 
    // Read NEO-6M GPS Data
    while (neoSerial.available() > 0) {
        gps.encode(neoSerial.read());
    }
 
    float latitude = 0.0, longitude = 0.0;
    if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
    }
 
    // Read Vajravegha Sensor Data
    float vajru_voltage = 0.0, vajru_current = 0.0, vajru_wattHour = 0.0, vajru_ampereHour = 0.0;
    if (vajraveghaSerial.available()) {
        String sensorData = vajraveghaSerial.readStringUntil('D'); // Read until 'D'
        vajru_current = sensorData.substring(1).toFloat();
        vajraveghaSerial.readStringUntil('D'); // Skip over-current flag
        sensorData = vajraveghaSerial.readStringUntil('D');
        vajru_voltage = sensorData.substring(1).toFloat();
        sensorData = vajraveghaSerial.readStringUntil('D');
        vajru_wattHour = sensorData.substring(2).toFloat();
        sensorData = vajraveghaSerial.readStringUntil('D');
        vajru_ampereHour = sensorData.substring(2).toFloat();
    }
 
    // Create JSON Payload
    String payload = "{";
    payload += "\"PZEM_Voltage\":" + String(voltage) + ",";
    payload += "\"PZEM_Current\":" + String(current) + ",";
    payload += "\"PZEM_Power\":" + String(power) + ",";
    payload += "\"PZEM_Energy\":" + String(energy) + ",";
    payload += "\"GPS_Latitude\":" + String(latitude, 6) + ",";
    payload += "\"GPS_Longitude\":" + String(longitude, 6) + ",";
    payload += "\"Vajravegha_Voltage\":" + String(vajru_voltage) + ",";
    payload += "\"Vajravegha_Current\":" + String(vajru_current) + ",";
    payload += "\"Vajravegha_Energy\":" + String(vajru_wattHour) + ",";
    payload += "\"Vajravegha_Charge\":" + String(vajru_ampereHour);
    payload += "}";
 
    Serial.println("Sending Data to ThingsBoard:");
    Serial.println(payload);
 
    // Publish Data to ThingsBoard
    client.publish("v1/devices/me/telemetry", payload.c_str());
 
    delay(5000); // Send data every 5 seconds
}
 
// Function to Reconnect to MQTT Server
void reconnect() {
    while (!client.connected()) {
        Serial.print("Connecting to ThingsBoard...");
        if (client.connect("ESP32Client", access_token, "")) {
            Serial.println("Connected!");
        } else {
            Serial.print("Failed, retrying in 5 seconds...");
            delay(5000);
        }
    }
}
 