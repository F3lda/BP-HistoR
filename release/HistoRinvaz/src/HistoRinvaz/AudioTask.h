#ifndef _AudioTask_H_
#define _AudioTask_H_


#include <Arduino.h>
#include <Audio.h>


// Digital I/O used for Audio output
#define I2S_DOUT      27 // DIN (DATA)
#define I2S_BCLK      26 // BCK (BIT CLOCK - SCK)
#define I2S_LRC       25 // LCK (LEFT-RIGHT CLOCK - WORD CLOCK)


void audioInit();

bool audioIsRunning();
bool audioPauseResume();
bool audioSetFilePos(uint32_t pos);
uint32_t audioGetCurrentTime();
uint8_t audioSetVolume(uint8_t vol);
uint8_t audioGetVolume();
bool audioConnecttohost(const char* host);
bool audioConnecttoSD(const char* filename);
bool audioConnecttoSPIFFS(const char* filename);
uint32_t audioStopSong();

bool audioHistorStopStationPrepareBeep(const char* filename);
bool audioHistorChangeStation(const char* host, uint8_t vol);


#endif
