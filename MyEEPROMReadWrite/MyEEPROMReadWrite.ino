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

#include <EEPROM.h>

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

class EEPromCommand {
  public:
     EEPromCommand() : command(-1), address(-1), write_value(0) {}
     EEPromCommand(int _command, int _address, byte _write_value) : command(_command), 
         address(_address), write_value(_write_value) {}
     EEPromCommand(const EEPromCommand &_in) : command(_in.command), 
         address(_in.address), write_value(_in.write_value) {}
     EEPromCommand & operator = (const EEPromCommand &_in) {
       if(&_in == this)
         return *this;
       command = _in.command;
       address = _in.address;
       write_value = _in.write_value;
     }

     void print() {
      printf("EEPromCommand: cmd=%d, add=%d, val=%d\n", command, address, write_value);
     }

     bool validate() {
       if ( (command == 1 || command == 2) && (address >= 0 && address <= 1023) ) {
         return true;
       } else if (command == 0) {
         usage();
       }
       return false;
     }

     void usage() {
       printf("EEPromCommand usage:\n");
       printf("  write address value\n");
       printf("    Example: write 50 2\n");
       printf("  or\n");
       printf("  read address\n");
       printf("    Example: read 50\n\n\n");
     }

     void clear() {
       command = 0;
       address = -1;
       write_value = 0;
     }


     int command;      // 0 for none, 1 for write, 2 for read. -1 means nothing entered.
     int address;      // EEPROM address to read or write to
     byte write_value; // value to write to the eeprom
};

class SerialReader {
  public:
    SerialReader() {};

  // Blocks and waits for an int (4 bytes.)
    EEPromCommand readCommand() {
      memset(cIncomingString,0,sizeof(cIncomingString));
      EEPromCommand oCmd;
      int iLoc = 0;
      char *cPt;
      char cValue[2] = {0};
      int iAvailable = Serial.available();

      if ( iAvailable > 0 ) {
        oCmd.clear();
        printf("%s:%d SerialReader::readCommand iAvailable=%d\n", __FILE__, __LINE__, iAvailable);
      }

      while ( iAvailable > 0 ) {
        cValue[1] = '\0';
        Serial.readBytes(cValue, 1);
        cValue[1] = '\0';
        cIncomingString[iLoc++] = cValue[0];
        iAvailable = Serial.available();
      }

      iLoc = 0;
      if ( strlen(cIncomingString) > 0 ) {
        printf("%s:%d SerialReader::readCommand cIncomingString=%s\n", __FILE__, __LINE__, cIncomingString);
        cPt = strtok (cIncomingString," ");
        while (cPt != NULL)
        {
          if ( iLoc == 0 ) {
            if ( strcmp(cPt,"write") == 0 ) {
              oCmd.command = 1;
            } else if ( strcmp(cPt,"read") == 0 ) {
              oCmd.command = 2;
            } else {
              printf("Invalid command = %s. No operation will be performed\n", cPt);
            }
          } else if ( iLoc == 1 ) {
            oCmd.address = atoi(cPt);
          } else if ( iLoc == 2 ) {
            oCmd.write_value = (byte)atoi(cPt);
          }
          cPt = strtok (NULL, " ");
          ++iLoc;
        }
      }

      return oCmd;  
    }

  private:
    char cIncomingString[256];
};

byte EEPROMWasSet[1024];

SerialReader* g_pSerialReader;

void setup() {
   Serial.begin(9600);
   Serial.setTimeout(999999999999);

   g_pSerialReader = new SerialReader();
   memset(EEPROMWasSet, 0, sizeof(EEPROMWasSet) );
}

void loop() {
  // put your main code here, to run repeatedly:
  EEPromCommand oCmd(g_pSerialReader->readCommand());

  if (  oCmd.validate() ) {
    // Debugging helper
    oCmd.print();

    if ( oCmd.command == 1 ) {
      EEPROM.write(oCmd.address, oCmd.write_value);
      EEPROMWasSet[oCmd.address] = 1;
    } else if ( oCmd.command == 2 ) {
      if ( EEPROMWasSet[oCmd.address] == 1 ) {
        byte byteValue = EEPROM.read(oCmd.address);
        printf("EEPROM address %d contained value %d\n", oCmd.address, byteValue);
      } else {
        printf("Address %d is uninitialized. You must write before you read.\n", oCmd.address);
      }
    }
  }

  delay(100);
}

