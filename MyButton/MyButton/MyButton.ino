/*
  Button

 Turns on and off a light emitting diode(LED) connected to digital
 pin 13, when pressing a pushbutton attached to pin 2.


 The circuit:
 * LED attached from pin 13 to ground
 * pushbutton attached to pin 2 from +5V
 * 10K resistor attached to pin 2 from ground

 * Note: on most Arduinos there is already an LED on the board
 attached to pin 13.


 created 2005
 by DojoDave <http://www.0j0.org>
 modified 30 Aug 2011
 by Tom Igoe

 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/Button
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


// constants won't change. They're used here to
// set pin numbers:
const int buttonPin = 2;     // the number of the pushbutton pin
const int ledPin =  13;      // the number of the LED pin

// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status
int lastbuttonState = 0;         // variable for reading the pushbutton status

void setup() {
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);

 #ifdef DEBUG
  Serial.begin(9600);
  Serial.setTimeout(999999999999);
#endif
}

void loop() {
  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  if ( lastbuttonState != buttonState ) {
    printf("%s:%d: Pin(%d) changed state to %d\n", __FILE__, __LINE__, buttonPin, buttonState);
    lastbuttonState = buttonState;
  }

  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    // turn LED on:
    digitalWrite(ledPin, HIGH);
  } else {
    // turn LED off:
    digitalWrite(ledPin, LOW);
  }
}
