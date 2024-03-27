#include "AudioTask.h"



Audio audio(true, I2S_DAC_CHANNEL_BOTH_EN); // at PINs 25 and 26



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

enum : uint8_t { SET_VOLUME, GET_VOLUME, CONNECTTOHOST, CONNECTTOSD, GET_AUDIOTIME, STOPSONG };

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

    Serial.println("Free Stack Space: ");
    Serial.println(uxTaskGetStackHighWaterMark(NULL)); //This is, the minimum free stack space there has been in bytes since the task started.
    Serial.print("setup() is running on core ");
    Serial.println(xPortGetCoreID());

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
            else if(audioRxTaskMessage.cmd == STOPSONG){
                audioTxTaskMessage.cmd = GET_VOLUME;
                audioTxTaskMessage.ret = audio.stopSong();
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
        20000,                 /* Stack size in bytes */
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

int audioSetVolume(uint8_t vol){  // 0...21
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

uint32_t audioStopSong(){
    audioTxMessage.cmd = STOPSONG;
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
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
}*/
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
