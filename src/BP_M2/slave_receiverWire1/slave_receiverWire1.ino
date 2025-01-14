// Wire Peripheral Receiver

// by Nicholas Zambetti <http://www.zambetti.com>


// Demonstrates use of the Wire library

// Receives data as an I2C/TWI Peripheral device

// Refer to the "Wire Master Writer" example for use with this


// Created 29 March 2006


// This example code is in the public domain.



#include <Wire.h>


void setup()

{
  Wire1.setPins(16, 17);
  Wire1.begin(8);                // join i2c bus with address #4

  Wire1.onReceive(receiveEvent); // register event
 Wire1.onRequest(requestEvent); // register event
  Serial.begin(9600);           // start serial for output

}


void loop()

{

  delay(100);

}
void requestEvent() {

  Wire1.write("ABC1234D"); // respond with message of 6 bytes

  // as expected by master

}

// function that executes whenever data is received from master

// this function is registered as an event, see setup()

void receiveEvent(int howMany)

{

  while(1 < Wire1.available()) // loop through all but the last

  {

    char c = Wire1.read(); // receive byte as a character

    Serial.print(c);         // print the character

  }

  int x = Wire1.read();    // receive byte as an integer

  Serial.println(x);         // print the integer

}
