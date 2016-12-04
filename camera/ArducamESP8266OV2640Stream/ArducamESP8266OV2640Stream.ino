// ArduCAM Mini demo (C)2016 Lee
// web: http://www.ArduCAM.com
// This program is a demo of how to use most of the functions
// of the library with ArduCAM ESP8266 2MP camera.
// This demo was made for ArduCAM ESP8266 OV2640 2MP Camera.
// It can take photo and send to the Web.
// It can take photo continuously as video streaming and send to the Web.
// The demo sketch will do the following tasks:
// 1. Set the camera to JEPG output mode.
// 2. if server.on("/capture", HTTP_GET, serverCapture),it can take photo and send to the Web.
// 3.if server.on("/stream", HTTP_GET, serverStream),it can take photo continuously as video 
//streaming and send to the Web.

// This program requires the ArduCAM V4.0.0 (or later) library and ArduCAM ESP8266 2MP camera
// and use Arduino IDE 1.5.8 compiler or above.
// Do NOT use arduino.org IDE. It wont work. Must use Arduino.cc

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"
#if !(defined ESP8266 )
#error Please select the ArduCAM ESP8266 UNO board in the Tools/Board
#endif

//This demo can only work on OV2640_MINI_2MP or ARDUCAM_SHIELD_V2 platform.
#if !(defined (OV2640_MINI_2MP)||(defined (ARDUCAM_SHIELD_V2) && defined (OV2640_CAM)))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif
// set GPIO16 as the slave select :
const int CS = 16;


// Motion sensor datasheet https://www.mpja.com/download/31227sc.pdf
const int MOTION_PIN = D4;
#define _USE_MOTION_ 1   // Change to a 0 if you dont want to use a motion sensor

//you can change the value of wifiType to select Station or AP mode.
//Default is AP mode.
int wifiType = 0; // 0:Station  1:AP

//AP mode configuration
//Default is arducam_esp8266.If you want,you can change the AP_aaid  to your favorite name
const char *AP_ssid = "arducam_esp8266"; 
//Default is no password.If you want to set password,put your password here
const char *AP_password = "";

//Station mode you should put your ssid and password
const char *ssid = ""; // Put your SSID here
const char *password = ""; // Put your PASSWORD here

/* OV2640_160x120 OV2640_176x144 OV2640_320x240 OV2640_352x288 OV2640_640x480 V2640_800x600 
   OV2640_1024x768 OV2640_1280x1024 OV2640_1600x1200
*/
int iCaptureSize = OV2640_1024x768;

class PirSensor {
  public:
    PirSensor(const int &iInPinNumber) : 
               m_iPin(iInPinNumber), 
               m_iCalibrateTime(60),     // 60s from datasheet
               m_iStateWaitTime(5),      // 5s from some example code?
               m_lLastMilliseconds(0),
               m_iLastState(-1) {
        pinMode(iInPinNumber, INPUT);
    };

    // Return false if no motion, true if motion
    bool isMotion() {
      int iReturn = 0;
      iReturn = digitalRead(m_iPin);
      printf("%s:%d PirSensor::run isMotion. iReturn=%d\n", __FILE__, __LINE__, iReturn);

      if ( m_iLastState == -1 ) {
        m_iLastState = iReturn;
        m_lLastMilliseconds = millis();
      }

      if ( iReturn )
         return false;
      else
         return true;
    }

    void Calibrate() {
      printf("%s:%d\n\tcalibrating sensor ", __FILE__, __LINE__);
      for(int i = 0; i < m_iCalibrateTime; i++){
        printf("%d ", i);
        delay(1000);
      }
      printf("\n");
      printf("%s:%d sensor calibration complete. ACTIVE\n", __FILE__, __LINE__);
      delay(50);
    }


  private:
    int m_iPin;
    unsigned int m_iCalibrateTime;      // Time in seconds to wait while the sensor comes onliine and calibrates
    unsigned int m_iStateWaitTime;      // Time to wait in seconds before we can assume motion stopped.
    unsigned long m_lLastMilliseconds;  // Last time in milliseconds that the sensor changed states.
    int m_iLastState;                   // Last Sensor State
};

ESP8266WebServer server(80);
PirSensor *myPIRSensor = NULL;  // LAzy create later later because of the digital pin setup

ArduCAM myCAM(OV2640, CS);

void start_capture(){
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
}

void camCapture(ArduCAM &myCAM){
  WiFiClient client = server.client();
  
  size_t len = myCAM.read_fifo_length();
  if (len >= 0x07ffff){
    printf("%s:%d Over size.\n", __FILE__, __LINE__);
    return;
  }else if (len == 0 ){
    printf("%s:%d Size is 0.\n", __FILE__, __LINE__);
    return;
  }
  
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  #if !(defined (ARDUCAM_SHIELD_V2) && defined (OV2640_CAM))
  SPI.transfer(0xFF);
  #endif
  if (!client.connected()) return;
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: image/jpeg\r\n";
  response += "Content-Length: " + String(len) + "\r\n\r\n";
  server.sendContent(response);
  
  static const size_t bufferSize = 4096;
  static uint8_t buffer[bufferSize] = {0xFF};
  
  while (len) {
      size_t will_copy = (len < bufferSize) ? len : bufferSize;
      SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
      if (!client.connected()) break;
      client.write(&buffer[0], will_copy);
      len -= will_copy;
  }
  
  myCAM.CS_HIGH();
}

void serverCapture(){
  printf("%s:%d serverCapture()\n", __FILE__, __LINE__);
  start_capture();
  printf("%s:%d CAM Capturing\n", __FILE__, __LINE__);

  int total_time = 0;

  total_time = millis();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  total_time = millis() - total_time;
  printf("%s:%d capture total_time used (in miliseconds):%d\n", __FILE__, __LINE__, total_time);
  
  total_time = 0;
  
  printf("%s:%d CAM Capture Done!\n", __FILE__, __LINE__);
  total_time = millis();
  camCapture(myCAM);
  total_time = millis() - total_time;
  printf("%s:%d send total_time used (in miliseconds):%d\n", __FILE__, __LINE__, total_time);
  printf("%s:%d CAM send Done!\n", __FILE__, __LINE__);
}

void serverStream(){
  printf("%s:%d serverStream()\n", __FILE__, __LINE__);
  WiFiClient client = server.client();
  
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);
  
  while (1){
    start_capture();
    
    while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
    
    size_t len = myCAM.read_fifo_length();
    printf("%s:%d len=%d\n", __FILE__, __LINE__, len);

    if ( myPIRSensor ) {
        bool bMotion = myPIRSensor->isMotion();
        printf("%s:%d bMotion=%d\n", __FILE__, __LINE__, bMotion);
        
    }
    
    if (len >= 0x07ffff){
       printf("%s:%d Over size.\n", __FILE__, __LINE__);
      continue;
    }else if (len == 0 ){
       printf("%s:%d Size is 0.\n", __FILE__, __LINE__);
      continue;
    }
    
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();
  #if !(defined (ARDUCAM_SHIELD_V2) && defined (OV2640_CAM))
  SPI.transfer(0xFF);
  #endif
  if (!client.connected()) break;
  response = "--frame\r\n";
  response += "Content-Type: image/jpeg\r\n\r\n";
  server.sendContent(response);
    
    static const size_t bufferSize = 4096;
    static uint8_t buffer[bufferSize] = {0xFF};
    
    while (len) {
      size_t will_copy = (len < bufferSize) ? len : bufferSize;
      SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
      if (!client.connected()) break;
      client.write(&buffer[0], will_copy);
      len -= will_copy;
    }
    myCAM.CS_HIGH();
    
    if (!client.connected()) {
      printf("%s:%d Client is gone. Break\n", __FILE__, __LINE__);
      break;
    }
  }
}

void handleNotFound(){
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text/plain", message);
  
  if (server.hasArg("ql")){
    int ql = server.arg("ql").toInt();
    myCAM.OV2640_set_JPEG_size(ql);delay(1000);
    printf("%s:%d QL change to: %s\n", __FILE__, __LINE__, server.arg("ql").c_str());
  }
}

void setup() {
  uint8_t vid, pid;
  uint8_t temp;
#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  Serial.begin(115200);
  printf("%s:%d ArduCAM Start!\n", __FILE__, __LINE__);

  // set the CS as an output:
  pinMode(CS, OUTPUT);

#if _USE_MOTION_==1
  // Set the Motion sensor pin
  myPIRSensor = new PirSensor(MOTION_PIN);
#endif

  // initialize SPI:
  SPI.begin();
  SPI.setFrequency(4000000); //4MHz

  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55){
    printf("%s:%d SPI1 interface Error!\n", __FILE__, __LINE__);
    while(1);
  }

  //Check if the camera module type is OV2640
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
     printf("%s:%d Can't find OV2640 module!\n", __FILE__, __LINE__);
  else
     printf("%s:%d OV2640 detected.\n", __FILE__, __LINE__);
 

  //Change to JPEG capture mode and initialize the OV2640 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  //myCAM.OV2640_set_JPEG_size(OV2640_320x240);
  myCAM.OV2640_set_JPEG_size(OV2640_1280x1024);
  myCAM.clear_fifo_flag();

  if (wifiType == 0){
    if(!strcmp(ssid,"SSID")){
       printf("%s:%d Please set your SSID\n", __FILE__, __LINE__);
       while(1);
    }
    if(!strcmp(password,"PASSWORD")){
       printf("%s:%d Please set your PASSWORD\n", __FILE__, __LINE__);
       while(1);
    }
    // Connect to WiFi network
    printf("\n\n");
    printf("%s:%d Connecting to %s\n", __FILE__, __LINE__, ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      printf(".");
    }
    printf("%s:%d WiFi connected\n", __FILE__, __LINE__);
    printf("\n");
    printf("%s:%d %s\n", __FILE__, __LINE__, WiFi.localIP().toString().c_str());
  } else if (wifiType == 1){
    printf("\n\n");
    printf("%s:%d Share AP: %s\n", __FILE__, __LINE__, AP_ssid);
    printf("%s:%d The password is: %s\n", __FILE__, __LINE__, AP_password);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_ssid, AP_password);
    printf("%s:%d %s\n", __FILE__, __LINE__, WiFi.softAPIP().toString().c_str());
  }
  
  // Start the server
  server.on("/capture", HTTP_GET, serverCapture);
  server.on("/stream", HTTP_GET, serverStream);
  server.onNotFound(handleNotFound);

  if ( myPIRSensor ) {
    myPIRSensor->Calibrate();
  }
  
  server.begin();
  printf("%s:%d Board is ready. Server started\n", __FILE__, __LINE__);
}

void loop() {
  server.handleClient();
}

