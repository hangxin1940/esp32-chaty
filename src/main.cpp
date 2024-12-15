#include <main.h>

void setup()
{
    // 初始化串口通信，波特率为115200
    Serial.begin(115200);

    // 初始化 UART2
    Serial2.begin(115200, SERIAL_8N1, 19, 20);
    // 配置引脚模式
    // 配置按键引脚为上拉输入模式，用于boot按键检测
    pinMode(key, INPUT_PULLUP);

    // 将led设置为输出模式
    pinMode(led, OUTPUT);
    // 将light设置为输出模式
    pinMode(light, OUTPUT);
    // 将light初始化为低电平
    digitalWrite(light, LOW);

    // 将led设置为输出模式
    pinMode(lcd_led, OUTPUT);
    // 将light设置为输出模式
    pinMode(lcd_led, OUTPUT);
    // 将light初始化为低电平
    digitalWrite(lcd_led, HIGH);
    // 初始化屏幕
    screen.init();
    // 显示文字
    screen.screen_zh_println(TFT_RED, "HELLO CHAT-Y !");
    screen.screen_zh_println();

    // 初始化音频模块mic
    mic.init();
    // 设置音频输出引脚和音量
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(volume);

    ai.base_url = base_url;
    ai.http_timeout = http_timeout;
    ai.openai_apiKey = openai_apiKey;
    ai.system_role = system_role;
    ai.audio = &audio;

    // 初始化Preferences
    preferences.begin("wifi_store");
    preferences.begin("music_store");
    // 连接网络
    screen.screen_zh_println(TFT_WHITE, "正在连接网络······");
    int result = wifiConnect();
    if (result == 1)
    {
        // 清空屏幕，在屏幕上输出提示信息
        screen.fillScreen(TFT_BLACK);
        screen.screen_zh_println(TFT_WHITE, "网络连接成功！");
        screen.screen_zh_println();
        screen.screen_zh_println(TFT_WHITE, "请进行语音唤醒或按boot键开始对话！");
        awake_flag = 0;
    }
    else
    {
        openWeb();
    }
    // 延迟1000毫秒，便于用户查看屏幕显示的信息，同时使设备充分初始化
    delay(1000);
}

void loop()
{
    // 音频处理循环
    audio.loop();

    // 如果音频正在播放
    if (audio.isRunning())
        digitalWrite(led, HIGH); // 点亮板载LED指示灯
    else
        digitalWrite(led, LOW); // 熄灭板载LED指示灯

    // 唤醒词识别
    if (!audio.isRunning() && awake_flag == 0 && await_flag == 1)
    {
        awake_flag = 1;
        startRecording();
    }

    // 检测boot按键是否按下
    if (digitalRead(key) == 0)
    {
        conflag = 0;
        loopcount++;
        Serial.print("loopcount：");
        Serial.println(loopcount);
        startRecording();
    }
    // 连续对话
    if (!audio.isRunning() && conflag == 1 && image_show == 0)
    {
        loopcount++;
        Serial.print("loopcount：");
        Serial.println(loopcount);
        startRecording();
    }
}


// 音量控制
void VolumeSet(String numberStr)
{
    volume = numberStr.toInt();
    audio.setVolume(volume);
    Serial.print("音量已调到: ");
    Serial.println(volume);
    // 在屏幕上显示音量
    // tft.fillRect(66, 152, 62, 7, TFT_WHITE);
    // tft.setCursor(66, 152);
    // tft.print("volume:");
    // tft.print(volume);

    conflag = 1;
}


void startRecording()
{
    // 初始化变量
    int silence = 0;
    int frame_index = 0;
    int voicebegin = 0;
    int voice = 0;
    String audio_id = randomString(12);

    // 创建一个静态JSON文档对象，2000一般够了，不够可以再加（最多不能超过4096），但是可能会发生内存溢出
    StaticJsonDocument<4096> doc;

    if (await_flag == 1)
    {
        screen.fillScreen(TFT_BLACK);
        screen.screen_zh_println(TFT_WHITE, "待机中......");
        screen.screen_zh_println();
        screen.screen_zh_println(TFT_WHITE, "请进行语音唤醒或按boot键开始对话！");
        screen.fillScreen(TFT_BLACK);
        screen.pushImage(0, 0, screen_width, screen_height, image_data_tiger_1);
    }
    else if (conflag == 1)
    {
        screen.fillScreen(TFT_BLACK);
        screen.screen_zh_println(TFT_WHITE, "连续对话中，请说话！");
    }
    else
    {
        screen.screen_zh_println(TFT_WHITE, "请说话！");
    }
    conflag = 0;

    Serial.println("开始录音");
    // 无限循环，用于录制和发送音频数据
    while (1)
    {
        // 待机状态（语音唤醒状态）也可通过boot键启动
        if (digitalRead(key) == 0 && await_flag == 1)
        {
            start_con = 1; // 对话开始标识
            await_flag = 0;
            frame_index = 0;
            break;
        }


        // 录制音频数据
        mic.Record();

        // 计算音频数据的RMS值
        float rms = calculateRMS((uint8_t*)mic.wavData[0], 1280);


        bool valid_rms = rms > noise_low && rms < 20000;

        if (valid_rms)
        {
            voice++;
            silence--;
            if (silence < 0)
            {
                silence = 0;
            }
            if (voice > voice_max)
            {
                voice = 5;
                if (silence == 0)
                {
                    voicebegin = 1;
                }
            }
        }
        else
        {
            voice--;
            if (voice < 0)
            {
                voice = 0;
            }
            if (voice == 0 && silence == 0)
            {
                voicebegin = 0;
            }

            if (voicebegin == 1)
            {
                silence++;
                if (silence > silence_max)
                {
                    silence = silence_max;
                    voice = 0;
                }
            }
        }

        if (voicebegin == 1)
        {
            int is_finish = 0;
            if (silence == silence_max)
            {
                is_finish = 1;
                voicebegin = 0;
                silence = 0;
            }

            String stt_text;
            int result = ai.audioTranscriptions(frame_index, audio_id, is_finish, (byte*)mic.wavData[0], 1280,
                                                stt_text);
            if (result == 0)
            {
                if (stt_text != nullptr && strcmp(stt_text.c_str(), "") != 0)
                {
                    screen.fillScreen(TFT_BLACK);
                    // tft.fillScreen(TFT_BLACK);
                    // tft.setCursor(0, 0);
                    // // 发送给大模型
                    screen.screen_zh_println(TFT_WHITE, stt_text);
                    String answer;
                    int ccresult = ai.chat_completions(stt_text, answer);
                    if (ccresult == 0)
                    {
                        if (answer != nullptr && strcmp(answer.c_str(), "") != 0)
                        {
                            screen.screen_zh_println(TFT_GREENYELLOW, answer);
                            ai.audio_speech(answer);
                            conflag = 1;
                        }
                    }

                    break;
                }
            }

            printf("rms: %f send frame_index: %d finish: %d\n", rms, frame_index, is_finish);
            frame_index++;
            if (is_finish == 1)
            {
                frame_index = 0;
            }
        }
        else
        {
            // printf("rms: %f voice:%d silence: %d\n", rms,voice, silence);
        }

        delay(40);
    }
}


int wifiConnect()
{
    // 断开当前WiFi连接
    WiFi.disconnect(true);

    preferences.begin("wifi_store");
    int numNetworks = preferences.getInt("numNetworks", 0);
    if (numNetworks == 0)
    {
        // 在屏幕上输出提示信息
        screen.screen_zh_println(TFT_WHITE, "无任何wifi存储信息！");
        screen.screen_zh_println();
        screen.screen_zh_println(TFT_WHITE, "请连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！");
        preferences.end();
        return 0;
    }

    // 获取存储的 WiFi 配置
    for (int i = 0; i < numNetworks; ++i)
    {
        String ssid = preferences.getString(("ssid" + String(i)).c_str(), "");
        String password = preferences.getString(("password" + String(i)).c_str(), "");

        // 尝试连接到存储的 WiFi 网络
        if (ssid.length() > 0 && password.length() > 0)
        {
            Serial.print("Connecting to ");
            Serial.println(ssid);
            Serial.print("password:");
            Serial.println(password);
            // 在屏幕上显示每个网络的连接情况
            screen.screen_zh_println(TFT_WHITE, ssid);

            uint8_t count = 0;
            WiFi.begin(ssid.c_str(), password.c_str());
            // 等待WiFi连接成功
            while (WiFi.status() != WL_CONNECTED)
            {
                // 闪烁板载LED以指示连接状态
                digitalWrite(led, ledstatus);
                ledstatus = !ledstatus;
                count++;

                // 如果尝试连接超过30次，则认为连接失败
                if (count >= 30)
                {
                    Serial.printf("\r\n-- wifi connect fail! --\r\n");
                    // 在屏幕上显示连接失败信息
                    screen.screen_zh_println(TFT_WHITE, "Failed!");
                    break;
                }

                // 等待100毫秒
                vTaskDelay(100);
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                // 向串口输出连接成功信息和IP地址
                Serial.printf("\r\n-- wifi connect success! --\r\n");
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());

                // 输出当前空闲堆内存大小
                Serial.println("Free Heap: " + String(ESP.getFreeHeap()));
                // 在屏幕上显示连接成功信息
                screen.screen_zh_println(TFT_WHITE, "Connected!");
                preferences.end();
                return 1;
            }
        }
    }
    // 清空屏幕
    screen.fillScreen(TFT_BLACK);
    // 在屏幕上输出提示信息
    screen.screen_zh_println(TFT_WHITE, "网络连接失败！请检查");
    screen.screen_zh_println(TFT_WHITE, "网络设备，确认可用后重启设备以建立连接！");
    screen.screen_zh_println(TFT_WHITE, "或者连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！");
    preferences.end();
    return 0;
}

// 计算录音数据的均方根值
float calculateRMS(uint8_t* buffer, int bufferSize)
{
    float sum = 0; // 初始化总和变量
    int16_t sample; // 定义16位整数类型的样本变量

    // 遍历音频数据缓冲区，每次处理两个字节（16位）
    for (int i = 0; i < bufferSize; i += 2)
    {
        // 将两个字节组合成一个16位的样本值
        sample = (buffer[i + 1] << 8) | buffer[i];

        // 将样本值平方后累加到总和中
        sum += sample * sample;
    }

    // 计算平均值（样本总数为bufferSize / 2）
    sum /= (bufferSize / 2);

    // 返回总和的平方根，即RMS值
    return sqrt(sum);
}
