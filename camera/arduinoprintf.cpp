#include "arduinoprintf.h"

#include <Arduino.h>

#ifndef _USE_ETS_PRINTF_

bool bFirstTry = true;

int serial_put_c(char c, FILE *fp)
{ 
	if(c == '\n')
		Serial.write('\r'); 
	Serial.write(c); 
}

void _myserialprintf(const char *myformat, ...)
{
  FILE stdif;
  va_list ap;
  
  if ( bFirstTry) {
	  delay(5000);
	  bFirstTry = false;
  }

  fdev_setup_stream(&stdif, serial_put_c, NULL, _FDEV_SETUP_WRITE);

  va_start(ap, myformat);
  vfprintf_P(&stdif, myformat, ap);
  va_end(ap);
}

#endif
