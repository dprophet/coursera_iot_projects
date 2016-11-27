#include "arduinoprintf.h"

// Comment and uncomment this if you want to stop debugging output
#define _DEBUG_

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

  fdev_setup_stream(&stdif, serial_put_c, NULL, _FDEV_SETUP_WRITE);

  va_start(ap, myformat);
  vfprintf_P(&stdif, myformat, ap);
  va_end(ap);
}

#ifdef _DEBUG_
  #define myserialprintf(myformat, ...) _myserialprintf(PSTR(myformat), ##__VA_ARGS__)
#else
  //NOOP. Do nothing since debug wasnt defined
  #define myserialprintf(fmt, ...)
#endif // _DEBUG_

