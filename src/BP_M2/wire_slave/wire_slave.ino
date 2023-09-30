#include <Wire.h>

#define BP_ESP_SLAVE_ID 8

void setup() {
    Serial.begin(9600);
    while(!Serial);
    
    Wire.begin(BP_ESP_SLAVE_ID);                // join i2c bus with address #8
    Wire.onRequest(requestEvent); // register event
    Wire.onReceive(receiveEvent); // register event

    Serial.println("ESP slave ON!");
}


void loop() {
    delay(100);
}


// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
    Wire.write("hello "); // respond with message of 6 bytes
    // as expected by master
}

void receiveEvent(int howMany) {
  while(1 < Wire.available()) // loop through all but the last
  {
    char c = Wire.read(); // receive byte as a character
    Serial.print(c);         // print the character
  }
  int x = Wire.read();    // receive byte as an integer
  Serial.println(x);         // print the integer
}
