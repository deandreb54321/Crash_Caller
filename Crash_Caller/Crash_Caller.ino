/*
 * Author: Deandre Baker
 * Description: The source code for the crash caller project
 */

// Includes the following libraries
#include <SoftwareSerial.h>
#include <Wire.h>
#include "TinyGPS++.h"
#include "talkie.h"
#include "ADXL345lib.h"

// Creates 2 software serial objects
SoftwareSerial SIM900(8, 7);
SoftwareSerial ss(11, 10);

// Creates an instance of the following objects
TinyGPSPlus gps;
Talkie voice;
Accelerometer acc;

// Boolean variables
bool fail = false;
bool stopSms = false;
bool crashDetected = false;

// Pins
int button = 4;
int relay = 5;

// String variables
String message = "";
String number = "14165668783";
String longitude;
String latitude;
String altitude;
String currentDate;
String currentTime;

// Double variables
double x, y, z, g;

void setup()
{
  // Initializes serial communication between the arduino and follwing devices at a specific baud rate
  Serial.begin(115200);
  SIM900.begin(19200);
  
  // Exits 'void setup()' if accelerometer is not detected
  if (acc.begin(OSEPP_ACC_SW_ON) != 0)
  {
    Serial.println("Error connecting to accelerometer");
    fail = true;
    return;
  }
  
  // Adjusts sensitivity and range of accelerometer
  acc.setSensitivity(ADXL345_RANGE_PM16G);
  
  // Declares the type of each pin
  ss.begin(9600);
  pinMode(relay, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  
  // Displays text on the serial monitor
  Serial.println("Crash Caller has been turned on");
}

void loop()
{
  // Exits 'void loop' if fail is true and the arduino failed to connect to accelerometer
  if (fail)
    return;
    
  // Reads the acceleration values along 3 axes and exits 'void loop()' if no values are detected
  if (acc.readGs(&x, &y, &z) != 0)
  {
    Serial.println("Failed to read accelerometer");
    return;
  }
  
  // Calculates net acceleration
  g = sqrt(x * x + y * y + z * z);
  
  // Reads data from the gps and stores it into the follwing string variables
  longitude = returnFloat(gps.location.lng(), gps.location.isValid(), 6);
  latitude = returnFloat(gps.location.lat(), gps.location.isValid(), 6);
  altitude = returnFloat(gps.altitude.meters(), gps.altitude.isValid(), 2);
  currentDate = returnDate(gps.date);
  currentTime = returnTime(gps.time);
  
  // Displays acceleration values
  Serial.print("X: ");
  Serial.print(x);
  Serial.print(" ");
  Serial.print("Y: ");
  Serial.print(y);
  Serial.print(" ");
  Serial.print("Z: ");
  Serial.print(z);
  Serial.print(" ");
  Serial.print("G: ");
  Serial.println(g);
  
  // Reports a crash if acceleration values are above the thresholds or if the car is upside-down or sideways
  if ((abs(g * 100) >= 150) || (abs(x * 100) >= abs(z * 100)) || (abs(y * 100) >= abs(z * 100)))
    crashDetected = true;
    
  // Runs following block if a crash is reported
  if (crashDetected)
  {
    // Alerts the user of a potential accident and waits 10 seconds before calling emergency services
    ss.end();
    digitalWrite(relay, HIGH);
    Serial.println("Crash Detected. About to contact EMS");
    delay(1000);
    voice.say(spCALL); voice.say(spN); voice.say(spNINE); voice.say(spONE); voice.say(spONE); voice.say(spN); voice.say(spTEN); voice.say(spSECONDS);
    delay(1000);
    voice.say(spPUSH); voice.say(spBUTTON); voice.say(spTWO); voice.say(spCANCEL);
    digitalWrite(relay, LOW);
    delay(1000);
    
    // Aborts the call if the button is pressed within 10 seconds
    for (int i = 0; i < 50; i++)
    {
      if (digitalRead(button) == 0)
      {
        stopSms = true;
        break;
      }
      delay(100);
    }
    
    // Contacts EMS if button is not pressed within 10 seconds
    if (stopSms == false)
    {
      // Following lines display gps information
      Serial.println("Calling EMS");
      Serial.println("Current location information");
      Serial.print("Longitude:  ");
      Serial.print(longitude);
      Serial.print("  Latitude: ");
      Serial.print(latitude);
      Serial.print("  Altitude: ");
      Serial.print(altitude);
      Serial.print("  Date:  ");
      Serial.print(currentDate);
      Serial.print("  Time: ");
      Serial.print(currentTime);
      Serial.println();
      
      // Following lines alert user that a call to EMS is being made
      digitalWrite(relay, HIGH);
      delay(1000);
      voice.say(spCALL); voice.say(spN); voice.say(spNINE); voice.say(spONE); voice.say(spONE);
      digitalWrite(relay, LOW);
      delay(1000);
      
      // Sends a SMS with the accident location information
      message = "Crash detected at location " + longitude + ", " + latitude;
      sendSMS();
      delay(2000);
      
      // Starts a call to EMS and waits 15 seconds for responder to answer
      call();
      delay(5000);
      
      // Runs an infinite loop as long as the button is not pressed
      do
      {
        digitalWrite(relay, HIGH);
        delay(1000);
        voice.say(spALERT); voice.say(spDANGER); voice.say(spIS); voice.say(spAT); voice.say(spPOSITION);
        delay(1000);
        sayNumber(longitude, "longitude");
        voice.say(spAND);
        sayNumber(latitude, "latitude");
        digitalWrite(relay, LOW);
        delay(1000);
      } while (digitalRead(button) == 1);
      // Hangs up
      SIM900.println("ATH");
    }
    // Cancels the call if the button was pressed within 10 seconds
    else
    {
      // Alerts the user that the call will not be made
      stopSms = false;
      Serial.println("Cancelled call");
      digitalWrite(relay, HIGH);
      delay(1000);
      voice.say(spCALL); voice.say(spIS); voice.say(spCANCEL);
      digitalWrite(relay, LOW);
      delay(1000);
    }
    
    // Resets 'crashDetected' to false
    crashDetected = false;
  }
  
  // 10 millisecond delay while simultaneously reading gps location information
  smartDelay(10);
  
}

// Function that calls a phone number
void call()
{
  // Sends the "AT" Command for calling to the SIM900 module and provides a phone number
  SIM900.print("ATD + +");
  SIM900.print(number);
  SIM900.println(";");
  delay(100);
  SIM900.println();
}

// Function that sends a sms message
void sendSMS()
{
  // Uses the "AT" commands for sending text messages and provides a number and a string message
  SIM900.print("AT+CMGF=1\r");
  delay(100);
  SIM900.print("AT + CMGS = \"+");
  SIM900.print(number);
  SIM900.println("\"");
  delay(100);
  SIM900.println(message);
  delay(100);
  SIM900.println((char)26);
  delay(100);
  SIM900.println();
  delay(5000);
}

// Function that creates a time delay while reading gps data 
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    // Reads gps data
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms); // Calculates the difference between the current and start run time of the function
}

// Function that returns a string for the specified gps location data
static String returnFloat(float val, bool valid, int prec)
{
  String floatString;
  if (!valid) {}
  else
  {
    // The 'prec' parameter determines the number of digits after the decimal place for 'val'
    floatString = String(val, prec);
  }
  smartDelay(0);
  return floatString;
}

// Function that returns a string for the gps date data
static String returnDate(TinyGPSDate &d)
{
  String dateString;
  if (!d.isValid()) {}
  else
  {
    // Arranges the date data into a readable format
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    dateString = String(sz);
  }
  smartDelay(0);
  return dateString;
}

// Function that returns a string for the gps time data
static String returnTime(TinyGPSTime &t)
{
  String timeString;
  if (!t.isValid()) {}
  else
  {
    // Arragnes the time data into a readable format
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    timeString = String(sz);
  }
  smartDelay(0);
  return timeString;
}

// Function for saying a number
void sayNumber(String number, String type)
{
  // Declares and assigns a private boolean variable
  bool negative = false;
  
  // Runs the 'for' loop as many times as there are digits in 'number'
  for (int i = 0; i < number.length(); i++)
  {
    // Says the corresponding number for the digit at position 'i', starting from the left
    switch (number.charAt(i))
    {
      case '-':
        negative = true; // Changes 'negative' to true if the number is negative
        break;
      case '0':
        voice.say(spZERO);
        break;
      case '1':
        voice.say(spONE);
        break;
      case '2':
        voice.say(spTWO);
        break;
      case '3':
        voice.say(spTHREE);
        break;
      case '4':
        voice.say(spFOUR);
        break;
      case '5':
        voice.say(spFIVE);
        break;
      case '6':
        voice.say(spSIX);
        break;
      case '7':
        voice.say(spSEVEN);
        break;
      case '8':
        voice.say(spEIGHT);
        break;
      case '9':
        voice.say(spNINE);
        break;
      case '.':
        voice.say(spPOINT);
        break;
    }
  }
  
  voice.say(spDEGREES);
  
  // Runs following block if the number is negative
  if (negative)
  {
    // Says "South" if the number is the latitude
    if (type == "latitude")
    {
      voice.say(spSOUTH);
    }
    // Says "West" if the number is the longitude
    else if (type == "longitude")
    {
      voice.say(spWEST);
    }
  }
  // Runs following block if the number is positive
  else
  {
    // Says "North" if the number is the latitude
    if (type == "latitude")
    {
      voice.say(spNORTH);
    }
    // Says "East" if the number is the longitude
    else if (type == "longitude")
    {
      voice.say(spEAST);
    }
  }
}
