#include <WiFi.h>
#include<Wire.h>
#define MQ_sensor 34  
#define wetfloor  35   
#define light_Red   15        // IN1
#define perfume   14          // IN2
#define fan       2          // IN3
#define light_Green   4      //IN4

const char simPIN[]   = "";
#define SMS_TARGET1 "+917020736605"
#define SMS_TARGET2 "+919989831337"
#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER   1024
#include <TinyGsmClient.h>
#define SerialMon Serial
#define SerialAT  Serial1

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

#define IP5306_ADDR          0x75
#define IP5306_REG_SYS_CTL0  0x00

// TTGO T-Call pins
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22
long t1,t2;


WiFiClient client;
long old_time1;
long old_time2;
const char* ssid = "Mani";
const char* password = "Mani6652";  

/*const char* ssid = "MANI DCS";
const char* password = "Mani1337"; */

const char* host = "thingworx.scetngp.com";
const int httpsPort = 80;

const char Thing[] = "MQ_135";
const char Property1[] = "NH3_1_PPM";

void Put(String ThingName, String ThingProperty, float Value)
{

  Serial.println(host);
  if (!client.connect(host, httpsPort))
  {
    Serial.println("connection failed");
    return;
  } else

  {
    Serial.println("Connected to ThingWorx.");
  }
  String url = "/Thingworx/Things/" + ThingName + "/Properties/" + ThingProperty;
  Serial.print("requesting URL: ");
  Serial.println(url);

  String strPUTReqVal = "{\"" + ThingProperty + "\":\"" + Value + "\"}";
  Serial.print("PUT Value: ");
  Serial.println(strPUTReqVal);

  client.print(String("PUT ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "appKey: 9cf679e1-979b-4e28-9cbf-627187f30772" + "\r\n" +
               "x-thingworx-session: false" + "\r\n" +
               "Accept: application/json" + "\r\n" +
               "Connection: close" + "\r\n" +
               "Content-Type: application/json" + "\r\n" +
               "Content-Length: " + String(strPUTReqVal.length()) + "\r\n\r\n" +
               strPUTReqVal + "\r\n\r\n");

  while (client.connected())
  {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  client.stop();
}


bool setPowerBoostKeepOn(int en) {
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL0);
  if (en) {
    Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  } else {
    Wire.write(0x35); // 0x37 is default reg value
  }
  return Wire.endTransmission() == 0;
}

void setup()
{  t1=0;
   t2=0;
  WiFi.begin(ssid, password);
  Serial.begin(115200);
  Serial.print("connecting to ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  old_time1 = 0;
  old_time2 = 0;
  pinMode(MQ_sensor, INPUT);
  pinMode(light_Green, OUTPUT);
  pinMode(light_Red, OUTPUT);
  pinMode(perfume, OUTPUT);
  pinMode(fan, OUTPUT);
  Wire.begin(I2C_SDA, I2C_SCL);
  bool isOk = setPowerBoostKeepOn(1);
  SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, HIGH);
  digitalWrite(MODEM_RST, LOW);
  digitalWrite(MODEM_POWER_ON, LOW);
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
  SerialMon.println("Initializing modem...");
  modem.restart();
  if (strlen(simPIN) && modem.getSimStatus() != 3 ) {
    modem.simUnlock(simPIN);
  }

}
float mq_reading()
{
  float av = 0;
  for (int i = 0; i < 1000; i++) {
    av = av + analogRead(MQ_sensor);
  }
  av = av / 1000;
  float volt = 3.03 * av / 4095.00;
  Serial.print(" RAW VALUE ADC_34 GPIO_pin ------->");
  Serial.println( av );
  Serial.print("Voltage output across RL(10K) --------->");
  Serial.println( volt );
  return av;
}


void loop() {
  // Mq Reading
  float  NH3_1 = mq_reading();
  Serial.print("NH3_1 in  PPM = ");
  Serial.println(NH3_1);

  ///Actions

  if (NH3_1 >= 150 &&  NH3_1 <= 250 &&  ( millis()- t2) > 60000L )
  {
    t2=0;
    //LOW Alert
    digitalWrite(light_Red, HIGH);  // 15 , l1 on RED LIGHT ON!!
    digitalWrite(light_Green, LOW); // 4 l4, off   GREEN LIGHT OFF
    Serial.println(" Washroom Dirty ");
    digitalWrite(fan, HIGH);  //2, l3 on   eX FAN ON!!1
    Serial.println("Exhaust FAN ON !");
       t1= millis();
    digitalWrite(perfume, LOW);// 0, l2, off   PERFUME OFF!!!
    Serial.println("PERFUME_OFF");
    Serial.println(" Module is awaiting for 4 Min.......");

  }
    
    else if ( (NH3_1 >=251  && NH3_1 <= 650) && ((millis() - old_time1 )> 60000L) )
    {  
      digitalWrite(light_Red, HIGH);
       digitalWrite(fan, HIGH);
       digitalWrite(light_Green, LOW);
       digitalWrite(perfume, HIGH);
       Serial.println(" SMS sent........");
       
      Serial.println(" SMS READY TO SEND ");
      String smsMessage1 = "Washroom_1 Sanitization Required !";

      if ((modem.sendSMS(SMS_TARGET1, smsMessage1)) || (modem.sendSMS(SMS_TARGET2, smsMessage1)))
       {
        SerialMon.println(smsMessage1);
        modem.restart();
      }
      else {
        SerialMon.println("SMS failed to send");
      }
      old_time1 = millis();
      
      }


       

  
  
  else if( NH3_1 <= 149 &&  (millis()- t1) > 60000L )
  { // No Action Required
    t1=0;
    digitalWrite(light_Red, LOW);
    digitalWrite(perfume, LOW);
    digitalWrite(fan, LOW);
    digitalWrite(light_Green, HIGH);
    Serial.println("Exhaust Fan Turned Off !");
    Serial.println("No Action Required-Okey condition !");
    t2= millis();
    
  }
  else
  {// all load off
    
    digitalWrite(light_Red, HIGH);
    digitalWrite(perfume, LOW);
    digitalWrite(fan, LOW);
    digitalWrite(light_Green, HIGH);
    Serial.println(" Default Mode- May be Cleaning Taking Place!");
    
  }

  Put(Thing, Property1, NH3_1);
  delay(60000L);
}
