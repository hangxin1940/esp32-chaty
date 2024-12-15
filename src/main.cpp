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
    screen.screen_zh_println(TFT_RED,"HELLO CHAT-Y !");
    screen.screen_zh_println();

    // 初始化音频模块mic
    mic.init();
    // 设置音频输出引脚和音量
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(volume);

    // 初始化Preferences
    preferences.begin("wifi_store");
    preferences.begin("music_store");
    // 连接网络
    screen.screen_zh_println(TFT_WHITE,"正在连接网络······");
    int result = wifiConnect();
    if (result == 1)
    {
        // 清空屏幕，在屏幕上输出提示信息
        screen.fillScreen(TFT_BLACK);
        screen.screen_zh_println(TFT_WHITE,"网络连接成功！");
        screen.screen_zh_println();
        screen.screen_zh_println(TFT_WHITE,"请进行语音唤醒或按boot键开始对话！");
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
    if (!audio.isRunning() && Answer == "" && conflag == 1 && image_show == 0)
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

void speakAnswer()
{
    if (Answer != "")
    {
        // tft.fillScreen(TFT_BLACK);
        // tft.setCursor(0, 0);
        // tft.print("assistant: ");

        String url = base_url + "/v1/audio/speech?file=1&input=" + Answer;
        audio.connecttohost(url.c_str());
        screen.screen_zh_println(TFT_WHITE, Answer);
        Answer = "";
        delay(500);
    }
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
    StaticJsonDocument<2000> doc;

    if (await_flag == 1)
    {
        screen.fillScreen(TFT_BLACK);
        screen.screen_zh_println(TFT_WHITE,"待机中......");
        screen.screen_zh_println();
        screen.screen_zh_println(TFT_WHITE,"请进行语音唤醒或按boot键开始对话！");
        screen.fillScreen(TFT_BLACK);
        // tft.pushImage(0, 0, width, height, image_data_client_pic);
        // tft.pushImage(0, 0, width, height, image_data_tiger_1);
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

            String jsonString;
            int result = audioTranscriptions(frame_index, audio_id, is_finish, (byte*)mic.wavData[0], 1280, jsonString);
            if (result == 0)
            {
                Serial.println("Transcription successful:");
                if (is_finish == 1)
                {
                    // 解析收到的JSON数据
                    DeserializationError error = deserializeJson(doc, jsonString);

                    // 如果解析没有错误
                    if (!error)
                    {
                        const char* stt_text = doc["text"];

                        if (stt_text != nullptr && strcmp(stt_text, "") != 0)
                        {
                            screen.fillScreen(TFT_BLACK);
                            // tft.fillScreen(TFT_BLACK);
                            // tft.setCursor(0, 0);
                            // // 发送给大模型
                            screen.screen_zh_println(TFT_WHITE, stt_text);
                            chat_completions(stt_text);
                            speakAnswer();
                            screen.screen_zh_println(TFT_GREENYELLOW, Answer);
                            break;
                        }
                    }
                }
            }
            else
            {
                Serial.println("Transcription failed.");
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
        screen.screen_zh_println(TFT_WHITE,"无任何wifi存储信息！");
        screen.screen_zh_println();
        screen.screen_zh_println(TFT_WHITE,"请连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！");
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
            screen.screen_zh_println(TFT_WHITE,ssid);

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
                    screen.screen_zh_println(TFT_WHITE,"Failed!");
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
                screen.screen_zh_println(TFT_WHITE,"Connected!");
                preferences.end();
                return 1;
            }
        }
    }
    // 清空屏幕
    screen.fillScreen(TFT_BLACK);
    // 在屏幕上输出提示信息
    screen.screen_zh_println(TFT_WHITE,"网络连接失败！请检查");
    screen.screen_zh_println(TFT_WHITE,"网络设备，确认可用后重启设备以建立连接！");
    screen.screen_zh_println(TFT_WHITE,"或者连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！");
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

// 问题发送给Chatgpt并接受回答，然后转成语音
void chat_completions(String content)
{
    HTTPClient http;
    http.setTimeout(http_timeout); // 设置请求超时时间
    http.begin(base_url + "/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    String token_key = String("Bearer ") + openai_apiKey;
    http.addHeader("Authorization", token_key);

    // 向串口输出提示信息
    Serial.println("Send message to llm!");

    // 生成连接参数的JSON文档
    DynamicJsonDocument jsonData = generate_chat_json(content, false);

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsonData, jsonString);
    jsonData.clear();

    // 向串口输出生成的JSON字符串
    Serial.println("POST: " + base_url + "/v1/chat/completions");
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode == 200)
    {
        String output = http.getString();
        http.end();
        StaticJsonDocument<4096> jsonResponse;
        // 解析收到的数据
        DeserializationError error = deserializeJson(jsonResponse, output);
        if (!error)
        {
            const char* content = jsonResponse["choices"][0]["message"]["content"];
            jsonResponse.clear();
            // 将内容追加到Answer字符串中
            Answer = content;
            content = "";
        }
        else
        {
            Answer = "";
        }
    }
    else
    {
        Serial.printf("Error %i \n", httpResponseCode);
        Serial.println(http.getString());
        http.end();
    }

    Serial.println("Answer: " + Answer);
}


// 问题发送给Chatgpt并接受回答，然后转成语音
void chat_completions_stream(String content)
{
    HTTPClient http;
    http.setTimeout(http_timeout); // 设置请求超时时间
    http.begin(base_url + "/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    String token_key = String("Bearer ") + openai_apiKey;
    http.addHeader("Authorization", token_key);

    // 向串口输出提示信息
    Serial.println("Send message to llm!");

    // 生成连接参数的JSON文档
    DynamicJsonDocument jsonData = generate_chat_json(content, true);

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsonData, jsonString);
    jsonData.clear();

    // 向串口输出生成的JSON字符串
    Serial.println("POST: " + base_url + "/v1/chat/completions");
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode == 200)
    {
        // 在 stream（流式调用） 模式下，基于 SSE (Server-Sent Events) 协议返回生成内容，每次返回结果为生成的部分内容片段
        WiFiClient* stream = http.getStreamPtr(); // 返回一个指向HTTP响应流的指针，通过它可以读取服务器返回的数据

        while (stream->connected())
        {
            // 这个循环会一直运行，直到客户端（即stream）断开连接。
            String line = stream->readStringUntil('\n'); // 从流中读取一行字符串，直到遇到换行符\n为止
            // 检查读取的行是否以data:开头。
            // 在SSE（Server-Sent Events）协议中，服务器发送的数据行通常以data:开头，这样客户端可以识别出这是实际的数据内容。
            if (line.startsWith("data:"))
            {
                // 如果行以data:开头，提取出data:后面的部分，并去掉首尾的空白字符。
                String data = line.substring(5);
                data.trim();
                // 输出读取的数据
                Serial.print("data: ");
                Serial.println(data);

                StaticJsonDocument<4096> jsonResponse;
                // 解析收到的数据
                DeserializationError error = deserializeJson(jsonResponse, data);

                // 如果解析没有错误
                if (!error)
                {
                    if (jsonResponse["choices"][0]["finish_reason"] == "stop")
                    {
                        stream->stop();
                        break;
                    }

                    const char* content = jsonResponse["choices"][0]["delta"]["content"];
                    jsonResponse.clear();
                    Serial.println(content);
                    // 将内容追加到Answer字符串中
                    Answer += content;
                    content = "";
                }
                else
                {
                    Answer = "";
                    stream->stop();
                    break;
                }
            }
        }
    }
    else
    {
        Serial.printf("Error %i \n", httpResponseCode);
        Serial.println(http.getString());
        http.end();
    }

    Serial.println("Answer: " + Answer);
}

DynamicJsonDocument generate_chat_json(String content, bool stream)
{
    // 创建一个容量为1500字节的动态JSON文档
    DynamicJsonDocument data(1500);

    data["max_tokens"] = 1024;
    data["temperature"] = 0.7;
    data["presence_penalty"] = 1.5;
    data["stream"] = stream;

    // 在message对象中创建一个名为text的嵌套数组
    JsonArray textArray = data["messages"].to<JsonArray>();

    JsonObject systemMessage = textArray.add<JsonObject>();
    systemMessage["role"] = "system";
    systemMessage["content"] = system_role;

    JsonObject userMessage = textArray.add<JsonObject>();
    userMessage["role"] = "user";
    userMessage["content"] = content;


    // 返回构建好的JSON文档
    return data;
}


int audioTranscriptions(int frame_index, String audio_id, int is_finished, uint8_t* audio_payload, size_t size,
                        String& output)
{
    HTTPClient http;
    http.setTimeout(http_timeout); // 设置请求超时时间
    http.begin(base_url + "/v1/audio/transcriptions");
    http.addHeader("Content-Type", "multipart/form-data");
    String token_key = String("Bearer ") + openai_apiKey;
    http.addHeader("Authorization", token_key);
    http.addHeader("Accept", "*");

    // Create the boundary string

    String boundary = "----WebKitFormBoundary" + randomString(16);
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    // Construct the multipart/form-data body
    String body = "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"audio_id\"\r\n\r\n";
    body += audio_id + "\r\n";
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"is_finish\"\r\n\r\n";
    body.concat(is_finished);
    body += "\r\n";
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"is_frame\"\r\n\r\n";
    body += "1\r\n";
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"frame_index\"\r\n\r\n";
    body.concat(frame_index);
    body += "\r\n";
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"audio_mime\"\r\n\r\n";
    body += "audio/L16;rate=8000\r\n";
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n";
    body += "Content-Type: audio/wav\r\n\r\n";
    // Convert the body to uint8_t format
    size_t bodyLength = body.length();
    uint8_t* bodyBytes = (uint8_t*)body.c_str();


    size_t totalLength = bodyLength + size + boundary.length() + 8;
    uint8_t* requestBody = new uint8_t[totalLength];
    memcpy(requestBody, bodyBytes, bodyLength);
    memcpy(requestBody + bodyLength, audio_payload, size);
    String endBoundary = "\r\n--" + boundary + "--\r\n";
    memcpy(requestBody + bodyLength + size, endBoundary.c_str(), endBoundary.length());

    Serial.print("POST: " + base_url + "/v1/audio/transcriptions");
    int httpResponseCode = http.POST(requestBody, totalLength);
    // Clean up
    delete[] requestBody;

    if (httpResponseCode == 200)
    {
        output = http.getString();
        http.end();
        return 0;
    }
    else
    {
        Serial.println(http.getString());
        http.end();
        return 1;
    }
}

String randomString(int len)
{
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const size_t maxIndex = (sizeof(charset) - 1);
    String str = "";
    for (int i = 0; i < len; ++i)
    {
        str += charset[rand() % maxIndex];
    }
    return str;
}
