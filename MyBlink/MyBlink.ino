

 #define MY_PIN 13
 #define MY_DELAY 500


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin MY_PIN as an output.
  pinMode(MY_PIN, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(MY_PIN, HIGH);   // turn MY_PIN to HIGH level
  delay(MY_DELAY);              // wait for MY_DELAY

  digitalWrite(MY_PIN, LOW);    // turn the LED off by making the voltage LOW
  delay(MY_DELAY);              // wait for MY_DELAY
}

