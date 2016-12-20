//Erik Work
// Working example based off
//   - https://github.com/ArduCAM/Arduino/blob/master/ArduCAM/examples/mini/ArduCAM_Mini_Capture2SD/ArduCAM_Mini_Capture2SD.ino

// Comment out the below line to stop debugging transmit across the serial port
#include "arduinoprintf.h"

// ArduCAM demo (C)2016 Lee
// web: http://www.ArduCAM.com
// This program is a demo of how to use most of the functions
// of the library with a supported camera modules, and can run on any Arduino platform.
//
// This demo was made for Omnivision 2MP/5MP sensor.
// It will run the ArduCAM ESP8266 5MP as a real 2MP digital camera, provide both JPEG capture.
// The demo sketch will do the following tasks:
// 1. Set the sensor to JPEG mode.
// 2. Capture and buffer the image to FIFO every 5 seconds 
// 3. Store the image to Micro SD/TF card with JPEG format in sequential.
// 4. Resolution can be changed by myCAM.set_JPEG_size() function.
// This program requires the ArduCAM V4.0.0 (or later) library and ArduCAM 2MP/5MP shield
// and use Arduino IDE 1.5.2 compiler or above
#include <ArduCAM.h>
#include <Wire.h>
#include <SPI.h>
#include <FileIO.h>
#include <avr/eeprom.h>
#include "memorysaver.h"
#include "drwatson_includes.h"

//This demo can only work on OV2640_MINI_2MP or OV5642_MINI_5MP or OV5642_MINI_5MP_BIT_ROTATION_FIXED
//or OV5640_MINI_5MP_PLUS or ARDUCAM_SHIELD_V2 platform.
#if !(defined OV5642_MINI_5MP || defined OV5642_MINI_5MP_BIT_ROTATION_FIXED || defined OV2640_MINI_2MP)
  #error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif
#define SD_CS 9
/*
4 Camera Adapter Board Pins
const int CS1 = 4;
const int CS2 = 5;
const int CS3 = 6;
const int CS4 = 7;
*/

/* 
#define OV2640_160x120    0 //160x120
#define OV2640_176x144    1 //176x144
#define OV2640_320x240    2 //320x240
#define OV2640_352x288    3 //352x288
#define OV2640_640x480    4 //640x480
#define OV2640_800x600    5 //800x600
#define OV2640_1024x768   6 //1024x768
#define OV2640_1280x1024  7 //1280x1024
#define OV2640_1600x1200  8 //1600x1200
*/
int iCaptureSize = OV2640_1024x768;

const int SPI_CS = 7;
ArduCAM *myCAM;

const unsigned int nTimeDelay = 60000;
unsigned long nLastRequest = 0;      // when you last made a request

#ifndef _DR_WATSON_INCLUDES_
  #error You must include a header with _AUTHORIZATION_TOKEN_ and _DRWATSON_ACCOUNT_URL defined
#endif

// Store the original root error for failure. We will print this over and over.
char cError[100]={0};

// Location is overlay because I did expanded the YUM dick space with an SD overlay
//  - root@linino:~# overlay-only -i
//  - http://www.arduino.org/forums/linino-and-openwrt/expand-the-yun-disk-space-598
//#define _CAMERA_ROOT_ "/overlay/DCIM/"
#define _CAMERA_ROOT_ "/mnt/sda1/DCIM/"

String myCAMSaveToSDFile() {
  String sFile(_CAMERA_ROOT_);
  char str[8];
  byte buf[256];
  static int i = 0;
  static int k = 0;
  int iCounter = 0;
  uint8_t temp = 0,temp_last=0;
  nLastRequest = millis();
  //Flush the FIFO
  myCAM->flush_fifo();
  //Clear the capture done flag
  myCAM->clear_fifo_flag();
  //Start capture
  myCAM->start_capture();
  printf("%s:%d start Capture infinite while loop\n", __FILE__, __LINE__);
  while(!myCAM->get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK));

  

  printf("%s:%d Capture Done.\n", __FILE__, __LINE__); 

  String sTIme = getTimeStamp();
  printf("%s:%d Called getTimeStamp\n", __FILE__, __LINE__); 
  printf("%s:%d time=%s!\n", __FILE__, __LINE__, sTIme.c_str()); 

  // DELETE ME
  k = 0;

  //Construct a file name
  k = k + 1;
  itoa(k, str, 10);
  strcat(str, ".jpg");
  sFile.concat(str);
 
  //Open the new file
  printf("%s:%d Trying to open file=%s\n", __FILE__, __LINE__, sFile.c_str());
 
  File file = FileSystem.open(sFile.c_str(), FILE_WRITE);
 
  if( !file ){
   printf("%s:%d open file failed. Filename: %s\n", __FILE__, __LINE__, sFile.c_str());
   sFile = "";
   return sFile;
  } else {
    printf("%s:%d File open success\n", __FILE__, __LINE__); 
  }
 
  i = 0;
  myCAM->CS_LOW();
  myCAM->set_fifo_burst();
  temp=SPI.transfer(0x00);
  //Read JPEG data from FIFO
  while ( (temp !=0xD9) | (temp_last !=0xFF)){
    ++iCounter;
    if ( iCounter % 100 == 0 ) {
      printf("%s:%d in loop. iCounter=%d. temp=0x%02x, temp_last=0x%02x\n", __FILE__, __LINE__, iCounter, temp, temp_last); 
    }
    temp_last = temp;
    temp = SPI.transfer(0x00);
    //Write image data to buffer if not full
    if( i < 256) {
       buf[i++] = temp;
    } else {
       //Write 256 bytes image data to file
       myCAM->CS_HIGH();
       file.write(buf ,256);
       i = 0;
       buf[i++] = temp;
       myCAM->CS_LOW();
       myCAM->set_fifo_burst();
    }
    delay(0);  
 }

  printf("%s:%d after while\n", __FILE__, __LINE__); 
 
 //Write the remain bytes in the buffer
 if(i > 0){
  myCAM->CS_HIGH();
  file.write(buf,i);
 }
 //Close the file
 file.close();
 printf("%s:%d CAM Save Done!\n", __FILE__, __LINE__);
 return sFile;
}

void setup(){
  uint8_t vid, pid;
  uint8_t temp = 0;
  Serial.begin(115200);
  Bridge.begin();    // To communicate with the YUN
  //Serial.begin(9600);
  //Console.begin();
  //while(!Console);   // wait for Serial port to connect.
  Serial.print("Beginning");
  printf("%s:%d Serial.begin\n", __FILE__, __LINE__); 
  delay(5000);
   printf("%s:%d byte size=%d, char size=%d, int size=%d\n", __FILE__, __LINE__, sizeof(byte), sizeof(char), sizeof(int)); 
  
#if defined (OV2640_MINI_2MP)
  myCAM = new ArduCAM( OV2640, SPI_CS );
#else
  myCAM = new ArduCAM( OV5642, SPI_CS );
#endif  
  
  if (!FileSystem.begin()) {
    sprintf(cError, "%s:%d FileSystem.begin Failed !", __FILE__, __LINE__);
    printf("%s", cError);
    return;
  }

  Wire.begin();
  printf("%s:%d ArduCAM Start!!\n", __FILE__, __LINE__); 

  //set the CS as an output:
  pinMode(SPI_CS,OUTPUT);

  printf("%s:%d Starting SPI test temp=%d (0x%02x). ARDUCHIP_TEST1=%d\n", __FILE__, __LINE__, temp, temp, ARDUCHIP_TEST1); 
  // initialize SPI:
  SPI.begin();
  //SPI.beginTransaction (SPISettings(16000000, MSBFIRST, SPI_MODE0));
  //SPI.beginTransaction (SPISettings(15000000, MSBFIRST, SPI_MODE0));
  //SPI.beginTransaction (SPISettings(16000000, MSBFIRST, SPI_MODE3));
 
  delay(5000);
  //Check if the ArduCAM SPI bus is OK
  myCAM->write_reg(ARDUCHIP_TEST1, 0x55);
  delay(1000);
  temp = myCAM->read_reg(ARDUCHIP_TEST1);
   
  if (temp != 0x55){
    sprintf(cError, "%s:%d SPI1 interface Error!. temp=%d (0x%02x)\n", __FILE__, __LINE__, temp, temp); 
    printf("%s", cError);
    return;
    //while(1);
  } else {
    printf("%s:%d SPI success!!! temp=%d (0x%02x)\n", __FILE__, __LINE__, temp, temp); 
  }
   
 #if defined (OV2640_MINI_2MP)
     //Check if the camera module type is OV2640
     myCAM->wrSensorReg8_8(0xff, 0x01);  
     myCAM->rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
     myCAM->rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
     if ((vid != 0x26) || (pid != 0x42)) {
       sprintf(cError, "%s:%d Can't find OV2640 module!. vid=0x%02x, pid=0x%02x\n", __FILE__, __LINE__, vid, pid);
       printf("%s", cError);
       return;
     } else {
       printf("%s:%d OV2640 detected. vid=0x%02x, pid=0x%02x\n", __FILE__, __LINE__, vid, pid);
     }
  #else
   //Check if the camera module type is OV5642
    myCAM->wrSensorReg16_8(0xff, 0x01);
    myCAM->rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    myCAM->rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
     if((vid != 0x56) || (pid != 0x42)) {
       sprintf(cError, "%s:%d Can't find OV5642 module!. vid=0x%02x, pid=0x%02x\n", __FILE__, __LINE__, vid, pid);
       printf("%s", cError);
       return;
     } else {
       printf("%s:%d OV5642 detected. vid=0x%02x, pid=0x%02x\n", __FILE__, __LINE__, vid, pid);
     }
  #endif
   myCAM->set_format(JPEG);
   myCAM->InitCAM();
 #if defined (OV2640_MINI_2MP)
   myCAM->OV2640_set_JPEG_size(iCaptureSize);
  #else
   myCAM->write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
   myCAM->OV5642_set_JPEG_size(iCaptureSize);
 #endif
  delay(1000);
}

void loop(){
  int iStatus;
  if ( cError[0] == '\0' ) {
    runCpuInfo();
    String sImageFile = myCAMSaveToSDFile();
    if ( sImageFile.length() > 0 ) {
      iStatus = sentImage(sImageFile);
      printf("%s:%d loop() iStatus=%d\n", __FILE__, __LINE__, iStatus);
      if ( iStatus != 0 ) {
        delay(5000000);
      }
    }
  } else {
    printf("%s:%d No capture. Previous error was\n\t%s\n", __FILE__, __LINE__, cError);
  }
  delay(nTimeDelay);
}

/*
curl -v -XPUT \
  -H "X-Auth-Token:  gAAAAABYTeji7MZoZcGZ5JukqPtJs9Ta_ZmM1CFtKZsxeQEYhk9roSPcuT7P_87ixZb_G7II9MgwwGkLdQ7TkQU9Q_xJ1zggdcoJiYn6uIox9Vwf-rxSi9nymk-g53pxM7u6NVOVwC64m0Auqrr1YekH444IEixI5Wpr5GenYMbRCyaaeC9ngDM" \
  --data-binary "@/DCIM/1.jpg" \
  "https://admin_cfc512f33dd8137c55d6e7e53c0577ecfcfeefe6:en0rLK2tD3~y.,BF@dal.objectstorage.open.softlayer.com/v1/AUTH_2f419a57b90442998d94564693c1252f/bluebirds/1.jpg
*/

int sentImage(const String &sImage) {
  Process drWatson;
  String sParam;
  printf("%s:%d Sending image %s to cloud\n", __FILE__, __LINE__, sImage.c_str());
  drWatson.begin("curl");
  drWatson.addParameter("-XPUT");
  drWatson.addParameter("-H");
  sParam = "X-Auth-Token: ";
  sParam += _AUTHORIZATION_TOKEN_;
  drWatson.addParameter(sParam);
  drWatson.addParameter("--data-binary");
  sParam = "@";
  sParam += sImage;
  drWatson.addParameter(sParam);
  sParam = _DRWATSON_ACCOUNT_URL;
  sParam += "1.jpg";
  drWatson.addParameter(sParam);
  drWatson.run();

#ifdef _DEBUG_
  delay(2000);
  // read the output of the command
  sParam = "";
  int iInitAvail = 0;
  iInitAvail = drWatson.available();
  while(drWatson.available()>0) {
    char c = drWatson.read();
    sParam += c;
  }
  printf("%s:%d iInitAvail=%d. Results of curl call=\n\t%s\n", __FILE__, __LINE__, iInitAvail, sParam.c_str());
#endif

  int iReturn = drWatson.exitValue();
  printf("%s:%d drWatson.exitValue()=%d\n", __FILE__, __LINE__, iReturn);

#ifdef _DEBUG_
  delay(2000);
  // read the output of the command
  sParam = "";
  iInitAvail = drWatson.available();
  while(drWatson.available()>0) {
    char c = drWatson.read();
    sParam += c;
  }
  printf("%s:%d iInitAvail=%d, Results of curl call=\n\t%s\n", __FILE__, __LINE__, iInitAvail, sParam.c_str());
#endif
  
  return iReturn;
}

String runCpuInfo() {
  Process p;
  String sReturn("");
  p.begin("cat");       
  p.addParameter("/proc/cpuinfo"); 
  p.run();
  int iReturn = p.exitValue();
  
  while(p.available()>0) {
    char c = p.read();
    sReturn += c;
  }
  printf("%s:%d runCpuInfo iReturn=%d. Results call=\n\t%s\n", __FILE__, __LINE__, iReturn, sReturn.c_str());
  return sReturn;
}

// getTimeStamp function return a string with the time stamp
// Yun Shield will call the Linux "date" command and get the time stamp
String getTimeStamp() {
  String result;
  Process time;
  // date is a command line utility to get the date and the time 
  // in different formats depending on the additional parameter 
  time.begin("date");
  time.addParameter("+%D-%T");   // parameters: D for the complete date mm/dd/yy
  //              T for the time hh:mm:ss
  time.run();   // run the command
  // read the output of the command
  while(time.available()>0) {
    char c = time.read();
    if(c != '\n')
      result += c;
  }
  return result;
}
