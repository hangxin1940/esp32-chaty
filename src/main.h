//
// Created by hang on 24-12-15.
//

#ifndef MAIN_H
#define MAIN_H

#endif //MAIN_H
#include <Arduino.h>
#include <Wire.h>
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include "HTTPClient.h"
#include "Mic.h"
#include "Audio.h"
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "Web_Scr_set.h"
#include "tiger_1.h"

// 定义引脚
#define key 0 // boot按键引脚

#define led 10 // 板载led引脚
#define lcd_led 9
#define light 3 // 灯光引脚
// 定义音频放大模块的I2S引脚定义
#define I2S_DOUT 40 // DIN引脚
#define I2S_BCLK 42 // BCLK引脚
#define I2S_LRC 2   // LRC引脚

String system_role = "";

const char* openai_apiKey = "";
String base_url = "http://192.168.1.16:12345";

// 定义一些全局变量
bool ledstatus = true; // 控制led闪烁
int http_timeout = 60000;
int noise_low = 200; // 噪声门限值
int volume = 80; // 初始音量大小（最小0，最大100）
int silence_max = 8; //最大静音周期
int voice_max = 2; //录音周期


int cursorY = 0;
// 语音唤醒
int awake_flag = 1;


String Answer = ""; // 存储llm回答，用于语音合成（较短的回答）
String text_temp = ""; // 存储超出当前屏幕的文字，在下一屏幕显示
int loopcount = 0; // 对话次数计数器
int conflag = 0; // 用于连续对话
int await_flag = 1; // 待机标识
int start_con = 0; // 标识是否开启了一轮对话
int image_show = 0;


// 创建音频对象
Mic mic;
Audio audio(false, 3, I2S_NUM_1);
// 参数: 是否使用内部DAC（数模转换器）如果设置为true，将使用ESP32的内部DAC进行音频输出。否则，将使用外部I2S设备。
// 指定启用的音频通道。可以设置为1（只启用左声道）或2（只启用右声道）或3（启用左右声道）
// 指定使用哪个I2S端口。ESP32有两个I2S端口，I2S_NUM_0和I2S_NUM_1。可以根据需要选择不同的I2S端口。

// 函数声明
void displayWrappedText(const String text, int x, int y, int maxWidth);
float calculateRMS(uint8_t* buffer, int bufferSize);
int wifiConnect();
void chat_completions(String content);
void chat_completions_stream(String content);
DynamicJsonDocument generate_chat_json(String content, bool stream);
int audioTranscriptions(int frame_index, String audio_id, int is_finished, uint8_t* audio_payload, size_t size,
                        String& output);
String randomString(int len);
void startRecording();
void tft_print_chat(String role, String content);
