#ifndef _AUDIO_H
#define _AUDIO_H

#include <Arduino.h>
#include "I2S.h"

// 16位，单声道，16000Hz，线性PCM
class Mic
{
    I2S* i2s;
    static const int buffSize = 1280;
    static const int headerSize = 44;
    static const int i2sBufferSize = 5120;
    char i2sBuffer[i2sBufferSize];

public:
    static const int wavDataSize = 30000; // 必须是dividedWavDataSize的倍数。录音时间约为1.9秒。
    static const int dividedWavDataSize = i2sBufferSize / 4;
    char** wavData; // 分段存储。因为在ESP32中无法分配大块连续内存区域。

    Mic();
    ~Mic();
    float RecordVoice();
    char* get_wav_data();
    void clear();
    void init();
    float calculateRMS();
};

#endif // _AUDIO_H
