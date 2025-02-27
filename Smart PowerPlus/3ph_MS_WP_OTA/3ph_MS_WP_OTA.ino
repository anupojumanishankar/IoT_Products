//MULTISPAN-OTA-WIFI PROVISINING-RGB - 22/01/2025
#define up_ver "2.0"
#include <WiFiManager.h>
#include <HTTPUpdate.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ModbusMaster.h>
#include <WiFi.h>
#include <HTTPClient.h>

Preferences preferences;

const int PushButton = 21;

// RGB LED pins
#define RED_PIN 15
#define GREEN_PIN 2
#define BLUE_PIN 4

WiFiClient wifiClient;
PubSubClient client(wifiClient);

float dataSet[19]; // Adjusted for registers
char outData[300]; // Correct buffer size for snprintf
 
// Registers for Voltage, Current, Power Factor, Avg PF, Frequency, Total Energy
int Reg[] = {
  0x000C, 0x0010, 0x0014, // Voltage V12, V23, V31
  0x001C, 0x0020, 0x0024, // Voltage V12, V23, V31
  0x002C, 0x0030, 0x0034, // I1, I2, I3
  0x003C, 0x003E, 0x0040, // Power Factor PF1, PF2, PF3
  0x0044,                 // Avg PF
  0x0046,                 // Frequency
  0x0000, 0x0004,         // Total Energy (KWH, KVAH)
  0x0054,                  // Total KW
  0x0064                   //Total kVA
};
 
#define MAX485_DE 5
#define MAX485_RE_NEG 5
#define SLAVE_ID 1
#define REG_COUNT 19 // Adjusted register count
 
ModbusMaster node;

const char* mqttServer = "thingsboard.cloud";
const int mqttPort = 1883;
const char* mqttUsername = "multispan3phdcsdcsptr"; // ThingsBoard access token

// Define Firmware Update URL
#define URL_fw_Bin "https://raw.githubusercontent.com/dcsplm/MS3PH/main/fww.bin"

// WiFi credentials
#define WIFI_SSID "DCS_SMART_POWERPLUS_V1"
#define WIFI_PASSWORD "dcs@12345"

String wifiSSID = "";
String wifiPassword = "";


void preTransmission() {
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}
 
void postTransmission() {
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}
 
float InttoFloat(uint16_t Data0, uint16_t Data1) {
  float x;
  unsigned long *p = (unsigned long*)&x;
  *p = ((unsigned long)Data0 << 16) | Data1;
  return x;
}
 
void Read_Reg_SELEC() {
  uint8_t result;
  uint16_t data[2];
      //setRGB(0, 0, 255);  // blue 
    //setRGB(0, 255, 0);  // red 
    setRGB(255, 0, 0);  // green   
  Serial.println("Reading Modbus registers...");
  for (int i = 0; i < REG_COUNT; i++) {
    result = node.readInputRegisters(Reg[i], 2);
    if (result == node.ku8MBSuccess) {
      data[0] = node.getResponseBuffer(0);
      data[1] = node.getResponseBuffer(1);
      dataSet[i] = InttoFloat(data[1], data[0]);
    } else {
      Serial.print("Error reading register ");
      Serial.println(Reg[i], HEX);
      dataSet[i] = 0.0; // Default value on error
    }
    delay(10);
  }
 
  snprintf(outData, sizeof(outData),
           "VN1: %0.2f, VN2: %0.2f, VN3: %0.2f, "
           "V12: %0.2f, V23: %0.2f, V31: %0.2f, "
           "I1: %0.2f, I2: %0.2f, I3: %0.2f, "
           "PF1: %0.2f, PF2: %0.2f, PF3: %0.2f, "
           "Avg PF: %0.2f, Frequency: %0.2f, "
           "Total Energy (KWH): %0.2f, Total Energy (KVAH): %0.2f, Total KW: %0.2f , Total KVA: %0.2f",
           dataSet[0], dataSet[1], dataSet[2], // Voltages
           dataSet[3], dataSet[4], dataSet[5], // VOL
           dataSet[6], dataSet[7], dataSet[8], // curren
           dataSet[9], // pf
           dataSet[10], // pf
           dataSet[11], //pf
           dataSet[12], //avgpf
           dataSet[13], //freq
           dataSet[14], //kwh
           dataSet[15], //kvah
           dataSet[16], // KW
           dataSet[17]); // Total KVA
  Serial.println(outData);
   
  String payload = String("{") +
                   "\"V12\":" + String(dataSet[3]) + "," +
                   "\"V23\":" + String(dataSet[4]) + "," +
                   "\"V31\":" + String(dataSet[5]) + "," +
                   "\"I1\":" + String(dataSet[6]) + "," +
                   "\"I2\":" + String(dataSet[7]) + "," +
                   "\"I3\":" + String(dataSet[8]) + "," +
                   "\"PF1\":" + String(dataSet[9]) + "," +
                   "\"PF2\":" + String(dataSet[10]) + "," +
                   "\"PF3\":" + String(dataSet[11]) + "," +
                   "\"Avg_PF\":" + String(dataSet[12]) + "," +
                   "\"Frequency\":" + String(dataSet[13]) + "," +
                   "\"Total_kWh\":" + String(dataSet[14]) + "," +
                   "\"TOTAL_KVAh\":" + String(dataSet[15]) + "," +
                   "\"Total_KVA\":" + String(dataSet[17]) + "," +
                   "\"VER\":" + up_ver + "," +
                   "\"TOTAL_KW\":" + String(dataSet[16]) +
                   "}";
 
  if (client.publish("v1/devices/me/telemetry", payload.c_str())) {
    Serial.println("Data sent to ThingsBoard successfully.");
    setRGB(0, 0, 255);  // blue on success
  } else {
    Serial.println("Failed to send data to ThingsBoard.");
    setRGB(0, 255, 0);  // red on fail
 
  }
}

void publishData(const char* topic, const String& property, const String& value) {
    StaticJsonDocument<200> doc;
    doc[property] = value;
    char buffer[256];
    size_t n = serializeJson(doc, buffer);
    if (!client.publish(topic, buffer, n)) {
        Serial.println("Failed to publish data to ThingsBoard.");
    } else {
        Serial.println("Data published successfully to ThingsBoard.");
    }
}

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
    Serial.println("WiFi connected.");
    setRGB(255, 0, 0);  // GREEN 
    
}

void firmwareUpdate() {
    setRGB(0, 255, 255);  // blue red
    publishData("v1/devices/me/telemetry", "OTA_Ver", up_ver);
    publishData("v1/devices/me/telemetry", "OTA_Status", "Starting firmware update...");
    delay(100);
    WiFiClientSecure client;
    client.setInsecure(); // Use this if no root CA is available
    delay(100);
    Serial.println("Starting firmware update...");
    
    t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

    switch (ret) {
        case HTTP_UPDATE_FAILED: {
            char errorMessage[256];
            snprintf(errorMessage, sizeof(errorMessage), "HTTP_UPDATE_FAILED Error (%d): %s", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            publishData("v1/devices/me/telemetry", "OTA_Status", errorMessage);
            delay(1000);
            Serial.println(errorMessage);
            break;
        }
        case HTTP_UPDATE_NO_UPDATES:
            publishData("v1/devices/me/telemetry", "OTA_Status", "HTTP_UPDATE_NO_UPDATES");
            delay(1000);
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;
        case HTTP_UPDATE_OK:
            publishData("v1/devices/me/telemetry", "OTA_Status", "HTTP_UPDATE_OK");
            delay(1000);
            Serial.println("HTTP_UPDATE_OK");
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



void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP32Client", mqttUsername, NULL)) {
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
                //setRGB(0, 0, 255);  // blue 
                setRGB(0, 255, 0);  // red 
                //setRGB(255, 0, 0);  // green   
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
              setRGB(0, 255, 0);// blue
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

void setRGB(int red, int green, int blue) {
    analogWrite(RED_PIN, red);
    analogWrite(GREEN_PIN, green);
    analogWrite(BLUE_PIN, blue);
}

void setup() {
    Serial.begin(115200);
    pinMode(PushButton, INPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    //setRGB(0, 0, 255);  // blue 
    setRGB(0, 255, 0);  // red 
    //setRGB(255, 0, 0);  // green   
    //setRGB(255, 255, 255);  // all 

    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    pinMode(MAX485_RE_NEG, OUTPUT);
    pinMode(MAX485_DE, OUTPUT);
    digitalWrite(MAX485_RE_NEG, 0);
    digitalWrite(MAX485_DE, 0);
 
    node.begin(SLAVE_ID, Serial2);
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
    preferences.begin("ota", false);
    preferences.begin("wifi-creds", false);
    wifiSSID = preferences.getString("wifiSSID", "");
    wifiPassword = preferences.getString("wifiPassword", "");
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
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);

    Serial.println("Setup complete.");
    setRGB(0, 0, 0);  // blue 
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    delay(2000);
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
