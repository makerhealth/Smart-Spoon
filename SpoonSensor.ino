#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include "BMA250.h"
#include <avr/power.h>
#include <avr/sleep.h>
#include "DSRTCLib.h"

//--------------Set standard deviation threshold for acceleration magintude-------------//
float stdthreshold = 100.0;

//--------------Set LEDPIN for tremor sensing indication------------------------//
int tremorlight = 5;

//Accel Setup
BMA250 accel;
//Logger Setup
const int chipSelect = 10;
           
//RTC Setup
int ledPin =  13;    // LED connected to digital pin 13
int INT_PIN = 3; // INTerrupt pin from the RTC. On Arduino Uno, this should be mapped to digital pin 2 or pin 3, which support external interrupts
int int_number = 1; // On Arduino Uno, INT0 corresponds to pin 2, and INT1 to pin 3
                                                          
DS1339 RTC = DS1339();

int iCounter = 0;
float norm = 0.0;
float accelnorms[5] = {0.0,0.0,0.0,0.0,0.0};
float mean = 0.0;
float resid= 0.0;
float stdev = 0.0;

void setup(void) {
    
  pinMode(ledPin, OUTPUT);    
  digitalWrite(ledPin, LOW);
  pinMode(tremorlight, OUTPUT);
  digitalWrite(tremorlight,LOW);
  Serial.begin(9600);
  Wire.begin();
  accel.begin(BMA250_range_2g, BMA250_update_time_64ms);           
                              
  //Initialize SD
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

    // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (dataFile){
    dataFile.println("......................");
    dataFile.println("Date, Time, X, Y, Z, Temp(C)");
    dataFile.close(); 
  }
  else
  {
    Serial.println("Couldn't open data file");
    return;
  }

  //Initialize RTC
    Serial.println ("DSRTCLib Tests");

  RTC.start(); // ensure RTC oscillator is running, if not already
  //set_time();
        
}

void loop() {
  String ahoraDate;
  String ahoraTime;
  currentTime(ahoraDate, ahoraTime);
  String ahora = ahoraDate + "  " + ahoraTime;

  accel.read();

  //calculate sliding window standard deviation of acceleration norms:
  norm = pow(square(accel.X)*1.0 + square(accel.Y) + square(accel.Z),0.5);

  for(int m= 0; m<4; m++){
    accelnorms[m] = accelnorms[m+1];
  }
  accelnorms[4] = norm;

  mean = 0;
  resid = 0;
  
  for(int m = 0; m<5; m++){
    mean = mean + accelnorms[m]/5.0;
  }

  for(int m = 0; m<5; m++){
    resid = resid + square(accelnorms[m] - mean)/5.0;
  }
  
  stdev = pow(resid, 0.5);  
  //Serial.println(stdev);

  if(stdev > stdthreshold){
    digitalWrite(tremorlight,HIGH);
  }
  else{
    digitalWrite(tremorlight,LOW);
  }
                                                      
  //Logging
  String dataString = "";
  dataString = String(ahoraDate)  + ", " + String(ahoraTime)  + ", " + String(accel.X) + ", " + String(accel.Y) + ", " + String(accel.Z) + ", " + String((accel.rawTemp*0.5)+24.0,1);
  
  // if the file is available, write to it:
  File dataFile = SD.open("datalog.csv", FILE_WRITE);
  if(dataFile)
  {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  }
  else
  {
    Serial.println("Couldn't access file");
  }
  delay(64);
                                 
}





int read_int(char sep)
{
  static byte c;
  static int i;

  i = 0;
  while (1)
  {
    while (!Serial.available())
    {;}
 
    c = Serial.read();
                       
  
    if (c == sep)
    {
                                         
                           
      return i;
    }
    if (isdigit(c))
    {
      i = i * 10 + c - '0';
    }
    else
    {
      Serial.print("\r\nERROR: \"");
      Serial.write(c);
      Serial.print("\" is not a digit\r\n");
      return -1;
    }
  }
}

int read_int(int numbytes)
{
  static byte c;
  static int i;
  int num = 0;

  i = 0;
  while (1)
  {
    while (!Serial.available())
    {;}
 
    c = Serial.read();
    num++;
                       
  
    if (isdigit(c))
    {
      i = i * 10 + c - '0';
    }
    else
    {
      Serial.print("\r\nERROR: \"");
      Serial.write(c);
      Serial.print("\" is not a digit\r\n");
      return -1;
    }
    if (num == numbytes)
    {
                                         
                           
      return i;
    }
  }
}

int read_date(int *year, int *month, int *day, int *hour, int* minute, int* second)
{

  *year = read_int(4);
  *month = read_int(2);
  *day = read_int(' ');
  *hour = read_int(':');
  *minute = read_int(':');
  *second = read_int(2);

  return 0;
}


void currentTime(String &nowDate, String &nowTime){
   RTC.readTime();
   nowDate = String(int(RTC.getMonths())) + "/" + String (int(RTC.getDays())) + "/" + String(RTC.getYears()-2000);
   nowTime = String(int(RTC.getHours())) + ":" + String(int(RTC.getMinutes())) + ":" + String(int(RTC.getSeconds()));  
}  


void set_time()
{
    Serial.println("Enter date and time (YYYYMMDD HH:MM:SS)");
    int year, month, day, hour, minute, second;
    int result = read_date(&year, &month, &day, &hour, &minute, &second);
    if (result != 0) {
      Serial.println("Date not in correct format!");
      return;
    } 
    
                             
    RTC.setSeconds(second);
    RTC.setMinutes(minute);
    RTC.setHours(hour);
    RTC.setDays(day);
    RTC.setMonths(month);
    RTC.setYears(year);
    RTC.writeTime();
    read_time();
}

void read_time() 
{
  Serial.print ("The current time is ");
  RTC.readTime();                                          
  printTime(0);
  Serial.println();
  
}


void printTime(byte type)
{
                                                          
  if(!type)
  {
    Serial.print(int(RTC.getMonths()));
    Serial.print("/");  
    Serial.print(int(RTC.getDays()));
    Serial.print("/");  
    Serial.print(RTC.getYears());
  }
  else
  {
                                                                                                                 
    {
      Serial.print(int(RTC.getDayOfWeek()));
      Serial.print("th day of week, ");
    }
          
    {
      Serial.print(int(RTC.getDays()));
      Serial.print("th day of month, ");      
    }
  }
  
  Serial.print("  ");
  Serial.print(int(RTC.getHours()));
  Serial.print(":");
  Serial.print(int(RTC.getMinutes()));
  Serial.print(":");
  Serial.print(int(RTC.getSeconds()));  
}


