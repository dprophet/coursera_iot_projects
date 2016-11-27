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
#include "memorysaver.h"
//This demo can only work on OV2640_MINI_2MP or OV5642_MINI_5MP or OV5642_MINI_5MP_BIT_ROTATION_FIXED
//or OV5640_MINI_5MP_PLUS or ARDUCAM_SHIELD_V2 platform.
#if !(defined OV5642_MINI_5MP || defined OV5642_MINI_5MP_BIT_ROTATION_FIXED || defined OV2640_MINI_2MP)
  #error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif
#define SD_CS 9
const int SPI_CS = 10;
ArduCAM *myCAM;

// Store the original root error for failure. We will print this over and over.
char cError[100]={0};

// Location is overlay because I did expanded the YUM dick space with an SD overlay
//  - root@linino:~# overlay-only -i
//  - http://www.arduino.org/forums/linino-and-openwrt/expand-the-yun-disk-space-598
#define _CAMERA_ROOT_ "/overlay/DCIM/"

void myCAMSaveToSDFile(){
  String sFile(_CAMERA_ROOT_);
  char str[8];
  byte buf[256];
  static int i = 0;
  static int k = 0;
  uint8_t temp = 0,temp_last=0;
  //Flush the FIFO
  myCAM->flush_fifo();
  //Clear the capture done flag
  myCAM->clear_fifo_flag();
  //Start capture
  myCAM->start_capture();
  printf("%s:%d start Capture infinite while loop\n", __FILE__, __LINE__);
  while(!myCAM->get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK));

  printf("%s:%d Capture Done!\n", __FILE__, __LINE__); 

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
   return;
  } else {
    printf("%s:%d File open success\n", __FILE__, __LINE__); 
  }
 
  i = 0;
  myCAM->CS_LOW();
  myCAM->set_fifo_burst();
  temp=SPI.transfer(0x00);
  //Read JPEG data from FIFO
  while ( (temp !=0xD9) | (temp_last !=0xFF)){
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
 
 //Write the remain bytes in the buffer
 if(i > 0){
  myCAM->CS_HIGH();
  file.write(buf,i);
 }
 //Close the file
 file.close();
 printf("%s:%d CAM Save Done!\n", __FILE__, __LINE__); 
}

void setup(){
  uint8_t vid, pid;
  uint8_t temp = 0;
  Serial.begin(115200);
  printf("%s:%d Serial.begin\n", __FILE__, __LINE__); 
  delay(5000);
  
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
  }
   
 #if defined (OV2640_MINI_2MP)
     //Check if the camera module type is OV2640
     myCAM->wrSensorReg8_8(0xff, 0x01);  
     myCAM->rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
     myCAM->rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
     if ((vid != 0x26) || (pid != 0x42)) {
       sprintf(cError, "%s:%d Can't find OV2640 module!\n", __FILE__, __LINE__);
       printf("%s", cError);
       return;
     } else {
       printf("%s:%d OV2640 detected.\n", __FILE__, __LINE__);
     }
  #else
   //Check if the camera module type is OV5642
    myCAM->wrSensorReg16_8(0xff, 0x01);
    myCAM->rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    myCAM->rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
     if((vid != 0x56) || (pid != 0x42)) {
       sprintf(cError, "%s:%d Can't find OV5642 module!\n", __FILE__, __LINE__);
       printf("%s", cError);
       return;
     } else {
       printf("%s:%d OV5642 detected.\n", __FILE__, __LINE__);
     }
  #endif
   myCAM->set_format(JPEG);
   myCAM->InitCAM();
 #if defined (OV2640_MINI_2MP)
   myCAM->OV2640_set_JPEG_size(OV2640_320x240);
  #else
   myCAM->write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
   myCAM->OV5642_set_JPEG_size(OV5642_320x240);
 #endif
  delay(1000);
}

void loop(){
  if ( cError[0] != '\0' ) {
    myCAMSaveToSDFile();
  } else {
    printf("%s:%d No capture. Previous error was\n\t%s\n", __FILE__, __LINE__, cError);
  }
  delay(5000);
}

