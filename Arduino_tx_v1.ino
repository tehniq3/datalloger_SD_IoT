/* Real time clock, calendar, temperature, humidity data logger using Arduino, DS3231 and DHT22 sensor
 * original sketch from https://simple-circuit.com/arduino-datalogger-sd-card-ds3231-dht22/
 * combined with https://nicuflorica.blogspot.com/2019/10/vizualizare-parametrii-panou-solar-pe.html
 * ver.1.0 - added second DHT22 sensor by Nicu FLORICA (niq_ro), 6.3.2021 
 * ver.2.0 - added tx sender on SoftwareSerial for Wemos Mini (ESP8266)
 */

#include <SoftwareSerial.h>
#include <SPI.h>              // Include SPI library (needed for the SD card)
#include <SD.h>               // Include SD library
//#include <LiquidCrystal.h>    // Include LCD library code
#include <LiquidCrystal_I2C.h>  // https://github.com/tehniq3/used_library
#include <Wire.h>             // Include Wire library code (needed for I2C protocol devices)
#include <DHT.h>              // Include DHT library code - https://github.com/tehniq3/used_library

#define B1      2  //A1            // Button B1 is connected to Arduino pin A1
#define B2      3  //A2            // Button B1 is connected to Arduino pin A2
#define DHTPIN1  4  //A3            // DHT22 data pin is connected to Arduino pin A3
#define DHTPIN2  5  

#define tx 7
#define rx 8
 
#define DHTTYPE DHT22         // DHT22 sensor is used
DHT dht1(DHTPIN1, DHTTYPE);     // Initialize DHT library
DHT dht2(DHTPIN2, DHTTYPE);     // Initialize DHT library

LiquidCrystal_I2C lcd(0x3F, 20, 4);  // // Set the LCD address to ox3F (try 0x27) for a 20 chars and 4 line display

SoftwareSerial mySerial(rx, tx); // RX, TX
 
File dataLog;
boolean sd_ok = 0;
char temperature1[] = " 00.0";
char temperature2[] = " 00.0";
char humidity1[]    = " 00.0";
char humidity2[]    = " 00.0";
char Taim[]     = "  :  :  ";
char Calendar[] = "  /  /20  ";
byte i, second, minute, hour, date, month, year, previous_second;
int Temp1, RH1, Temp11;
int Temp2, RH2, Temp21;

unsigned long previoustp = 30000;
unsigned long previoustp2 = 0;
unsigned long previoustp3 = 0;
unsigned long nani = 30000;  // pause time between sensors readings, in ms
unsigned long nani2 = 600000;  // pause time between SD card writting, in ms

byte a0, a1, a2, a3;
byte b0, b1, b2, b3, b4;
byte c0, c1, c2, c3;
byte d0, d1, d2, d3, d4;
int ar, br, cr, dr;

 
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  mySerial.begin(9600);
  Serial.print("Initializing SD card...");
  if (!SD.begin())
    Serial.println("initialization failed!");
  else {
    Serial.println("initialization done.");
    sd_ok = 1;
  }
  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
                          
  Wire.begin();  // Join i2c bus
  lcd.begin();  // initialize the LCD
//  lcd.init();   // initialize the LCD (for other library)
  lcd.clear();
  lcd.backlight();
  
  dht1.begin();
  dht2.begin();
  
  lcd.setCursor(6, 1);
  lcd.print("Zona 1");
  lcd.setCursor(14, 1);
  lcd.print("Zona 2");  
  lcd.setCursor(0, 2);
  lcd.print("Temp:");
  lcd.setCursor(10, 2);
  lcd.write(223);     // Print degree symbol ( °)
  lcd.setCursor(11, 2);  lcd.write('C');
  lcd.setCursor(0, 3);   lcd.print("RH:        %       %");
  lcd.setCursor(18, 2);  lcd.write(223);     // Print degree symbol ( °)
  lcd.setCursor(19, 2);  lcd.write('C');
  
  Serial.println("   DATE    |   TIME   | TEMP1 [°C] | HUMI1 [%] | TEMP2 [°C] | HUMI2 [%] ");
  
  if(sd_ok) {                                       // If SD card initialization was OK
    dataLog = SD.open("Logger.txt", FILE_WRITE);    // Open file Logger.txt
    if(dataLog) {                                   // if the file opened okay, write to it:
      dataLog.println("   DATE    |   TIME   | TEMP1 [°C] | HUMI1 [%] | TEMP2 [°C] | HUMI2 [%] ");
      dataLog.close();                              // Close the file
    }
  }
  
//lcd.noBacklight();
DS3231_read();
DS3231_display();
masuratori();
previoustp = millis();

} // end setup
 
void DS3231_display(){
  // Convert BCD to decimal
  second = (second >> 4) * 10 + (second & 0x0F);
  minute = (minute >> 4) * 10 + (minute & 0x0F);
  hour   = (hour >> 4)   * 10 + (hour & 0x0F);
  date   = (date >> 4)   * 10 + (date & 0x0F);
  month  = (month >> 4)  * 10 + (month & 0x0F);
  year   = (year >> 4)   * 10 + (year & 0x0F);
  // End conversion
  Taim[7]     = second % 10 + 48;
  Taim[6]     = second / 10 + 48;
  Taim[4]      = minute % 10 + 48;
  Taim[3]      = minute / 10 + 48;
  Taim[1]      = hour   % 10 + 48;
  Taim[0]      = hour   / 10 + 48;
  Calendar[9] = year   % 10 + 48;
  Calendar[8] = year   / 10 + 48;
  Calendar[4]  = month  % 10 + 48;
  Calendar[3]  = month  / 10 + 48;
  Calendar[1]  = date   % 10 + 48;
  Calendar[0]  = date   / 10 + 48;                               // Display time
  lcd.setCursor(10, 0);
  lcd.print(Calendar);                           // Display calendar
  lcd.setCursor(0, 0);
  lcd.print(Taim);
  lcd.setCursor(1, 1);
}

void blink_parameter(){
  byte j = 0;
  while(j < 10 && digitalRead(B1) && digitalRead(B2)){
    j++;
    delay(25);
  }
}
byte edit(byte x, byte y, byte parameter){
  char text[3];
  while(!digitalRead(B1));                        // Wait until button (pin #8) released
  while(true){
    while(!digitalRead(B2)){                      // If button (pin #9) is pressed
      parameter++;
      if(i == 0 && parameter > 23)               // If hours > 23 ==> hours = 0
        parameter = 0;
      if(i == 1 && parameter > 59)               // If minutes > 59 ==> minutes = 0
        parameter = 0;
      if(i == 2 && parameter > 31)               // If date > 31 ==> date = 1
        parameter = 1;
      if(i == 3 && parameter > 12)               // If month > 12 ==> month = 1
        parameter = 1;
      if(i == 4 && parameter > 99)               // If year > 99 ==> year = 0
        parameter = 0;
      sprintf(text,"%02u", parameter);
      lcd.setCursor(x, y);
      lcd.print(text);
      delay(200);                                // Wait 200ms
    }
    lcd.setCursor(x, y);
    lcd.print("  ");                             // Display two spaces
    blink_parameter();
    sprintf(text,"%02u", parameter);
    lcd.setCursor(x, y);
    lcd.print(text);
    blink_parameter();
    if(!digitalRead(B1)){                         // If button (pin #8) is pressed
      i++;                                       // Increament 'i' for the next parameter
      return parameter;                          // Return parameter value and exit
    }
  }
}
 
void loop() {
  if (!digitalRead(B1) or (!digitalRead(B2)))
  {
  lcd.backlight();
  previoustp3 = millis();
  }
  if (millis() - previoustp3 > 5*nani)
  {
    lcd.noBacklight();
  }
  
  if(!digitalRead(B1)){                           // If button (pin #8) is pressed
    i = 0;
    hour   = edit(0, 0, hour);
    minute = edit(3, 0, minute);
    date   = edit(10, 0, date);
    month  = edit(13, 0, month);
    year   = edit(18, 0, year);
    // Convert decimal to BCD
    minute = ((minute / 10) << 4) + (minute % 10);
    hour = ((hour / 10) << 4) + (hour % 10);
    date = ((date / 10) << 4) + (date % 10);
    month = ((month / 10) << 4) + (month % 10);
    year = ((year / 10) << 4) + (year % 10);
    // End conversion
    // Write data to DS3231 RTC
    Wire.beginTransmission(0x68);               // Start I2C protocol with DS3231 address
    Wire.write(0);                              // Send register address
    Wire.write(0);                              // Reset sesonds and start oscillator
    Wire.write(minute);                         // Write minute
    Wire.write(hour);                           // Write hour
    Wire.write(1);                              // Write day (not used)
    Wire.write(date);                           // Write date
    Wire.write(month);                          // Write month
    Wire.write(year);                           // Write year
    Wire.endTransmission();                     // Stop transmission and release the I2C bus
    delay(200);                                 // Wait 200ms
  }
 
  DS3231_read();                    // Read year from register 6
  
  DS3231_display();                             // Diaplay time & calendar
  
  if(previous_second != second){
    previous_second = second;

//  if ((millis() - previoustp > nani) or (millis() < nani))
  if (millis() - previoustp > nani)
  {
    masuratori();
    previoustp = millis();
    }
    
    if (millis() - previoustp2 > nani2)
    {
    trimitere();
    previoustp2 = millis();
    }
    if (millis() - previoustp2 > 500)
    lcd.setCursor(0, 1);
    lcd.print(" ");
  }
  
  delay(50);                                          // Wait 50ms
} // end main loop

void masuratori()
{
      RH1 = dht1.readHumidity() * 10;      // Read humidity x 10   
    Temp1 = dht1.readTemperature() * 10;  //Read temperature in degree Celsius x 10
      RH2 = dht2.readHumidity() * 10;      // Read humidity x 10    
    Temp2 = dht2.readTemperature() * 10;  //Read temperature in degree Celsius x 10
   /*
     RH1 = random(0,+1000);  // for tests
    Temp1 = random(-300,+400);  // for tests
    RH2 = random(0,+1000);  // for tests
    Temp2 = random(-300,+400);  // for tests
    */
  //  previoustp = millis();
 // }
    
    if(Temp1 < 0){
      temperature1[0] = '-';                     // If temperature < 0 put minus sign
      Temp11 = abs(Temp1);                         // Absolute value of 'Temp'
    }
    else
    {
      temperature1[0] = ' ';                     // otherwise (temperature > 0) put space
      Temp11 = abs(Temp1); 
    }
    if (abs(Temp1) >= 100)  
     temperature1[1]   = (Temp11 / 100) % 10  + 48;
    else
    {
     temperature1[0]    = ' '; 
     if(Temp2 < 0)
      temperature1[1] = '-'; 
     else
      temperature1[1] = ' '; 
    }
    temperature1[2]   = (Temp11 / 10)  % 10  + 48;
    temperature1[4]   =  Temp11 % 10 + 48;
    if(RH1 >= 1000)
      humidity1[0]    = '1';                     // If humidity >= 100.0% put '1' of hundreds
    else
      humidity1[0]    = ' ';                     // otherwise (humidity < 100) put space
    if(RH1 >= 100)
     humidity1[1]      = (RH1 / 100) % 10 + 48;
    else
     humidity1[1]    = ' ';      
    humidity1[2]      = (RH1 / 10) % 10 + 48;
    humidity1[4]      =  RH1 % 10 + 48;

    lcd.setCursor(5, 2);
    lcd.print(temperature1);
    lcd.setCursor(5, 3);
    lcd.print(humidity1);
    
    if(Temp2 < 0){
      temperature2[0] = '-';                     // If temperature < 0 put minus sign
      Temp21 = abs(Temp2);                         // Absolute value of 'Temp'
    }
    else
    {
      temperature2[0] = ' ';                     // otherwise (temperature > 0) put space
      Temp21 = Temp2;
    }   
    if (abs(Temp2) >= 100)  
     temperature2[1]   = (Temp21 / 100) % 10  + 48;
    else
    {
     temperature2[0]    = ' '; 
     if(Temp2 < 0)
      temperature2[1] = '-'; 
     else
      temperature2[1] = ' '; 
    }
    temperature2[2]   = (Temp21 / 10)  % 10  + 48;
    temperature2[4]   =  Temp21 % 10 + 48;
    if(RH2 >= 1000)
      humidity2[0]    = '1';                     // If humidity >= 100.0% put '1' of hundreds
    else
      humidity2[0]    = ' ';                     // otherwise (humidity < 100) put space
    if(RH2 >= 100)
     humidity2[1]      = (RH2 / 100) % 10 + 48;    
    else
     humidity2[1]    = ' ';    
    humidity2[2]      = (RH2 / 10) % 10 + 48;
    humidity2[4]      =  RH2 % 10 + 48;
 
    lcd.setCursor(13, 2);
    lcd.print(temperature2);
    lcd.setCursor(13, 3);
    lcd.print(humidity2);
 
    // Send data to Arduino IDE serial monitor
    Serial.print(Calendar);
    Serial.print(" | ");
    Serial.print(Taim);
    Serial.print(" |   ");
    Serial.print(temperature1);
    Serial.print("    |  ");
    Serial.print(humidity1);
    Serial.print("    |   ");
    Serial.print(temperature2);
    Serial.print("    |  ");
    Serial.println(humidity2);
}

void trimitere()
{
      lcd.setCursor(0, 1);
    lcd.print("x");
    if(sd_ok) {                                       // If SD card initialization was OK
      dataLog = SD.open("Logger.txt", FILE_WRITE);    // Open file Logger.txt
      if(dataLog) {                                   // if the file opened okay, write to it:
        dataLog.print(Calendar);
        dataLog.print(" | ");
        dataLog.print(Taim);
        dataLog.print(" |   ");
        dataLog.print(temperature1);
        dataLog.print("    | ");
        dataLog.print(humidity1);
        dataLog.print("   |   ");
        dataLog.print(temperature2);
        dataLog.print("    | ");
        dataLog.println(humidity2);        
        dataLog.close();                              // Close the file
      }
     }

if (Temp1 < 0)   
   a0 = 1;
 else
   a0 = 0; 
a1 = Temp11/100;
ar = Temp11%100;
a2 = ar/10;
a3 = ar%10;

b1 = RH1/1000;
br = RH1%1000;
b2 = br/100;
br = br%100;
b3 = br/10;
b4 = br%10;

if (Temp2 < 0)   
   c0 = 1;
 else
   c0 = 0; 
c1 = Temp21/100;
cr = Temp21%100;
c2 = cr/10;
c3 = cr%10;

d1 = RH2/1000;
dr = RH2%1000;
d2 = dr/100;
dr = dr%100;
d3 = dr/10;
d4 = dr%10;

//------Sending Data to receiver--------//
    mySerial.print('*'); // Starting char
    mySerial.print(a0); 
    mySerial.print(a1); 
    mySerial.print(a2); 
    mySerial.print(a3); 
    mySerial.print(b1); 
    mySerial.print(b2); 
    mySerial.print(b3); 
    mySerial.print(b4);
    mySerial.print(c0); 
    mySerial.print(c1); 
    mySerial.print(c2); 
    mySerial.print(c3); 
    mySerial.print(d1); 
    mySerial.print(d2); 
    mySerial.print(d3); 
    mySerial.print(d4);
    mySerial.print('#'); // Ending char
    //------------------------------------//
}

void DS3231_read()
{
   Wire.beginTransmission(0x68);                 // Start I2C protocol with DS3231 address
  Wire.write(0);                                // Send register address
  Wire.endTransmission(false);                  // I2C restart
  Wire.requestFrom(0x68, 7);                    // Request 7 bytes from DS3231 and release I2C bus at end of reading
  second = Wire.read();                         // Read seconds from register 0
  minute = Wire.read();                         // Read minuts from register 1
  hour   = Wire.read();                         // Read hour from register 2
  Wire.read();                                  // Read day from register 3 (not used)
  date   = Wire.read();                         // Read date from register 4
  month  = Wire.read();                         // Read month from register 5
  year   = Wire.read();     
}
