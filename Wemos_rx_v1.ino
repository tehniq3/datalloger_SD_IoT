/* 
* WeMos Mini rx - ver.0 
* original from https://nicuflorica.blogspot.com/2019/10/vizualizare-parametrii-panou-solar-pe.html
*/

#include <ESP8266WiFi.h>
//--------------Your Data----------------//
String apiKey = "apikeyfromThingspeak"; // <<<<< YOUR API KEY
const char* ssid = "niq_ro"; // <<<<<<<< Your Wi-Fi SSID 
const char* password = "meritabere"; // <<<<<<<< Your Wi-Fi Pasword
//--------------------------------------//
const char* server = "api.thingspeak.com";


byte semn1, semn2;
float Temp1, Temp2;
float Temp11, Temp21;
float RH1, RH2;

WiFiClient client;
unsigned char buff[17], i;
String buffer1, buffer2;

void setup()
{
  Serial.begin(9600);
  delay(10);
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.println();
  Serial.print("Se conecteaza la ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectat");
}

void loop()
{
  if (Serial.available() > 0)
  {
    delay(100);
    while (Serial.available() > 0)
    {
      buffer1 = Serial.readString();
      if (buffer1[0] == '*')
      {
        if (buffer1[17] == '#')
        {
          Serial.println(buffer1);
     semn1 = buffer1[1] - 0x30;
     Temp11 = ((buffer1[2] - 0x30)*100 + (buffer1[3] - 0x30)*10 + (buffer1[4] - 0x30));
     if (semn1 == 0)
         Temp1 = Temp11/10.;
       else
         Temp1 = -Temp1/10.1;
     RH1 = ((buffer1[5] - 0x30) * 1000 + (buffer1[6] - 0x30)*100 + (buffer1[7] - 0x30)*10 + (buffer1[8] - 0x30));
     RH1 = RH1/10.;
     semn2 = buffer1[9] - 0x30;
     Temp21 = ((buffer1[10] - 0x30)*100 + (buffer1[11] - 0x30)*10 + (buffer1[12] - 0x30));
     if (semn2 == 0)
         Temp2 = Temp21/10.;
       else
         Temp2 = -Temp21/10.;
     RH2 = ((buffer1[13] - 0x30) * 1000 + (buffer1[14] - 0x30)*100 + (buffer1[15] - 0x30)*10 + (buffer1[16] - 0x30));
     RH2 = RH2/10.;
        }
      }
    }
Serial.println(" ----------------------- ");
Serial.print("Temp1 = ");
Serial.print(Temp1);
Serial.print("[°C], RH1 = ");
Serial.print(RH1);
Serial.print("%, Temp2 = ");
Serial.print(Temp2);
Serial.print("[°C], RH2 = ");
Serial.print(RH2);
Serial.println("% ----------------------- ");    

 if (client.connect(server, 80))
  {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(Temp1);
    postStr += "&field2=";
    postStr += String(RH1);
    postStr += "&field3=";
    postStr += String(Temp2);  
    postStr += "&field4=";
    postStr += String(RH2);       
    postStr += "\r\n\r\n";
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
  //  Serial.println(postStr);
  }
  client.stop();
  Serial.println("I'm waitting new data...");
  } 
}
//-------------Electronics-project-hub>com-------------//
