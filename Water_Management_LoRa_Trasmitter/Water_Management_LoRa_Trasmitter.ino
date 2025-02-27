//Lora Transmitter Code
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>
#include <SoftwareSerial.h>
 
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
 
// Pin definition for TDS sensor
#define TdsSensorPin 36
 
// Pin definitions for Ultrasonic Sensor
#define pinRX 13
#define pinTX 12
 
// Array to store incoming serial data
unsigned char data_buffer[4] = {0};
 
// Integer to store distance
int distance = 0;
int x = 0;
 
// Variable to hold checksum
unsigned char CS;
 
// Object to represent software serial port
SoftwareSerial mySerial(pinRX, pinTX);
 
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
  display.print("LORA SENDER");
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
 
// Function to read TDS value from sensor
float readTDS() {
  int sensorValue = analogRead(TdsSensorPin);
 
  // Convert the sensor value to TDS value based on calibration
  // Replace these calculations with your actual calibration logic
  float voltage = sensorValue * (3.3 / 4095.0);
  float tdsValue = (133.42 * voltage * voltage * voltage - 255.86 * voltage * voltage + 857.39 * voltage) * 0.5;
 
  return tdsValue;
}
 
void sendReadings() {
  float tdsValue = readTDS();
 
  // Send LoRa message
  String LoRaMessage = "TDS: " + String(tdsValue) + " ppm, Distance: " + String(distance) + " cm";
  LoRa.beginPacket();
  LoRa.print(LoRaMessage);
  LoRa.endPacket();
 
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("LoRa packet sent!");
  display.setCursor(0, 20);
  display.print("TDS Value:");
  display.setCursor(72, 20);
  display.print(tdsValue);
  display.setCursor(0, 30);
  display.print("Distance:");
  display.setCursor(72, 30);
  display.print(distance);
  display.setCursor(0, 50);
  display.print("Reading ID:");
  display.setCursor(66, 50);
  display.print(millis() / 1000); // Use millis() as reading ID
  display.display(); // Update OLED display
 
  Serial.print("Sending packet: ");
  Serial.println(LoRaMessage);
}
 
void setup() {
  mySerial.begin(9600);
  Serial.begin(9600);
  startOLED();
  startLoRa();
}
 
void loop() {
  // Run if data available
  if (mySerial.available() > 0) {
    delay(4);
    // Check for packet header character 0xff
    if (mySerial.read() == 0xff) {
      // Insert header into array
      data_buffer[0] = 0xff;
      // Read remaining 3 characters of data and insert into array
      for (int i = 1; i < 4; i++) {
        data_buffer[i] = mySerial.read();
      }
 
      // Compute checksum
      CS = data_buffer[0] + data_buffer[1] + data_buffer[2];
      // If checksum is valid, compose distance from data
      if (data_buffer[3] == CS) {
        x = (data_buffer[1] << 8) + data_buffer[2];
 
       distance = x/10;
 
        // Print distance to OLED display
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Distance: ");
        display.print(distance);
        display.println(" cm");
        display.display();
       
        // Print to serial monitor
        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.println(" mm");
      }
    }
  }
 
  // Send TDS and Distance readings via LoRa
  sendReadings();
  delay(500); // Adjust delay as needed
}