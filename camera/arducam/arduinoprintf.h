#ifndef _ARDUINO_PRINTF_
#define _ARDUINO_PRINTF_

#include <Arduino.h>

#ifndef fdev_setup_stream
#include "ets_sys.h"
#define _USE_ETS_PRINTF_  // ets_printf is defined in some arduino IDE but not others.
#endif

#define _DEBUG_

// implement a real vararg printf for debugging across the ardbino serial port. Its dumb they never provided one.
#define printf(...) myserialprintf(__VA_ARGS__)

#ifdef _DEBUG_
#ifdef _USE_ETS_PRINTF_
  #define myserialprintf(...) ets_printf(__VA_ARGS__)
#else
  #define myserialprintf(myformat, ...) _myserialprintf(PSTR(myformat), ##__VA_ARGS__)
#endif
#else
  //NOOP. Do nothing since debug wasnt defined
  #define myserialprintf(fmt, ...)
#endif // _DEBUG_

void _myserialprintf(const char *, ...);


#endif // _ARDUINO_PRINTF_
