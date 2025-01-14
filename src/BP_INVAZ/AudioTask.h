#ifndef _AudioTask_H_
#define _AudioTask_H_


#include <Arduino.h>
#include <Audio.h>


// Digital I/O used for Audio output
#define I2S_DOUT      27 // DIN (DATA)
#define I2S_BCLK      26 // BCK (BIT CLOCK - SCK)
#define I2S_LRC       25 // LCK (LEFT-RIGHT CLOCK - WORD CLOCK)


void audioInit();

int audioSetVolume(uint8_t vol);
uint8_t audioGetVolume();
uint32_t audioGetCurrentTime();
uint32_t audioStopSong();
bool audioConnecttohost(const char* host);
bool audioConnecttoSD(const char* filename);


#endif
