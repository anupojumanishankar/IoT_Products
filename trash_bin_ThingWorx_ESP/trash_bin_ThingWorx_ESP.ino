#include <ESP8266WiFi.h>
#include <Servo.h>
WiFiClient client;
#define red_led D7
#define green_led D6
#define alarm  D5
#define trigPin_obj D0
#define echoPin_obj D1
#define trigPin_level D2
#define echoPin_level D3
bool Full_empety;
bool Open_close;
long DURATION_obj;
float DISTANCE_obj;
long DURATION_level;
float DISTANCE_level;
const float velocity=0.034;
Servo myservo; 


const char* ssid = "MANI DCS";
const char* password = "Mani1337";
/*const char* ssid = "dlink-B2CE";
const char* password = "Scetngp2021";  
/*const char* ssid = "realme";
const char* password = "eeeetttt";  */

const char* host = "thingworx.scetngp.com"; 
const int httpsPort = 80;

const char Thing[] = "dust_bin_sensors_Mani";
const char Property1[] = "Full_empety";
const char Property2[] = "Open_close";
const char Property3[] = "Ultrasconic_level";
const char Property4[] = "Ultrasconic_obj";

void Put(String ThingName, String ThingProperty, int Value)
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
               "appKey: 6b3c910d-8ce5-4d18-916d-e9240a97ca0b" + "\r\n" +
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

void setup() 
{
  pinMode(alarm, OUTPUT);
  pinMode(red_led,OUTPUT);
  pinMode(green_led,OUTPUT);
 pinMode(trigPin_obj, OUTPUT);
 pinMode(echoPin_obj, INPUT);
 pinMode(trigPin_level, OUTPUT);
 pinMode(echoPin_level, INPUT);
  myservo.attach(D4);  
  Serial.begin(115200);
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
 {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

void loop() 
{
  digitalWrite(trigPin_obj, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin_obj, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin_obj, LOW);
  DURATION_obj = pulseIn(echoPin_obj, HIGH);
  DISTANCE_obj = 0.034 * DURATION_obj / 2;
  float Ultrasconic_obj = DISTANCE_obj;
  delay(300);
  digitalWrite(trigPin_level, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin_level, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin_level, LOW);
  DURATION_level = pulseIn(echoPin_level, HIGH);
  DISTANCE_level = 0.034 * DURATION_level / 2;
  float Ultrasconic_level = DISTANCE_level;

  Serial.print("distance_obj=");
  Serial.println(DISTANCE_obj);
  Serial.print("distance_level=");
  Serial.println(DISTANCE_level);
 delay(300);


 if(DISTANCE_level>5 && DISTANCE_obj<7)
  {
     Open_close = true;
     Full_empety = false;
    
    Serial.println("Trash detected,Garbage level under limit, Dust bin opened.");
    digitalWrite(green_led,HIGH);
    delay(300);
     digitalWrite(green_led,LOW);
    delay(300);
    digitalWrite(red_led,LOW);
    tone(alarm,1000);
    delay(2000);
    noTone(alarm);
    myservo.write(180);
      delay(2500);
  }
   /*else if(DISTANCE_level<5 && DISTANCE_obj<7)
    {
       Serial.println("Trash detected,Garbage level over the limit, Dust bin remain colsed.");  
     Open_close = true;
     Full_empety = false;
    myservo.write(0);         
  }*/
  else
  {
    
    Serial.println("Trash Not detected, Dust bin remain colsed.");
    Open_close = false;
     Full_empety = true;
     digitalWrite(red_led,HIGH);
    delay(300);
     digitalWrite(red_led,LOW);
    delay(300);
    digitalWrite(green_led,LOW);
    tone(alarm,300);
    delay(2000);
    noTone(alarm);
    myservo.write(0);
  } 
  Put(Thing,Property1,Full_empety);
  Put(Thing,Property2,Open_close);
  Put(Thing,Property3,DISTANCE_level);  
  Put(Thing,Property4,DISTANCE_obj);
  
  delay(500);
}
