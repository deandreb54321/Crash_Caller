#include <SoftwareSerial.h>
#include <Wire.h>
#include "ADXL345lib.h"
#include "TinyGPS++.h"
#include "talkie.h"

// Configure software serial port (RX, TX)
SoftwareSerial SIM900(8, 7); 
SoftwareSerial ss(11, 10);

Accelerometer acc;
TinyGPSPlus gps;
Talkie voice;

bool fail;
bool crashDetected;
int crashThreshold;
int button;
int talkTime = 30000;
bool stopSms;
String message = "Testing";
String number = "14169862726";
String longitude;
String latitude;
String altitude;
String currentDate;
String currentTime;
double x, y, z, g;

void setup() {
  // Arduino communicates with SIM900 GSM shield at a baud rate of 19200
  // Make sure that corresponds to the baud rate of your module
  SIM900.begin(19200);
  ss.begin(9600);
  Serial.begin(9600);
  if (acc.begin(OSEPP_ACC_SW_ON) != 0)
  {
    Serial.println("Error connecting to accelerometer");
    fail = true;
    return;
  }

    acc.setSensitivity(ADXL345_RANGE_PM8G);
  
  // Give time to your GSM shield log on to network
  delay(20000);   
  
}

void loop() { 
  if (fail)
    return;

  if (acc.readGs(&x, &y, &z) != 0)
  {
    Serial.println("Failed to read accelerometer");
    return;
  }

  g = sqrt(x*x + y*y + z*z);

  if (g > crashThreshold)
  {
    Serial.println("About to contact EMS");
    voice.say(spCALL);voice.say(spN);voice.say(spNINE);voice.say(spONE);voice.say(spONE);voice.say(spN);voice.say(spTEN);voice.say(spSECONDS);
    delay(1000);
    voice.say(spPUSH);voice.say(spBUTTON);voice.say(spTWO);voice.say(spCANCEL);
    
    for (int i = 0; i < 10000; i++)
    {
      if (digitalRead(button) == 1)
      {
        stopSms = true;
        break;
      }
      delay(1);
    }
    if (stopSms == false)
    {
      Serial.println("Calling EMS");
      call();
      voice.say(spCALL);voice.say(spN);voice.say(spNINE);voice.say(spONE);voice.say(spONE);
      
      Serial.println("Current location information");
      voice.say(spDANGER);voice.say(spIS);voice.say(spAT);voice.say(spPOSITION);
      Serial.print("Longitude:  ");
      longitude = returnFloat(gps.location.lng(), gps.location.isValid(), 6);
      //sayNumber(longitude);
      voice.say(spAND);
      Serial.print("Latitude: ");
      latitude = returnFloat(gps.location.lat(), gps.location.isValid(), 6);
      //sayNumber(latitude);
      
      Serial.print("  Altitude: ");
      altitude = returnFloat(gps.altitude.meters(), gps.altitude.isValid(), 2);
      Serial.print("  Date:  ");
      currentDate = returnDate(gps.date);
      Serial.print("  Time: ");
      currentTime = returnTime(gps.time);
      Serial.println();
      
      smartDelay(1000);
      sendSMS(message);
    }
    else
    {
      Serial.println("Cancelled call");
    }
  }
}

void call() {
  // REPLACE THE X's WITH THE NUMER YOU WANT TO DIAL
  // USE INTERNATIONAL FORMAT CODE FOR MOBILE NUMBERS
  SIM900.print("ATD + +");
  SIM900.print(number);
  SIM900.println(";");
  delay(100);
  SIM900.println();
  
 // In this example, the call only last 30 seconds
 // You can edit the phone call duration in the delay time
  delay(talkTime);
  // AT command to hang up
  SIM900.println("ATH"); // hang up
}

void sendSMS(String message) {
  // AT command to set SIM900 to SMS mode
  SIM900.print("AT+CMGF=1\r"); 
  delay(100);

  // REPLACE THE X's WITH THE RECIPIENT'S MOBILE NUMBER
  // USE INTERNATIONAL FORMAT CODE FOR MOBILE NUMBERS
  SIM900.print("AT + CMGS = \"+");
  SIM900.print(number);
  SIM900.println("\""); 
  delay(100);
  
  // REPLACE WITH YOUR OWN SMS MESSAGE CONTENT
  SIM900.println(message); 
  delay(100);

  // End AT command with a ^Z, ASCII code 26
  SIM900.println((char)26); 
  delay(100);
  SIM900.println();
  // Give module time to send SMS
  delay(5000); 
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static String returnFloat(float val, bool valid, int prec)
{
  String floatString;
  if (!valid){}
  else
  {
    floatString = String(val, prec);
  }
  smartDelay(0);
  return floatString;
}

static String returnDate(TinyGPSDate &d)
{
  String dateString;
  if (!d.isValid()){}
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    dateString = String(sz);
  }
  smartDelay(0);
  return dateString;    
}

static String returnTime(TinyGPSTime &t)
{
  String timeString;
  if (!t.isValid()){}
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    timeString = String(sz);
  }
  smartDelay(0);
  return timeString;
}
