#include "AudioTask.h"


Audio audio(true, I2S_DAC_CHANNEL_BOTH_EN); // at PINs 25 and 26



//****************************************************************************************
//                                   A U D I O _ T A S K                                 *
//****************************************************************************************
void audioStartStop(bool audioisrunning) __attribute__((weak));
bool audioRunning = false;

struct audioMessage{
    uint8_t     cmd;
    const char* txt;
    uint32_t    value;
    uint32_t    ret;
} audioTxMessage, audioRxMessage;

enum : uint8_t { IS_RUNNING, PAUSE_RESUME, SET_FILE_POS, GET_CURRENT_TIME, SET_VOLUME, GET_VOLUME, CONNECT_TO_HOST, CONNECT_TO_SD, CONNECT_TO_SPIFFS, STOP_SONG, HISTOR_STOP_STATION_PREPARE_BEEP, HISTOR_CHANGE_STATION };

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

    //audio.setVolume(0); // 0...21
    audio.setConnectionTimeout(3000, 10000);

    Serial.println("Free Stack Space: ");
    Serial.println(uxTaskGetStackHighWaterMark(NULL)); //This is, the minimum free stack space there has been in bytes since the task started.
    Serial.print("setup() is running on core ");
    Serial.println(xPortGetCoreID());

    bool beep_played = false;

    while(true){
        if(xQueueReceive(audioSetQueue, &audioRxTaskMessage, 1) == pdPASS) {
            if(audioRxTaskMessage.cmd == IS_RUNNING){
                audioTxTaskMessage.cmd = IS_RUNNING;
                audioTxTaskMessage.ret = audio.isRunning();
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == PAUSE_RESUME){
                audioTxTaskMessage.cmd = PAUSE_RESUME;
                audioTxTaskMessage.ret = audio.pauseResume();
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == SET_FILE_POS){
                audioTxTaskMessage.cmd = SET_FILE_POS;
                audioTxTaskMessage.ret = audio.setFilePos(audioRxTaskMessage.value);
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == GET_CURRENT_TIME){
                audioTxTaskMessage.cmd = GET_CURRENT_TIME;
                audioTxTaskMessage.ret = audio.getAudioCurrentTime();
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == SET_VOLUME){
                audioTxTaskMessage.cmd = SET_VOLUME;
                audio.setVolume(audioRxTaskMessage.value);
                audioTxTaskMessage.ret = audioRxTaskMessage.value;
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == GET_VOLUME){
                audioTxTaskMessage.cmd = GET_VOLUME;
                audioTxTaskMessage.ret = audio.getVolume();
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == CONNECT_TO_HOST){
                audioTxTaskMessage.cmd = CONNECT_TO_HOST;
                audioTxTaskMessage.ret = audio.connecttohost(audioRxTaskMessage.txt);
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == CONNECT_TO_SD){
                audioTxTaskMessage.cmd = CONNECT_TO_SD;
                audioTxTaskMessage.ret = audio.connecttoFS(SD, audioRxTaskMessage.txt);
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == CONNECT_TO_SPIFFS){
                audioTxTaskMessage.cmd = CONNECT_TO_SPIFFS;
                audioTxTaskMessage.ret = audio.connecttoFS(SPIFFS, audioRxTaskMessage.txt);
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == STOP_SONG){
                audioTxTaskMessage.cmd = GET_VOLUME;
                audioTxTaskMessage.ret = audio.stopSong();
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == HISTOR_STOP_STATION_PREPARE_BEEP){
                audioTxTaskMessage.cmd = HISTOR_STOP_STATION_PREPARE_BEEP;
                audio.stopSong();
                audioTxTaskMessage.ret = audio.connecttoFS(SPIFFS, audioRxTaskMessage.txt);
                audio.pauseResume();
                audio.setFilePos(0);
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            }
            else if(audioRxTaskMessage.cmd == HISTOR_CHANGE_STATION){
                audioTxTaskMessage.cmd = HISTOR_CHANGE_STATION;
                audio.setVolume(21);
                beep_played = true;
                audioTxTaskMessage.ret = audio.pauseResume();
                Serial.print("IS RUNNING: ");
                Serial.println(audio.isRunning());
            }
            else{
                log_i("error");
            }
        }
        audio.loop();
        if (!audio.isRunning()) {
            if (beep_played) {
                beep_played = false;

                audio.stopSong();
                audio.setVolume(audioRxTaskMessage.value);
                audio.connecttohost(audioRxTaskMessage.txt);
                audio.loop();
                xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
            } else {
                delay(1000/30);
            }
            
            if (audioRunning == true) {
                audioRunning = false;
                if(audioStartStop) {audioStartStop(false);}
            }
        } else {
            if (audioRunning == false) {
                audioRunning = true;
                if(audioStartStop) {audioStartStop(true);}
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
        15000,                 /* Stack size in bytes */
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



bool audioIsRunning(){
    audioTxMessage.cmd = IS_RUNNING;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

bool audioPauseResume(){
    audioTxMessage.cmd = PAUSE_RESUME;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

bool audioSetFilePos(uint32_t pos){
    audioTxMessage.cmd = SET_FILE_POS;
    audioTxMessage.value = pos;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

uint32_t audioGetCurrentTime(){
    audioTxMessage.cmd = GET_CURRENT_TIME;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

uint8_t audioSetVolume(uint8_t vol){  // 0...21
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

bool audioConnecttohost(const char* host){
    audioTxMessage.cmd = CONNECT_TO_HOST;
    audioTxMessage.txt = host;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

bool audioConnecttoSD(const char* filename){
    audioTxMessage.cmd = CONNECT_TO_SD;
    audioTxMessage.txt = filename;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

bool audioConnecttoSPIFFS(const char* filename){
    audioTxMessage.cmd = CONNECT_TO_SPIFFS;
    audioTxMessage.txt = filename;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

uint32_t audioStopSong(){
    audioTxMessage.cmd = STOP_SONG;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}





bool audioHistorStopStationPrepareBeep(const char* filename){
    audioTxMessage.cmd = HISTOR_STOP_STATION_PREPARE_BEEP;
    audioTxMessage.txt = filename;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}

bool audioHistorChangeStation(const char* host, uint8_t vol){  // 0...21
    audioTxMessage.cmd = HISTOR_CHANGE_STATION;
    audioTxMessage.txt = host;
    audioTxMessage.value = vol;
    audioMessage RX = transmitReceive(audioTxMessage);
    return RX.ret;
}



//*****************************************************************************************
//                                  E V E N T S                                           *
//*****************************************************************************************

/*void audioStartStop(bool audioisrunning){
    Serial.print("audioisrunning    ");Serial.println(audioisrunning);
    Serial.print("audioStartStop() is running on core ");
    Serial.println(xPortGetCoreID());
}*/
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
/*void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info); // audio file Artist: ; Title: ;
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);// radio station
}
void audio_showstreamtitle(const char *info){
    Serial.print("streamtitle ");Serial.println(info);// radio info
}*/
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
    Serial.print("eof_mp3() is running on core ");
    Serial.println(xPortGetCoreID());
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
void audio_icydescription(const char *info){
    Serial.print("audio_icydescription ");Serial.println(info);
}
void audio_eof_stream(const char *info){
    Serial.print("audio_eof_stream ");Serial.println(info);
}
