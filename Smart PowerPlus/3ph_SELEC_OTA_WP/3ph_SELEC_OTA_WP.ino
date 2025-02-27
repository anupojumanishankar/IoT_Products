 #define up_ver "1.0"
#include <WiFiManager.h>
#include <HTTPUpdate.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ModbusMaster.h>
#include <WiFi.h>
#include <HTTPClient.h>

const int PushButton = 21;

Preferences preferences;

// Define Firmware Update URL
#define URL_fw_Bin "https://raw.githubusercontent.com/dcsplm/SELEC3PH/main/fww.bin"

// WiFi credentials
#define WIFI_SSID "DCS_SMART_POWERPLUS_V1"
#define WIFI_PASSWORD "dcs@12345"

const char* mqtt_server = "thingsboard.cloud";
const int mqttPort = 1883;
const char* access_token = "singandsonsdcsdcsptr";  // Replace with your ThingsBoard device access token

// RGB LED pins
#define RED_PIN 15
#define GREEN_PIN 2
#define BLUE_PIN 4

WiFiClient espClient;
PubSubClient client(espClient);
float dataSet[15];
char outData[300];

// Registers for Voltage, Current, Power Factor, etc.
int Reg[] = {0x0009, 0x000B, 0x000D,   // Voltage V12, V23, V31
             0x0011, 0x0013, 0x0015,   // Current I1, I2, I3
             0x002F, 0x0031, 0x0033,   // Power Factor PF1, PF2, PF3
             0x0035,                   // Avg PF
             0x0037,                   // Frequency
             0x0039,                   // Total kWH
             0x0029,                   // TOTAL KW
             0x003B                    // TOTAL KVAh
};

#define MAX485_DE      5
#define MAX485_RE_NEG  5
#define SLAVE_ID 1
#define REG_COUNT 14  // Updated to include all registers

ModbusMaster node;

String wifiSSID = "";
String wifiPassword = "";

void setupWiFi() {
    WiFiManager wifiManager;
    if (!wifiManager.autoConnect(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("Failed to connect to WiFi.");
        ESP.restart();
    }
    wifiSSID= WiFi.SSID();
    wifiPassword= WiFi.psk();
    preferences.putString("wifiSSID", wifiSSID);
    preferences.putString("wifiPassword", wifiPassword);
    Serial.println("WIFI credentials saved");
    wifiSSID = preferences.getString("wifiSSID", "");
    wifiPassword = preferences.getString("wifiPassword", "");
    Serial.println(wifiSSID);
    Serial.println(wifiPassword);
    Serial.println("WiFi connected.");
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP32Client", access_token, NULL)) {
            Serial.println("Connected to MQTT server.");
            // Subscribe to RPC request topic
            client.subscribe("v1/devices/me/rpc/request/+",1);
            Serial.println("Subscribed to RPC topic.");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" - Trying again in 5 seconds...");
            delay(5000);
            if(WiFi.status() != WL_CONNECTED){
            setRGB(0, 0, 255);
            Serial.println("Attempting to reconnect to Wi-Fi...");
            //WiFi.disconnect(false); // Disconnect from current network without erasing credentials
            delay(1000); // Allow some time to process disconnect
            wifiSSID = preferences.getString("wifiSSID", "");
            wifiPassword = preferences.getString("wifiPassword", "");
            WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str()); // Attempt to reconnect using saved credentials
            if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nReconnected to Wi-Fi successfully.");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            int Push_button_state = digitalRead(PushButton);

            if ( Push_button_state == HIGH ){
              WiFi.disconnect(true); // Disconnects and erases saved credentials
              delay(3000); // Allow some time to disconnect
        // Restart the ESP32
              preferences.clear();
              Serial.println("resetting wifi");
              WiFiManager wifiManager;
              wifiManager.resetSettings();
              delay(1000);
              ESP.restart(); 
            }
            }
             else {
              Serial.println("\nFailed to reconnect to Wi-Fi.");
              setRGB(0, 0, 0);
              delay(100);
              setRGB(0, 0, 255);
            }

            

    if ( digitalRead(PushButton) == HIGH ){
 
      WiFi.disconnect(true); // Disconnects and erases saved credentials
      delay(3000); // Allow some time to disconnect
      // Restart the ESP32
      Serial.println("resetting wifi");
      WiFiManager wifiManager;
      wifiManager.resetSettings();
      delay(1000);
      ESP.restart(); 
    }
  }

  delay(100); // Small delay for loop execution
            }
        }
    
}

void preTransmission() {
    digitalWrite(MAX485_RE_NEG, 1);
    digitalWrite(MAX485_DE, 1);
}

void postTransmission() {
    digitalWrite(MAX485_RE_NEG, 0);
    digitalWrite(MAX485_DE, 0);
}

void setRGB(int red, int green, int blue) {
    analogWrite(RED_PIN, red);
    analogWrite(GREEN_PIN, green);
    analogWrite(BLUE_PIN, blue);
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    pinMode(MAX485_RE_NEG, OUTPUT);
    pinMode(MAX485_DE, OUTPUT);
    digitalWrite(MAX485_RE_NEG, 0);
    digitalWrite(MAX485_DE, 0);
    node.begin(SLAVE_ID, Serial2);
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
    pinMode(PushButton, INPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    setRGB(255, 255, 255);  // blue 
    preferences.begin("wifi-creds", false);
    wifiSSID = preferences.getString("wifiSSID", "");
    wifiPassword = preferences.getString("wifiPassword", "");
    Serial.println(wifiSSID);
    Serial.println(wifiPassword);
    if (wifiSSID.isEmpty() || wifiPassword.isEmpty()) {
        Serial.println("No saved Wi-Fi credentials found. Starting WP ...");   
        setupWiFi();
        preferences.begin("ota", false);
    }
    else{
      int count = 0; // Initialize attempt counter
      const int maxAttempts = 20; // Maximum number of connection attempts

      // Call WiFi.begin() once before the loop
      WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
      Serial.println("Attempting to connect to WiFi...");
      while(WiFi.status() != WL_CONNECTED){
      delay(1000);
      setRGB(0, 0, 255);
      delay(1000);
      setRGB(0, 255, 0);
      int count = 0 ;
       count++;
    if (count >= maxAttempts) {
        Serial.println("\nFailed to connect to WiFi after maximum attempts. Restarting...");
        ESP.restart(); // Restart the device as a last resort
    }
      int Push_button_state = digitalRead(PushButton);
      if ( Push_button_state == HIGH ){
        WiFi.disconnect(true); // Disconnects and erases saved credentials
        delay(3000); // Allow some time to disconnect
        // Restart the ESP32
        preferences.clear();
        Serial.println("resetting wifi");
        WiFiManager wifiManager;
        wifiManager.resetSettings();
        delay(1000);
        ESP.restart(); 
      }
      }
    }
    if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    setRGB(255, 0, 0); // Indicate WiFi connected (green)
}
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    setRGB(0, 0, 0);  // blue 
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    delay(2000);
    Read_Reg_SELEC();
    delay(2000);
    client.loop();
    delay(2000);
    int Push_button_state = digitalRead(PushButton);

    if ( Push_button_state == HIGH ){
        WiFi.disconnect(true); // Disconnects and erases saved credentials
        delay(3000); // Allow some time to disconnect
        // Restart the ESP32
        preferences.clear();
        Serial.println("resetting wifi");
        WiFiManager wifiManager;
        wifiManager.resetSettings();
        delay(1000);
        ESP.restart(); 
    }
}

float InttoFloat(uint16_t Data0, uint16_t Data1) {
    float x;
    unsigned long *p = (unsigned long*)&x;
    *p = (unsigned long)Data0 << 16 | Data1;
    return x;
}

void Read_Reg_SELEC(void) {
    uint8_t result;
    uint16_t data[50];
    Serial.println("Reading from Meter...");
    for (int i = 0; i < REG_COUNT; i++) {
        result = node.readInputRegisters(Reg[i], 2);
        if (result == node.ku8MBSuccess) {
            for (int k = 0; k < 2; k++) {
                data[k] = node.getResponseBuffer(k);
            }
            dataSet[i] = InttoFloat(data[1], data[0]);
        }
        delay(10);
    }
    setRGB(0, 255, 0);  // blue 
    delay(100);
    setRGB(0, 0, 0);  // blue 
    snprintf(outData, 300,
             "V12: %0.2f, V23: %0.2f, V31: %0.2f, "
             "I1: %0.2f, I2: %0.2f, I3: %0.2f, "
             "PF1: %0.2f, PF2: %0.2f, PF3: %0.2f, Avg PF: %0.2f, "
             "Frequency: %0.2f, Total_kWh: %0.2f, TOTAL_KW: %0.2f, TOTAL_KVAh: %0.2f",
             dataSet[0], dataSet[1], dataSet[2],
             dataSet[3], dataSet[4], dataSet[5],
             dataSet[6], dataSet[7], dataSet[8],
             dataSet[9],
             dataSet[10],
             dataSet[11],
             dataSet[12],
             dataSet[13]);
    Serial.println(outData);

    String payload = String("{") +
                     "\"V12\":" + String(dataSet[0]) + "," +
                     "\"V23\":" + String(dataSet[1]) + "," +
                     "\"V31\":" + String(dataSet[2]) + "," +
                     "\"I1\":" + String(dataSet[3]) + "," +
                     "\"I2\":" + String(dataSet[4]) + "," +
                     "\"I3\":" + String(dataSet[5]) + "," +
                     "\"PF1\":" + String(dataSet[6]) + "," +
                     "\"PF2\":" + String(dataSet[7]) + "," +
                     "\"PF3\":" + String(dataSet[8]) + "," +
                     "\"Avg_PF\":" + String(dataSet[9]) + "," +
                     "\"Frequency\":" + String(dataSet[10]) + "," +
                     "\"Total_kWh\":" + String(dataSet[11]) + "," +
                     "\"TOTAL_KW\":" + String(dataSet[12]) + "," +
                     "\"VER\":" + up_ver + "," +
                     "\"TOTAL_KVAh\":" + String(dataSet[13]) +
                     "}";
    if (client.publish("v1/devices/me/telemetry", payload.c_str())) {
        //setRGB(0, 0, 255);  // Red 
        setRGB(255, 0, 0);  // Green on success
    } else {
        //setRGB(0, 255, 0);  // blue 
        setRGB(0, 0, 255);  // red on fail
    }
}

void firmwareUpdate() {
    setRGB(0, 255, 255);  //red blue
    publishData("v1/devices/me/telemetry", "OTA_Ver", up_ver);
    publishData("v1/devices/me/telemetry", "OTA_Status", "Starting firmware update...");
    WiFiClientSecure client;
    client.setInsecure();
    t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            publishData("v1/devices/me/telemetry", "OTA_Status", "HTTP_UPDATE_FAILED");
            break;
        case HTTP_UPDATE_NO_UPDATES:
            publishData("v1/devices/me/telemetry", "OTA_Status", "HTTP_UPDATE_NO_UPDATES");
            break;
        case HTTP_UPDATE_OK:
            publishData("v1/devices/me/telemetry", "OTA_Status", "HTTP_UPDATE_OK");
            break;
    }
}
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived on topic: ");
    Serial.println(topic);
    
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Message payload: ");
    Serial.println(message);

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Serial.print("JSON parsing error: ");
        Serial.println(error.c_str());
        return;
    }

    // Check if the topic starts with the correct RPC topic path
    if (String(topic).startsWith("v1/devices/me/rpc/request/")) {
        Serial.println("Valid RPC call received.");
        const char* method = doc["method"];
        Serial.print("Method: ");
        Serial.println(method);

        if (String(method) == "updateFirmware") {
            publishData("v1/devices/me/telemetry", "OTA_Ver", up_ver);
            publishData("v1/devices/me/telemetry", "OTA_Status", "Starting firmware update...");
            firmwareUpdate();
        } else {
            Serial.println("Unknown RPC method.");
        }
    } else {
        Serial.println("Message received on an unexpected topic.");
    }
}

void publishData(const char* topic, const String& property, const String& value) {
    StaticJsonDocument<200> doc;
    doc[property] = value;
    char buffer[256];
    size_t n = serializeJson(doc, buffer);
    if (client.publish(topic, buffer, n)) {
        Serial.println("Data published successfully.");
    } else {
        Serial.println("Failed to publish data.");
    }
}
