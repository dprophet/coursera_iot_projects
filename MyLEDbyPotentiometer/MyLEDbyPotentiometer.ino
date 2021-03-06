/*
 *  Copyright (c) 2016, Erik Anderson  https://www.linkedin.com/in/erikanderson
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
#define DEBUG 1

// implement a real vararg printf for debugging across tge ardbino serial port. Its dumb they never provided one.
#define printf(...) myserialprintf(__VA_ARGS__)

#ifdef DEBUG
 #define myserialprintf(myformat, ...) _myserialprintf(PSTR(myformat), ##__VA_ARGS__)

  int serial_put_c(char c, FILE *fp)
  { 
      if(c == '\n')
          Serial.write('\r'); 
 
      Serial.write(c); 
  }
  
  void _myserialprintf(const char *myformat, ...)
  {
#ifdef DEBUG
  FILE stdif;
  va_list ap;
  
   fdev_setup_stream(&stdif, serial_put_c, NULL, _FDEV_SETUP_WRITE);
  
   va_start(ap, myformat);
   vfprintf_P(&stdif, myformat, ap);
   va_end(ap);
#endif
  }
 
#else
  // Do notning since debug wasnt defined
  #define myserialprintf(fmt, ...)
#endif


const int aPin = A0;
const int ledPin = 9;//the led attach to pin 9
int inputValue = 0;//variable to store the value coming from sensor
int outputValue = 0;//variable to store the output value
/******************************************/
void setup()
{
#ifdef DEBUG
  Serial.begin(9600);
#endif
}
/******************************************/
void loop()
{
  inputValue = analogRead(aPin); //read the potentiometer
  int iCalc = (int) (((double)inputValue / (double)1023.0) * 255.0);
  printf("inputValue=%d, iCalc=%d\n", inputValue, iCalc);
  analogWrite(ledPin, iCalc);//turn the led on depend on the output value
  delay(100);
}
/*******************************************/
