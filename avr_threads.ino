/*
I2C addresses:

Found address: 60 (0x3C)     - OLED 4-line display
Found address: 60 (0x3C)     - OLED 8-lines
Found address: 87 (0x57)     - RTC 1
Found address: 104 (0x68)    - RTC 2
Found address: 119 (0x77)    - pressure sensor
not found AM2320, 92 >>> 0x5C ?



*/



//#include <LiquidCrystal.h>
#include <Thread.h>
//#include <stdlib.h>   // for itoa()
#include <SPI.h>      // for sd card
#include <SD.h>
#include <Wire.h>     // for AM2320
//#include <AM2320.h>   // for AM2320
#include "Adafruit_Sensor.h"  // for AM2320, connect to I2C
#include "Adafruit_AM2320.h"
#include "Adafruit_BMP085.h"  // Pressure sensor, I2C (with pullup resistors onboard)
#include "DS3231.h"   // RTC
#include <U8x8lib.h>

#define PAMMHG (0.00750062)

//LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
//AM2320 temp_humid;
Adafruit_AM2320 temp_humid = Adafruit_AM2320();
Adafruit_BMP085 bmp;
RTClib RTC;

//U8X8_SSD1306_128X32_UNIVISION_SW_I2C oled(/* clock=*/ 5, /* data=*/ 4, /* reset=*/ U8X8_PIN_NONE);   // OLEDs without Reset of the Display
U8X8_SSD1306_128X32_UNIVISION_HW_I2C oled(/* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA);   // pin remapping with ESP8266 HW I2C

Thread taskOne = Thread();  // thread for task one, print time in seconds since controller start
Thread taskTwo = Thread();  // thread for task two, animation for ">" moving in the first LCD row between
                            // positions 9 and 15
Thread taskThr = Thread();  // thread for task three, read and print light sensor data at the end of the second row
Thread taskFour = Thread(); // read temp & humidity sensor, print to serial

typedef struct {
  char symbol;// = ' ';
  bool dir;// = false;
  int x_begin;// = 9;
  int x_end;// = 15;
  int x_old;// = 12;
  int x_coord;// = 12; 
} Arrow;

//Arrow* animatedArrow = (Arrow*)malloc(sizeof(Arrow));
Arrow animatedArrow[] = {' ', false, 9, 15, 9, 9};

File logfile;         //lghtsnsr.log
File logTempHumid;    // temphd.log

void taskOneFunc(){
  // Printing seconds since restart on the first row
  // lcd.setCursor(5, 0);
  // lcd.write("    ");
  // lcd.setCursor(5, 0);
  // lcd.print(millis() / 1000);
  oled.setCursor(5, 0);
  oled.print(millis() / 1000);
}

void taskTwoFunc(){
  // Running arrow ">" or "<"
  //Arrow animatedArrow;
  arrowStep(animatedArrow);
  //delay(120);
//  lcd.setCursor(15, 1);
//  lcd.write(' ');
//  lcd.setCursor(14, 1);
//  lcd.write('@');
}

void taskThreeFunc(){
  // Read and display light sensor data at the end of the second line
  int lightSensorVal = analogRead(A0);
  int sc;
  //char buf[4];                                // for variant with itoa() of lightSensorVal
  sc = lightSensorVal < 1000 ? 13 : 12;       // align 3-4 digit value
  sc = lightSensorVal < 100 ? sc + 1 : sc;    // consider 2 digit value as well
  oled.setCursor(12, 1);
  oled.print("    ");                          // erase previous value
  oled.setCursor(sc, 1);
  oled.print(lightSensorVal);                  // print new value (0-4 digits)
  // write to log
  /* Uncomment this -------------*/
  // logfile = SD.open("lghtsnsr.log", FILE_WRITE);
  // if (logfile){
  //   //logfile.printf("Light sensor value: %s", itoa(lightSensorVal, buf, 10));  // itoa() variant
  //   logfile.print(String("** Light sensor value: ") + lightSensorVal + " time[" + String(millis() / 1000, DEC) + "] s" + '\n');   // using String wrapping, which has dynamic concatenation
  //   logfile.close();
  //   lcd.setCursor(sc - 1, 1);
  //   lcd.print("+");             // indicate successful writing to the log
  // } else {
  //   lcd.setCursor(sc - 1, 1);
  //   lcd.print("?");  
  // }
  /* ------------------------------*/
}

void taskFourFunc(){
  // Read and display & send temperature & humidity from AM2320
  // and athmospheric pressure taken from BMP180 GY68 Digital Barometric Pressure Sensor Board Module compatible with BMP085
//  switch(temp_humid.Read()){
//    case 2:
//      Serial.println("Temperature sensor: CRC failed");
//      break;
//    case 1:
//      Serial.println("Temperature sensor: Offline");
//      break;
//    case 0:
//  Serial.print("Temperature: ");
//  Serial.print(temp_humid.readTemperature());
//  Serial.print(" C, humidity: ");
//  Serial.print(temp_humid.readHumidity());
//  Serial.println(" %");
  
  DateTime now = RTC.now();
  int year = now.year();
  int mnth = now.month();
  int day = now.day();
  int hour = now.hour();
  int minu = now.minute();
  int seco = now.second();
  //String s = seco < 10? ":0" : ":"
  String logString = String("Time: ") 
    + (year < 10 ? "0" : ""  ) + year
    + (mnth < 10 ? "/0" : "/") + mnth
    + (day  < 10 ? "/0" : "/") + day
    + (hour < 10 ? " 0" : " ") + hour
    + (minu < 10 ? ":0" : ":") + minu 
    + (seco < 10 ? ":0" : ":") + seco + "," 
    + " Temperature: " + temp_humid.readTemperature() 
    + " C, Humidity: " + temp_humid.readHumidity() 
    + " %, Pressure: " + bmp.readPressure() * PAMMHG + " mmHg\n";
  /* Uncomment this -------------*/
  // logTempHumid = SD.open("temphd.log", FILE_WRITE);
  // if(logTempHumid){
  //   //logTempHumid.print(String("AM2320: Temperature: ") + temp_humid.readTemperature() + " C, Humidity: " + temp_humid.readHumidity() + " %" + " time[" + String(millis() / 1000, DEC) + "] s" + '\n');
  //   //logTempHumid.print(String("BMP180: Temperature: ") + bmp.readTemperature() + " C, Pressure: " + bmp.readPressure() * 0.007501 + " mmhHg, Alt: " + bmp.readAltitude() + "m, Pressure (sea level): " + bmp.readSealevelPressure() + '\n');
  //   /*
  //   logTempHumid.print(String("Temperature: ") + temp_humid.readTemperature() + " C, Humidity: " + temp_humid.readHumidity() + " %, Pressure: " + bmp.readPressure() * PAMMHG + " mmHg\n");  
  //   */
  //   logTempHumid.print(logString);
  //   logTempHumid.close();
  // }
  /* -------------------------*/
  //Serial.print(String("AM2320: Temperature: ") + temp_humid.readTemperature() + " C, Humidity: " + temp_humid.readHumidity() + " %" + " time[" + String(millis() / 1000, DEC) + "] s" + '\n');
  //Serial.print(String("BMP180: Temperature: ") + bmp.readTemperature() + " C, Pressure: " + bmp.readPressure() * 0.007501 + " mmhHg, Alt: " + bmp.readAltitude() + "m, Pressure (sea level): " + bmp.readSealevelPressure() + '\n');
  Serial.print(logString);
  // oled.drawString(0, 2, "Temp: ");
  oled.setCursor(0, 2);
  oled.print("Temp.   : ");
  oled.print(int(temp_humid.readTemperature()));
  oled.print(" C");
  oled.setCursor(0, 3);
  oled.print("Humidity: ");
  oled.print(int(temp_humid.readHumidity()));
  oled.print(" %");
  // oled.setCursor(0, 4);
  // oled.print("Pressure: ");
  // oled.print(int(bmp.readPressure() * PAMMHG));
  // oled.print("mmH");
  oled.setCursor(0, 1);
  oled.print("P: ");
  oled.print(int(bmp.readPressure() * PAMMHG));
  oled.print(" mmHg");
  /*
  Serial.print(String("Time: ") 
    + (year < 10 ? "0" : ""  ) + year
    + (mnth < 10 ? "/0" : "/") + mnth
    + (day  < 10 ? "/0" : "/") + day
    + (hour < 10 ? " 0" : " ") + hour
    + (minu < 10 ? ":0" : ":") + minu 
    + (seco < 10 ? ":0" : ":") + seco 
    + " Temperature: " + temp_humid.readTemperature() 
    + " C, Humidity: " + temp_humid.readHumidity() 
    + " %, Pressure: " + bmp.readPressure() * PAMMHG + " mmHg\n"); 
    */
//      break;  
    
//  }
}

void arrowStep(Arrow *self){
  // function performs one single movement of the character ">" or "<"
  // depending on its current movement direction
  
  //char symbol;
  if(self->dir)
    self->symbol = '>';
  else
    self->symbol = '<';
  
  // update LCD 
  oled.setCursor(self->x_old, 0);
  oled.write(" ");
  oled.setCursor(self->x_coord, 0);
  oled.write(self->symbol);

  // update current coordinate and direction
  if (self->x_coord == self->x_end || self->x_coord == self->x_begin)
    self->dir = !self->dir;
  self->x_old = self->x_coord;
  if (self->dir)
    self->x_coord++;
   else
    self->x_coord--;

  // // debug info on the second row: current coordinate and moving direction
  // oled.setCursor(3, 1);
  // oled.print("        ");
  // int sc;
  // sc = self->x_coord < 10 ? 4 : 3;    // align one/two-digit number
  // oled.setCursor(sc, 1);
  // oled.print(self->x_coord);
  // oled.print(self->dir ? "true" : "false");
}

void sdCardProgram() {
  // SD reader is connected to SPI pins (MISO/MOSI/SCK/CS, 5V pwr)
  // setup sd card variables
  Sd2Card card;
  SdVolume volume;
  SdFile root;
  
  //oled.begin();
  oled.setCursor(0, 0);
  oled.print("SD Card Init...");
  oled.setCursor(0, 1);
  if (!card.init(SPI_HALF_SPEED, 4)){
    oled.print("Init failed");
    Serial.print("SD: INIT FAILED");}
  else
    oled.print("Init OK");
  delay(3000);
  //char cardType[10];
  String cardType = "xxxx";
  oled.setCursor(0, 2);
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      cardType = "SD1";
      break;
    case SD_CARD_TYPE_SD2:
      cardType = "SD2";
      break;
    case SD_CARD_TYPE_SDHC:
      cardType = "SDHC";
      break;
    default:
      cardType = "Unkn";
  }
  oled.print("Card Type: ");
  oled.println(cardType);
  delay(5000);
  if (!volume.init(card)) {
    oled.print("No FAT partition");
    delay(3000);
  }
  else {
    oled.clear();
    oled.setCursor(0, 0);
    //lcd.autoscroll();
    uint32_t volumeSize;
    volumeSize = volume.blocksPerCluster();    // clusters are collections of blocks
    volumeSize *= volume.clusterCount();       // we'll have a lot of clusters
    volumeSize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
    oled.print("Vol. size (Kb):");
    oled.setCursor(0, 1);
    oled.print(volumeSize);
    delay(7000);
    oled.clear();
    //root.openRoot(volume);
    // list all files in the card with date and size
    //root.ls();

    SD.begin();   // (CS_pin) -> sd card initialization
    logfile = SD.open("lghtsnsr.log", FILE_WRITE);    // create log file on the card
    if (logfile){
      logfile.println("Starting log...");
      logfile.close();
      oled.print("LOG CREATED [OK]");
    }
    else
      oled.print("FILE ERROR [01]");
    
    delay(5000);
    oled.clear();
    //lcd.noAutoscroll();
  }
}

void setup(){
  Wire.begin();
  Serial.begin(115200);
  
  oled.begin();
  oled.setPowerSave(0);
  oled.setFont(u8x8_font_chroma48medium8_r); // u8x8_font_chroma48medium8_r u8x8_font_px437wyse700a_2x2_r u8g2_font_courB12_tf 
  oled.setContrast(11);
  // oled.drawString(0,0,"Test STRING...");

  sdCardProgram();
  temp_humid.begin();   // init am2320
  bmp.begin();          // init bmp180

  
  oled.clear();
  // lcd.begin(16, 2);
  oled.setCursor(0, 0);
  oled.print("Sec: ");
  oled.setCursor(0, 1);
  //oled.print("D:");
  //Arrow animatedArrow = {" ", true, 12, 15, 12, 12};
  taskOne.onRun(taskOneFunc);
  taskTwo.onRun(taskTwoFunc);
  taskThr.onRun(taskThreeFunc);
  taskFour.onRun(taskFourFunc);
  //taskTwo.onRun(arrowStep(animatedArrow));
  taskOne.setInterval(100);   // call taskOne every 1000 ms
  taskTwo.setInterval(300);   // call taskTwo every 100 ms
  taskThr.setInterval(3770);  // call taskThree every 1.77 sec
  taskFour.setInterval(2000); // call temp & humid every 5 sec
}

void loop(){
  
  if (taskOne.shouldRun())
    taskOne.run();

  if (taskTwo.shouldRun())
    taskTwo.run();

  if (taskThr.shouldRun())
    taskThr.run();

  if (taskFour.shouldRun())
    taskFour.run();
}
