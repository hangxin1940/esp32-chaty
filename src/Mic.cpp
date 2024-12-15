#include "Mic.h"

Mic::Mic()
{
    // 构造函数中初始化成员变量和分配内存
    wavData = nullptr;
    i2s = nullptr;
    i2s = new I2S();
}

Mic::~Mic()
{
    for (int i = 0; i < wavDataSize / dividedWavDataSize; ++i)
        delete[] wavData[i];
    delete[] wavData;
    delete i2s;
}

void Mic::init()
{
  wavData = new char *[1];
  for (int i = 0; i < 1; ++i)
    wavData[i] = new char[buffSize];
}

void Mic::clear()
{
    i2s->clear();
}

float Mic::RecordVoice()
{
    i2s->Read(i2sBuffer, i2sBufferSize);
    for (int i = 0; i < i2sBufferSize / 8; ++i)
    {
        wavData[0][2 * i] = i2sBuffer[8 * i + 2];
        wavData[0][2 * i + 1] = i2sBuffer[8 * i + 3];
    }
    float rms = calculateRMS();

    return rms;
}

char* Mic::get_wav_data()
{
    return wavData[0];
}

float Mic::calculateRMS()
{
    float sum = 0;
    int16_t sample;

    // 每次处理两个字节，16位
    for (int i = 0; i < buffSize; i += 2)
    {
        // 从缓冲区中读取16位样本，注意字节顺序
        sample = (wavData[0][i + 1] << 8) | wavData[0][i];

        // 计算平方和
        sum += sample * sample;
    }

    // 计算平均值
    sum /= (buffSize / 2);

    // 返回RMS值
    return sqrt(sum);
}
