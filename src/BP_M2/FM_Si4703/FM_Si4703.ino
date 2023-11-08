#include <Si4703_Breakout.h>
#include <Wire.h>

int resetPin = 18;
int SDIO = 21;
int SCLK = 22;

Si4703_Breakout radio(resetPin, SDIO, SCLK);
int channel;
int volume;
char rdsBuffer[10];

void setup()
{
  Serial.begin(9600);
  while(!Serial);
  Serial.println("\n\nSi4703_Breakout Test Sketch");
  Serial.println("===========================");  
  Serial.println("a b     Favourite stations");
  Serial.println("+ -     Volume (max 15)");
  Serial.println("u d     Seek up / down");
  Serial.println("r       Listen for RDS Data (15 sec timeout)");
  Serial.println("g       get current Channel");
  Serial.println("Send me a command letter.");
  

  radio.powerOn();
  radio.setVolume(0);
}

void loop()
{
  if (Serial.available())
  {
    char ch = Serial.read();
    if (ch == 'u') 
    {
      channel = radio.seekUp();
      displayInfo();
    } 
    else if (ch == 'd') 
    {
      channel = radio.seekDown();
      displayInfo();
    } 
    else if (ch == '+') 
    {
      volume ++;
      if (volume == 16) volume = 15;
      radio.setVolume(volume);
      displayInfo();
    } 
    else if (ch == '-') 
    {
      volume --;
      if (volume < 0) volume = 0;
      radio.setVolume(volume);
      displayInfo();
    } 
    else if (ch == 'a')
    {
      channel = 997; // Rock FM
      radio.setChannel(channel);
      displayInfo();
    }
    else if (ch == 'b')
    {
      channel = 1045; // BBC R4
      radio.setChannel(channel);
      displayInfo();
    }
    else if (ch == 'r')
    {
      if(radio.readRDS(rdsBuffer, 1) == 0) {
          Serial.println("No RDS");
      } else {
          Serial.println("RDS listening");
          radio.readRDS(rdsBuffer, 15000);
          Serial.print("RDS heard:");
          Serial.println(rdsBuffer);
      }
    }
    else if (ch == 'g')
    {
      Serial.print("Current Volume: ");
      Serial.println(radio.getVolume());
      Serial.print("Current Channel: ");
      Serial.println(radio.getChannel());    
      Serial.print("Current RSSI: ");
      Serial.println(radio.getRSSI());      
      Serial.print("Current Firmware: ");
      Serial.println(radio.getFirmware());
      if(radio.getFirmware() == 0 && radio.getRSSI() == 0){
          radio.powerOn();
          radio.setVolume(volume);
          radio.setChannel(channel);
      }   
    }
  }
}

void displayInfo()
{
   Serial.print("Channel:"); Serial.print(channel); 
   Serial.print(" Volume:"); Serial.println(volume); 
}
