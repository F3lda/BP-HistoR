//**********************************************************************************************************
//*    audioI2S-- I2S audiodecoder for ESP32,                                                              *
//**********************************************************************************************************
//
// first release on 11/2018
// Version 3  , Jul.02/2020
//
//
// THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT.
// FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR
// OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
//



// knihovny pro LCD přes I2C
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// nastavení adresy I2C (0x27 v mém případě),
// a dále počtu znaků a řádků LCD, zde 20x4
LiquidCrystal_I2C lcd(0x27, 16, 2); // SDA -> D21; SCL -> D22; VCC -> VIN (5V)
//https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/





#include "Arduino.h"
#include "WiFiMulti.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"

// Digital I/O used
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18

#define I2S_DOUT      27
#define I2S_BCLK      26
#define I2S_LRC       25

Audio audio;
WiFiMulti wifiMulti;
String ssid =     "DodikovyInternety";
String password = "dodikovachyse84";


void setup() {

    Serial.begin(115200);
    while(!Serial);

    Serial.print("setup() is running on core ");
    Serial.println(xPortGetCoreID());


    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setFrequency(1000000);
    SD.begin(SD_CS);
    
    
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(ssid.c_str(), password.c_str());
    wifiMulti.run();
    if(WiFi.status() != WL_CONNECTED){
        WiFi.disconnect(true);
        wifiMulti.run();
    }
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(12); // 0...21

    //audio.connecttoFS(SD, "/DqnceRemix/50 Cent - Candy Shop 2k23 (KROB Pump Up edit).mp3");
//    audio.connecttohost("http://www.wdr.de/wdrlive/media/einslive.m3u");
//    audio.connecttohost("http://somafm.com/wma128/missioncontrol.asx"); //  asx
//    audio.connecttohost("http://mp3.ffh.de/radioffh/hqlivestream.aac"); //  128k aac
      //audio.connecttohost("http://mp3.ffh.de/radioffh/hqlivestream.mp3"); //  128k mp3
      audio.connecttohost("https://ice5.abradio.cz/hitvysocina128.mp3");
  

  
    Serial.println("LCD display - start");
    
    // inicializace LCD
    lcd.begin();
    // zapnutí podsvícení
    lcd.backlight();
    
    // veškeré číslování je od nuly, poslední znak je tedy 15, 1
    lcd.print("Test LCD - I2C");
    lcd.setCursor ( 0, 1);
    lcd.print("--------------------");
    lcd.setCursor ( 7, 1);
    lcd.print("!");
    delay(2000);
}
int lastSeconds = -1;
void loop()
{
    audio.loop();
    if(Serial.available()){ // put streamURL in serial monitor
        audio.stopSong();
        String r=Serial.readString(); r.trim();
        if(r.length()>5) audio.connecttohost(r.c_str());//audio.connecttoFS(SD, r.c_str());
        log_i("free heap=%i", ESP.getFreeHeap());
    }

    if(lastSeconds != millis() / 1000){
        lcd.setCursor(7, 1);
        // vytisknutí počtu sekund od začátku programu
        lcd.print(millis() / 1000);
        
        Serial.println("LCD display");
        lastSeconds = (int)(millis() / 1000);
    }
    //delay(1000); // do nothing
}

// optional
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info);
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);
}
void audio_showstreamtitle(const char *info){
    Serial.print("streamtitle ");Serial.println(info);
}
void audio_bitrate(const char *info){
    Serial.print("bitrate     ");Serial.println(info);
}
void audio_commercial(const char *info){  //duration in sec
    Serial.print("commercial  ");Serial.println(info);
}
void audio_icyurl(const char *info){  //homepage
    Serial.print("icyurl      ");Serial.println(info);
}
void audio_lasthost(const char *info){  //stream URL played
    Serial.print("lasthost    ");Serial.println(info);
}
