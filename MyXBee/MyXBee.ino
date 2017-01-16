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

 // Seeedstudio XBee Shield V2.0 with umpers between #2 XB_TX-DIGITAL and #3 XB_RX-DIGITAL

#include <SoftwareSerial.h>

SoftwareSerial XBeeSerial(2, 3); // RX, TX

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  XBeeSerial.begin(9600);
  Serial.println("setup()");
}

void loop() {
  int iDebugSerial = 0;
  int iXBeeSerial = 0;
  char cByte;
  String sOut;
  
  iDebugSerial = Serial.available();
  iXBeeSerial  = XBeeSerial.available();

  if ( iDebugSerial || iXBeeSerial ) {
    delay(50);
    iDebugSerial = Serial.available();
    iXBeeSerial  = XBeeSerial.available();
    sOut += "iDebugSerial=";
    sOut += iDebugSerial;
    sOut += ", iXBeeSerial=";
    sOut += iXBeeSerial;
    Serial.println(sOut);
    sOut = "";    
  }
  
  // put your main code here, to run repeatedly:
  if ( iDebugSerial > 0 ) {
    delay(50);
    while (Serial.available()) {
      cByte = Serial.read();
      sOut += cByte;
    }    
  }

  if ( sOut.length() > 0 ) {
    Serial.print("Sending to XBee:");
    Serial.println(sOut);
    XBeeSerial.println(sOut);
  }

  sOut = "";

  if ( iXBeeSerial > 0 ) {

    delay(50);
    while (XBeeSerial.available()) {
      cByte = XBeeSerial.read();
      sOut += cByte;
    }    
  }

  if ( sOut.length() > 0 ) {
    Serial.println(sOut);
  }

}
