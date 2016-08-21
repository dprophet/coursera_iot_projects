

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
#define DEBUG 1

// implement a real vararg printf for debugging across tge ardbino serial port. Its dumb they never provided one.
#define printf(...) myserialprintf(__VA_ARGS__)

#ifdef DEBUG
 #define myserialprintf(myformat, ...) _myserialprintf(PSTR(myformat), ##__VA_ARGS__)

  extern "C" {
   int serial_put_c(char c, FILE *fp)
   { 
       if(c == '\n')
         Serial.write('\r'); 
 
     Serial.write(c); 
   }
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

#define MY_PIN 13
#define MY_WAIT_TIME 100

class SerialReader {
  public:
    SerialReader() {};

	// Blocks and waits for an int (4 bytes.)
    bool read_int(int &iValue) {
      printf("%s:%d SerialReader::read_int()\n", __FILE__, __LINE__);
      int iAvailable = Serial.available();
      printf("%s:%d SerialReader::read_int iAvailable=%d\n", __FILE__, __LINE__, iAvailable);
      if ( iAvailable > 0 ) {
        char cValue[2];
        Serial.readBytes(cValue, 1);
        cValue[1] = '\0';        
        int iRead = Serial.read();
        printf("%s:%d SerialReader::read_int read=%s\n", __FILE__, __LINE__, cValue);
        iValue = atoi(cValue);
        return true;
      } else {
        return false;
      }
    }

  private:
    int m_iPin;
};

class PinToggler {
  public:
    PinToggler(const int &iInPinNumber) : m_iPin(iInPinNumber), m_iCurrentState(-1) {
        pinMode(m_iPin, OUTPUT);
    };

	// Blocks and waits for an int (4 bytes.)
    void setHigh() {
	    digitalWrite(m_iPin, HIGH);  // Turn the pin HIGH
	    m_iCurrentState = 1;
    }

    void setLow() {
	    digitalWrite(m_iPin, LOW);  // Turn the pin LOW
	    m_iCurrentState = 0;
    }
	
	void setState(int iInState) {
     printf("%s:%d setState() iInState=%d, current=%d\n", __FILE__, __LINE__, iInState, m_iCurrentState);
	   if ( !(iInState == 0 || iInState == 1) ) {
	     printf("%s:%d ERROR: invalid pin state=%d. Ignoring\n", __FILE__, __LINE__, iInState);
		   return;
	   }
	   
	   if ( m_iCurrentState == iInState ) {
	     printf("%s:%d setState same pin state. Ignoring\n", __FILE__, __LINE__);
	     return;
	   }
	   
	   if ( iInState ) {
	     digitalWrite(m_iPin, HIGH); 
	   } else {
	     digitalWrite(m_iPin, LOW); 
	   }
	   
	   m_iCurrentState = iInState;
	}

  private:
    int m_iPin;
	  int m_iCurrentState;
};

SerialReader* g_pSerialReader;
PinToggler* g_pPinToggler;

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
   Serial.setTimeout(999999999999);
#endif

  printf("%s:%d setup begining\n", __FILE__, __LINE__);
  g_pSerialReader = new SerialReader();
  g_pPinToggler = new PinToggler(MY_PIN);
  printf("%s:%d setup end\n", __FILE__, __LINE__);
}

void loop() {
  int iValue = 0;
  if ( g_pSerialReader->read_int(iValue) ) {
    g_pPinToggler->setState(iValue);
  }
  delay(MY_WAIT_TIME); 
}

