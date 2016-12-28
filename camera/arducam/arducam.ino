/*  Copyright (c) 2016, Erik Anderson  https://www.linkedin.com/in/erikanderson
 * All rights reserved.
 * Standard 3 clause BSD license
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following 
 *    disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
// Comment out the below line to stop debugging transmit across the serial port
#include "arduinoprintf.h"

#include <ArduCAM.h>
#include <SparkFunBME280.h>
#include <Wire.h>
#include <SPI.h>
#include <FileIO.h>
#include "memorysaver.h"

//This demo can only work on OV2640_MINI_2MP or OV5642_MINI_5MP or OV5642_MINI_5MP_BIT_ROTATION_FIXED
//or OV5640_MINI_5MP_PLUS or ARDUCAM_SHIELD_V2 platform.
#if !(defined OV5642_MINI_5MP || defined OV5642_MINI_5MP_BIT_ROTATION_FIXED || defined OV2640_MINI_2MP)
  #error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif

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

static char cFileName[] = __FILE__;

static char cFloat[20];  // Max float string size is 20

const int SPI_CS = 7;
ArduCAM *myCAM = NULL;
//Global sensor object
BME280 *mySensor = NULL;
#define _USE_BME280_ 1

// The 2 below defines because our sketch code space is full
#define _METRIC_OR_IMPERIAL_ 1  // 1 for Celsius and meters. Anything else for Fahrenheit and Feet
#define _SENSOR_SHOW_REGS_ 0   // May not have enough sketch space for this. 1 to turn on BME280 registry printing.

const unsigned long nTimeDelay = 60000000;

// Store the original root error for failure. We will print this over and over.
char cError[100]={0};

// Location is overlay because I did expanded the YUM dick space with an SD overlay
//  - root@linino:~# overlay-only -i
//  - http://www.arduino.org/forums/linino-and-openwrt/expand-the-yun-disk-space-598
//#define _CAMERA_ROOT_ "/overlay/DCIM/"
//#define _CAMERA_ROOT_ "/mnt/sda1/DCIM/"    // For the Seeeduino Cloud
//#define _CAMERA_ROOT_ " /tmp/run/mountd/mmcblk0p1/"      // For the LinKit Smart 7688 Duo
#define _CAMERA_ROOT_ "/overlay/DCIM/"                           // For the LinKit Smart 7688 Duo

bool myCAMSaveToSDFile() {
  char str[8];
  byte buf[256];
  static int i = 0;
  static int k = 0;
  int iCounter = 0;
  uint8_t temp = 0,temp_last=0;
  //Flush the FIFO
  myCAM->flush_fifo();
  //Clear the capture done flag
  myCAM->clear_fifo_flag();
  //Start capture
  myCAM->start_capture();
  printf("%s:%d start Capture infinite while loop\n", cFileName, __LINE__);
  while(!myCAM->get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK));

  printf("Capture Done.\n"); 

  // DELETE ME
  k = 0;

  //Construct a file name
  k = k + 1;
  itoa(k, str, 10);
  strcat(str, ".jpg");
  sprintf((char *)buf,"%s%s", _CAMERA_ROOT_, str);
 
  //Open the new file
  printf("Opening file=%s\n", (char *) buf);
 
  File file = FileSystem.open((char *) buf, FILE_WRITE);
 
  if( !file ) {
    sprintf(cError, "%s:%d open file failed. Filename: %s\n", cFileName, __LINE__, (char *) buf);
    printf("%s", cError);
    return false;
  }
 
  i = 0;
  myCAM->CS_LOW();
  myCAM->set_fifo_burst();
  temp=SPI.transfer(0x00);
  //Read JPEG data from FIFO
  while ( (temp !=0xD9) | (temp_last !=0xFF)){
    ++iCounter;
    if ( iCounter % 1000 == 0 ) {
      printf("in loop. iCounter=%d. temp=0x%02x, temp_last=0x%02x\n", iCounter, temp, temp_last); 
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
 
  //Write the remain bytes in the buffer
  if (i > 0) {
    myCAM->CS_HIGH();
    file.write(buf,i);
  }
  
  //Close the file
  file.close();
  printf("CAM Save Done!\n");
  return true;
}

void setup() {
  uint8_t vid, pid;
  uint8_t temp = 0;
  Serial.begin(115200);
//  Serial1.begin(57600);
  //Bridge.begin();
  //Serial.begin(9600);
  printf("%s:%d setup()\n", cFileName, __LINE__); 
  delay(3000);
  printf("Trying bridge\n");
  Bridge.begin();
  printf("Bridge success\n");
  
#if defined (OV2640_MINI_2MP)
  myCAM = new ArduCAM( OV2640, SPI_CS );
#else
  myCAM = new ArduCAM( OV5642, SPI_CS );
#endif  
  
  if (!FileSystem.begin()) {
    sprintf(cError, "%s:%d FileSystem.begin Failed !", cFileName, __LINE__);
    printf("%s", cError);
    return;
  }

  Wire.begin();

  //set the CS as an output:
  pinMode(SPI_CS,OUTPUT);

  // initialize SPI:
  SPI.begin();

  delay(5000);
  //Check if the ArduCAM SPI bus is OK
  myCAM->write_reg(ARDUCHIP_TEST1, 0x55);
  delay(1000);
  temp = myCAM->read_reg(ARDUCHIP_TEST1);
   
  if (temp != 0x55){
    sprintf(cError, "%s:%d SPI1 interface Error!. temp=%d (0x%02x)\n", cFileName, __LINE__, temp, temp); 
    printf("%s", cError);
    return;
  }
   
 #if defined (OV2640_MINI_2MP)
     //Check if the camera module type is OV2640
     myCAM->wrSensorReg8_8(0xff, 0x01);  
     myCAM->rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
     myCAM->rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
     if ((vid != 0x26) || (pid != 0x42)) {
       sprintf(cError, "%s:%d Can't find OV2640 module!. vid=0x%02x, pid=0x%02x\n", cFileName, __LINE__, vid, pid);
       printf("%s", cError);
       return;
     } else {
       printf("OV2640 detected\n");
     }
  #else
   //Check if the camera module type is OV5642
    myCAM->wrSensorReg16_8(0xff, 0x01);
    myCAM->rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    myCAM->rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
     if((vid != 0x56) || (pid != 0x42)) {
       sprintf(cError, "%s:%d Can't find OV5642 module!. vid=0x%02x, pid=0x%02x\n", cFileName, __LINE__, vid, pid);
       printf("%s", cError);
       return;
     } else {
       printf("OV5642 detected\n");
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

  setupSensor();
}

void loop() {
  int iStatus;
  if ( cError[0] == '\0' ) {
    printf("loop()\n");

    dumpSensors("/overlay/DCIM/1.json");

    if ( myCAMSaveToSDFile() ) {
      printf("%s:%d loop() capture success\n", cFileName, __LINE__, iStatus);
      callBridge();
    }
  } else {
    printf("No capture. Previous error was\n\t%s\n", cError);
  }
  printf("Delaying = %ld\n", nTimeDelay);
  delay(nTimeDelay);
}

void setupSensor() {
  uint8_t temp = 0;
  uint16_t temp16 = 0;
  printf("setupSensor()\n");
#ifdef _USE_BME280_
  mySensor = new BME280();
  mySensor->settings.commInterface = I2C_MODE;
  mySensor->settings.I2CAddress = 0x77;
  mySensor->settings.runMode = 3; //Normal mode
  mySensor->settings.tStandby = 0;
  mySensor->settings.filter = 0;
  mySensor->settings.tempOverSample = 1;
  mySensor->settings.pressOverSample = 1;
  mySensor->settings.humidOverSample = 1;

  //Calling .begin() causes the settings to be loaded
  delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
  temp = mySensor->begin();

#if (_SENSOR_SHOW_REGS_ == 1)
  printf("sensor begin=0x%02x\n", temp);
  
  printf("ID(0xD0): 0x%02x\n", mySensor->readRegister(BME280_CHIP_ID_REG));

  printf("Reset register(0xE0): 0x%02x\n", mySensor->readRegister(BME280_RST_REG));

  printf("ctrl_meas(0xF4): 0x%02x\n", mySensor->readRegister(BME280_CTRL_MEAS_REG));

  printf("ctrl_hum(0xF2): 0x%02x\n", mySensor->readRegister(BME280_CTRL_HUMIDITY_REG));
  
  printf("\n\nDisplaying all regs\n");
  uint8_t memCounter = 0x80;
  uint8_t tempReadData;
  for(int rowi = 8; rowi < 16; rowi++ )
  {
    printf("0x%02x:  ",rowi);
    for(int coli = 0; coli < 16; coli++ )
    {
      tempReadData = mySensor->readRegister(memCounter);
      //printf("%02x%02x ", ((tempReadData >> 4) & 0x0F), (tempReadData & 0x0F));
      printf("%02x ", tempReadData);
      memCounter++;
    }
    printf("\n");
  }
  
  printf("\n\nDisplaying concatenated calibration words\n");
  
  //printf("dig_T1 0x%04\n", mySensor->calibration.dig_T1);
  printf("dig_T1 %d\n", mySensor->calibration.dig_T1);
  printf("dig_T2 %d\n", mySensor->calibration.dig_T2);
  printf("dig_T3 %d\n", mySensor->calibration.dig_T3);

  printf("dig_P1 %d\n", mySensor->calibration.dig_P1);
  printf("dig_P2 %d\n", mySensor->calibration.dig_P2);
  printf("dig_P3 %d\n", mySensor->calibration.dig_P3);
  printf("dig_P4 %d\n", mySensor->calibration.dig_P4);
  printf("dig_P5 %d\n", mySensor->calibration.dig_P5);
  printf("dig_P6 %d\n", mySensor->calibration.dig_P6);
  printf("dig_P7 %d\n", mySensor->calibration.dig_P7);
  printf("dig_P8 %d\n", mySensor->calibration.dig_P8);
  printf("dig_P9 %d\n", mySensor->calibration.dig_P9);
  
  printf("dig_H1 %d\n", mySensor->calibration.dig_H1);
  printf("dig_H2 %d\n", mySensor->calibration.dig_H2);
  printf("dig_H3 %d\n", mySensor->calibration.dig_H3);
  printf("dig_H4 %d4\n", mySensor->calibration.dig_H4);
  printf("dig_H5 %d\n", mySensor->calibration.dig_H5);
  printf("dig_H6 %d\n", mySensor->calibration.dig_H6);

#endif  // _SENSOR_SHOW_REGS_

#endif  // _USE_BME280_

    
}

void dumpSensors(char *cJSONFileName) {
  printf("dumpSensors() to %s\n", cJSONFileName);
  File fSensorData = FileSystem.open(cJSONFileName, FILE_WRITE);
  char *cFloatData;
  
  if ( mySensor ) {
      float fTemp = 0.0;
      File fSensorData = FileSystem.open(cJSONFileName, FILE_WRITE);

      fSensorData.print("{units:");
      fSensorData.print(_METRIC_OR_IMPERIAL_);
      fSensorData.print(",");
      
#if (_METRIC_OR_IMPERIAL_ == 1)
      fTemp = mySensor->readTempC();
      cFloatData = ftoa(fTemp,2);
      printf("Temperature: %s C\n", cFloatData);
#else
      fTemp = mySensor->readTempF();
      cFloatData = ftoa(fTemp,2);
      printf("Temperature: %s F\n", cFloatData);
#endif

      fSensorData.print("temperature:");
      fSensorData.print(cFloatData);
      fSensorData.print(", ");

      fTemp = mySensor->readFloatPressure();
      cFloatData = ftoa(fTemp,2);
      printf("Pressure: %s Pa\n", cFloatData); 

      fSensorData.print("pressure:");
      fSensorData.print(cFloatData);
      fSensorData.print(", ");

#if (_METRIC_OR_IMPERIAL_ == 1)
      fTemp = mySensor->readFloatAltitudeMeters();
      cFloatData = ftoa(fTemp,2);
      printf("Altitude:  %sm\n", cFloatData); 
#else
      fTemp = mySensor->readFloatAltitudeMeters();
      cFloatData = ftoa(fTemp,2);
      printf("Altitude: %sft\n", cFloatData); 
#endif

      fSensorData.print("altitude:");
      fSensorData.print(cFloatData);
      fSensorData.print(", ");

      fTemp = mySensor->readFloatHumidity();
      cFloatData = ftoa(fTemp,2);
      printf("RH: %s%%\n\n", cFloatData); 

      fSensorData.print("rh:");
      fSensorData.print(cFloatData);
      fSensorData.print("}\n");
      fSensorData.close();
  }
}

// reverses a string 'str' of length 'len'
void reverse(char *str, int len)
{
    int i=0, j=len-1, temp;
    while (i<j)
    {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++; j--;
    }
}
 
 // Converts a given integer x to string str[].  d is the number
 // of digits required in output. If d is more than the number
 // of digits in x, then 0s are added at the beginning.
int intToStr(long x, char str[], int d)
{
    int i = 0;
    while (x)
    {
        str[i++] = (x%10) + '0';
        x = x/10;
    }
 
    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';
 
    reverse(str, i);
    str[i] = '\0';
    return i;
}
 
// Low compiled size code to convert float to char array
char *ftoa(float n, int afterpoint)
{
    cFloat[0] = '\0';
    // Extract integer part
    long ipart = (long)n;
 
    // Extract floating part
    float fpart = n - (float)ipart;
 
    // convert integer part to string
    int i = intToStr(ipart, cFloat, 0);
 
    // check for display option after point
    if (afterpoint != 0)
    {
        cFloat[i] = '.';  // add dot
 
        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter is needed
        // to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);
 
        intToStr((int)fpart, cFloat + i + 1, afterpoint);
    }
    return cFloat;
}

// getTimeStamp function return a string with the time stamp
// Yun Shield will call the Linux "date" command and get the time stamp
void callBridge() {
  Process python;
  printf("callBridge()\n");
  // date is a command line utility to get the date and the time 
  // in different formats depending on the additional parameter 
  python.begin("python");
  python.addParameter("sendToCloud.py");   // parameters: D for the complete date mm/dd/yy
  python.run();   // run the command
  // read the output of the command
  while(python.available()>0) {
    char c = python.read();
    printf("%c",c);
  }
}

