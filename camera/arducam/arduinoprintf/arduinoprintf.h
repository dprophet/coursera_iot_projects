#ifndef _ARDUINO_PRINTF_
#define _ARDUINO_PRINTF_

#include <Arduino.h>

// implement a real vararg printf for debugging across the ardbino serial port. Its dumb they never provided one.
#define printf(...) myserialprintf(__VA_ARGS__)

#ifdef __cplusplus
extern "C"
{
#endif

int serial_put_c(char, FILE*);

void _myserialprintf(const char *, ...);

#ifdef __cplusplus
}
#endif

#endif // _ARDUINO_PRINTF_
