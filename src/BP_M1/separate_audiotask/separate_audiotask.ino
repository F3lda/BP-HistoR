#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"


// knihovny pro LCD přes I2C
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// nastavení adresy I2C (0x27 v mém případě),
// a dále počtu znaků a řádků LCD, zde 20x4
LiquidCrystal_I2C lcd(0x27, 16, 2); // SDA -> D21; SCL -> D22; VCC -> VIN (5V)
//https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/





#include "SPI.h"
#include "SD.h"
#include "FS.h"

// Digital I/O used
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18

// Digital I/O used
#define I2S_DOUT      27
#define I2S_BCLK      26
#define I2S_LRC       25

Audio audio;

String ssid =     "DodikovyInternety";
String password = "dodikovachyse84";

//****************************************************************************************
//                                   A U D I O _ T A S K                                 *
//****************************************************************************************
void audioStartStop(bool audioisrunning) __attribute__((weak));
bool audioIsRunning = false;

struct audioMessage{
    uint8_t     cmd;
    const char* txt;
    uint32_t    value;
    uint32_t    ret;
} audioTxMessage, audioRxMessage;

enum : uint8_t { SET_VOLUME, GET_VOLUME, CONNECTTOHOST, CONNECTTOSD, GET_AUDIOTIME };

QueueHandle_t audioSetQueue = NULL;
QueueHandle_t audioGetQueue = NULL;

void CreateQueues(){
    audioSetQueue = xQueueCreate(10, sizeof(struct audioMessage));
    audioGetQueue = xQueueCreate(10, sizeof(struct audioMessage));
}

void audioTask(void *parameter) {
    CreateQueues();
    if(!audioSetQueue || !audioGetQueue){
        log_e("queues are not initialized");
        while(true){;}  // endless loop
    }

    struct audioMessage audioRxTaskMessage;
    struct audioMessage audioTxTaskMessage;

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(15); // 0...21

    Serial.println("Free Stack Space: ");
    Serial.println(uxTaskGetStackHighWaterMark(NULL)); //This is, the minimum free stack space there has been in bytes since the task started.

    while(true){
        if(xQueueReceive(audioSetQueue, &audioRxTaskMessage, 1) == pdPASS) {
            if(audioRxTaskMessage.cmd == SET_VOLUME){
                audioTxTaskMessage.cmd = SET_VOLUME;
                audio.setVolume(audioRxTaskMessage.value);
                audioTxTaskMessage.ret = 1;
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == CONNECTTOHOST){
                audioTxTaskMessage.cmd = CONNECTTOHOST;
                audioTxTaskMessage.ret = audio.connecttohost(audioRxTaskMessage.txt);
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == CONNECTTOSD){
                audioTxTaskMessage.cmd = CONNECTTOSD;
                audioTxTaskMessage.ret = audio.connecttoSD(audioRxTaskMessage.txt);
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == GET_VOLUME){
                audioTxTaskMessage.cmd = GET_VOLUME;
                audioTxTaskMessage.ret = audio.getVolume();
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == GET_AUDIOTIME){
                audioTxTaskMessage.cmd = GET_AUDIOTIME;
                audioTxTaskMessage.ret = audio.getAudioCurrentTime();
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else{
                log_i("error");
            }
        }
        audio.loop();
        if (!audio.isRunning()) {
            sleep(1);
            if (audioIsRunning == true) {
                audioIsRunning = false;
                if(audioStartStop)audioStartStop(false);
            }
        } else {
            if (audioIsRunning == false) {
                audioIsRunning = true;
                if(audioStartStop)audioStartStop(true);
            }
        }
            //Serial.println("Free Stack Space: ");
            //Serial.println(uxTaskGetStackHighWaterMark(NULL)); //This is, the minimum free stack space there has been in bytes since the task started.
    }
}

void audioInit() {
    xTaskCreatePinnedToCore(
        audioTask,             /* Function to implement the task */
        "audioplay",           /* Name of the task */
        10000,                 /* Stack size in bytes */
        NULL,                  /* Task input parameter */
        2 | portPRIVILEGE_BIT, /* Priority of the task */
        NULL,                  /* Task handle. */
        0                      /* Core where the task should run */
    );
}

audioMessage transmitReceive(audioMessage msg){
    xQueueSend(audioSetQueue, &msg, portMAX_DELAY);
    if(xQueueReceive(audioGetQueue, &audioRxMessage, portMAX_DELAY) == pdPASS){
        if(msg.cmd != audioRxMessage.cmd){
            log_e("wrong reply from message queue");
        }
    }
    return audioRxMessage;
}

int audioSetVolume(uint8_t vol){
    audioTxMessage.cmd = SET_VOLUME;
    audioTxMessage.value = vol;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

uint8_t audioGetVolume(){
    audioTxMessage.cmd = GET_VOLUME;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

uint32_t audioGetCurrentTime(){
    audioTxMessage.cmd = GET_AUDIOTIME;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

bool audioConnecttohost(const char* host){
    audioTxMessage.cmd = CONNECTTOHOST;
    audioTxMessage.txt = host;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

bool audioConnecttoSD(const char* filename){
    audioTxMessage.cmd = CONNECTTOSD;
    audioTxMessage.txt = filename;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}



bool I2CaddressIsActive(byte address) {
    Wire.begin();
    Wire.beginTransmission(address);
    return !Wire.endTransmission();
}

//****************************************************************************************
//                                   S E T U P                                           *
//****************************************************************************************

void setup() {
    Serial.begin(9600);
    while(!Serial);
    Serial.print("setup() is running on core ");
    Serial.println(xPortGetCoreID());
 
	  Serial.println(String(ESP.getHeapSize() / 1024) + " Kb");
 
    WiFi.begin(ssid.c_str(), password.c_str());
    while (WiFi.status() != WL_CONNECTED) delay(1500);
	
	  pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setFrequency(1000000);
    SD.begin(SD_CS);

    audioInit();
    delay(1500);

    audioConnecttohost("https://ice5.abradio.cz/hitvysocina128.mp3");
    //audioConnecttoSD("/DqnceRemix/50 Cent - Candy Shop 2k23 (KROB Pump Up edit).mp3");
    audioSetVolume(15);
	  Serial.print("current volume is: ");
	  Serial.println(audioGetVolume());
	
	
	
	  Serial.println("LCD display - start");

    if(I2CaddressIsActive(0x27)) {
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
}

//****************************************************************************************
//                                   L O O P                                             *
//****************************************************************************************

void loop(){
    // your own code here
    Serial.print("loop() is running on core ");
    Serial.println(xPortGetCoreID());
	
	if(Serial.available()){ // put streamURL in serial monitor
        audio.stopSong();
        String r=Serial.readString(); r.trim();
        if(r.length()>5) audioConnecttoSD(r.c_str());//audio.connecttohost(r.c_str());
        log_i("free heap=%i", ESP.getFreeHeap());
    }

    if(audioIsRunning){
        Serial.print("current audio time is: ");
        Serial.println(audioGetCurrentTime());
    }
    Serial.print("current audio time is: ");
    Serial.println(audio.getAudioCurrentTime());


    //if(lastSeconds != millis() / 1000){
        //---lcd.setCursor(7, 1);
        // vytisknutí počtu sekund od začátku programu
        //---lcd.print(millis() / 1000);
        
        Serial.println("LCD display");
        //lastSeconds = (int)(millis() / 1000);
    //}
    delay(1000); // do nothing
}
//*****************************************************************************************
//                                  E V E N T S                                           *
//*****************************************************************************************

void audioStartStop(bool audioisrunning){  //stream URL played
    Serial.print("audioisrunning    ");Serial.println(audioisrunning);
    Serial.print("audioStartStop() is running on core ");
    Serial.println(xPortGetCoreID());
}
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
