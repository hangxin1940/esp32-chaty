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
String base_url = "http://192.168.1.6:12345";

// 定义一些全局变量
bool ledstatus = true; // 控制led闪烁
unsigned long urlTime = 0;
int noise_low = 200; // 噪声门限值
int volume = 80; // 初始音量大小（最小0，最大100）
int silence_max = 8; //最大静音周期
int voice_max = 2; //录音周期
// 音乐播放
int mainStatus = 0;
int conStatus = 0;
int musicnum = 0; // 音乐位置下标
int musicplay = 0; // 是否进入连续播放音乐状态
int cursorY = 0;
// 语音唤醒
int awake_flag = 1;


// 定义字符串变量，用于存储鉴权参数
String Date = "";

String askquestion = ""; // 存储stt语音转文字信息，即用户的提问信息
String Answer = ""; // 存储llm回答，用于语音合成（较短的回答）
std::vector<String> subAnswers; // 存储llm回答，用于语音合成（较长的回答，分段存储）
int subindex = 0; // subAnswers的下标，用于voicePlay()
String text_temp = ""; // 存储超出当前屏幕的文字，在下一屏幕显示
int loopcount = 0; // 对话次数计数器
int flag = 0; // 用来确保subAnswer1一定是大模型回答最开始的内容
int conflag = 0; // 用于连续对话
int await_flag = 1; // 待机标识
int start_con = 0; // 标识是否开启了一轮对话
int image_show = 0;

using namespace websockets; // 使用WebSocket命名空间
// 创建WebSocket客户端对象
WebsocketsClient webSocketClient; // 与llm通信
WebsocketsClient webSocketClient1; // 与stt通信

// 创建音频对象
Mic mic;
Audio audio(false, 3, I2S_NUM_1);
// 参数: 是否使用内部DAC（数模转换器）如果设置为true，将使用ESP32的内部DAC进行音频输出。否则，将使用外部I2S设备。
// 指定启用的音频通道。可以设置为1（只启用左声道）或2（只启用右声道）或3（启用左右声道）
// 指定使用哪个I2S端口。ESP32有两个I2S端口，I2S_NUM_0和I2S_NUM_1。可以根据需要选择不同的I2S端口。

// 函数声明
void displayWrappedText(const string& text1, int x, int y, int maxWidth);
void generateRoleChat(String role, String content);
void checkLen();
float calculateRMS(uint8_t* buffer, int bufferSize);
int wifiConnect();
void getTimeFromServer();
void chat_completions(String content);
DynamicJsonDocument generate_chat_json(String content);
int audioTranscriptions(int frame_index, String audio_id, int is_finished, uint8_t* audio_payload, size_t size,
                        String& output);
String randomString(int len);
void startRecording();
void tft_print_chat(String role, String content);
char* urlencode(const char* str, bool spacesOnly);

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
    tft.init();
    tft.setRotation(0); // 设置屏幕方向，0-3顺时针转
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK); // 设置屏幕背景为黑色
    // tft.setTextColor(TFT_BLUE); // 设置字体颜色为白色
    // tft.setTextColor(0x5DCE); // 设置字体颜色为黑色
    tft.setTextWrap(true); // 开启文本自动换行，只支持英文

    // 初始化U8g2
    u8g2.begin(tft);
    u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体库
    u8g2.setFontMode(1); // 设置字体模式为透明模式，不设置的话中文字符会变成一个黑色方块
    u8g2.setForegroundColor(0x7E7B); // 设置字体颜色为黑色
    // 显示文字
    u8g2.setCursor(0, 11);
    u8g2.print("HELLO CHAT-Y !");

    // 初始化音频模块mic
    mic.init();
    // 设置音频输出引脚和音量
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(volume);

    // 初始化Preferences
    preferences.begin("wifi_store");
    preferences.begin("music_store");
    // 连接网络
    u8g2.setCursor(0, u8g2.getCursorY() + 12);
    u8g2.print("正在连接网络······");
    int result = wifiConnect();

    // 从百度服务器获取当前时间
    getTimeFromServer();

    if (result == 1)
    {
        // 清空屏幕，在屏幕上输出提示信息
        tft.fillScreen(TFT_BLACK);
        u8g2.setCursor(0, 11);
        u8g2.print("网络连接成功！");
        displayWrappedText("请进行语音唤醒或按boot键开始对话！", 0, u8g2.getCursorY() + 12, width);
        awake_flag = 0;
    }
    else
    {
        openWeb();
    }
    // 记录当前时间，用于后续时间戳比较
    urlTime = millis();
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
    if (!audio.isRunning() && Answer == "" && subindex == subAnswers.size() && musicplay == 0 && conflag == 1 &&
        image_show == 0)
    {
        loopcount++;
        Serial.print("loopcount：");
        Serial.println(loopcount);
        startRecording();
    }
}

// 自动换行显示u8g2文本的函数
void displayWrappedText(const string& text1, int x, int y, int maxWidth)
{
    tft.setTextColor(TFT_BLUE); // 设置字体颜色
    int cursorX = x;
    int cursorY = y + 24;
    int lineHeight = u8g2.getFontAscent() - u8g2.getFontDescent() + 2; // 中文字符12像素高度
    int start = 0; // 指示待输出字符串已经输出到哪一个字符
    int num = text1.size();
    int i = 0;

    while (start < num)
    {
        u8g2.setCursor(cursorX, cursorY);
        int wid = 0;
        int numBytes = 0;

        // Calculate how many bytes fit in the maxWidth
        while (i < num)
        {
            int size = 1;
            if (text1[i] & 0x80)
            {
                // 核心所在
                char temp = text1[i];
                temp <<= 1;
                do
                {
                    temp <<= 1;
                    ++size;
                }
                while (temp & 0x80);
            }
            string subWord;
            subWord = text1.substr(i, size); // 取得单个中文或英文字符

            int charBytes = subWord.size(); // 获取字符的字节长度

            int charWidth = charBytes == 3 ? 12 : 6; // 中文字符12像素宽度，英文字符6像素宽度
            if (wid + charWidth > maxWidth - cursorX)
            {
                break;
            }
            numBytes += charBytes;
            wid += charWidth;

            i += size;
        }

        if (cursorY <= height - 10)
        {
            u8g2.print(text1.substr(start, numBytes).c_str());
            cursorY += lineHeight;
            cursorX = 0;
            start += numBytes;
        }
        else
        {
            text_temp = text1.substr(start).c_str();
            break;
        }
    }
}


void tft_print_chat(String role, String content)
{
    int cursorX = 0;
    int cursorY = 0; // 默认位置

    if (role == "user")
    {
        cursorY = 0; // 用户角色，内容在 (0, 0)
        tft.setCursor(cursorX, cursorY);
        tft.setTextColor(0xFA20); // 设置字体颜色为
        tft.print(role);
        tft.print(": ");
        u8g2.setForegroundColor(0x7E7B); // 设置字体颜色为黑色
        // tft.setTextColor(TFT_PINK); // 设置字体颜色
    }
    else if (role == "assistant")
    {
        cursorY = 70; // 助手角色，内容在 (0, 70)
        tft.setCursor(cursorX, cursorY);
        tft.setTextColor(0x7FE0); // 设置字体颜色为
        tft.print(role);
        tft.print(": ");
        u8g2.setForegroundColor(0x8DF1); // 设置字体颜色为黑色
        // tft.setTextColor(0x8DF1); // 设置字体颜色为
    }

    displayWrappedText(content.c_str(), cursorX, tft.getCursorY(), width);

    // 更新光标位置用于后续内容
    tft.setCursor(0, tft.getCursorY() + 2);
}

// 音量控制
void VolumeSet(String numberStr)
{
    volume = numberStr.toInt();
    audio.setVolume(volume);
    Serial.print("音量已调到: ");
    Serial.println(volume);
    // 在屏幕上显示音量
    tft.fillRect(66, 152, 62, 7, TFT_WHITE);
    tft.setCursor(66, 152);
    tft.print("volume:");
    tft.print(volume);

    conflag = 1;
}

void speakAnswer()
{
    if (Answer != "")
    {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.print("assistant: ");

        String url = base_url + "/v1/audio/speech?file=1&input=" + Answer;
        audio.connecttohost(url.c_str());
        displayWrappedText(Answer.c_str(), tft.getCursorX(), tft.getCursorY(), width);
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
        tft.fillScreen(TFT_BLACK);
        u8g2.setCursor(0, 11);
        u8g2.print("待机中......");
        // tft.pushImage(0, 0, width, height, image_data_client_pic);
        tft.pushImage(0, 0, width, height, image_data_tiger_1);
    }
    else if (conflag == 1)
    {
        tft.fillScreen(TFT_BLACK);
        u8g2.setCursor(0, 11);
        u8g2.print("连续对话中，请说话！");
    }
    else
    {
        u8g2.setCursor(0, 159);
        u8g2.print("请说话！");
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
            webSocketClient1.close();
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
                            tft.fillScreen(TFT_BLACK);
                            tft.setCursor(0, 0);
                            // 发送给大模型
                            tft_print_chat("user", stt_text);
                            chat_completions(stt_text);
                            speakAnswer();
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
        u8g2.setCursor(0, u8g2.getCursorY() + 12);
        u8g2.print("无任何wifi存储信息！");
        displayWrappedText("请连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！", 0, u8g2.getCursorY() + 12,
                           width);
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
            u8g2.setCursor(0, u8g2.getCursorY() + 12);
            u8g2.print(ssid);

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
                    u8g2.setCursor(u8g2.getCursorX() + 6, u8g2.getCursorY());
                    u8g2.print("Failed!");
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
                u8g2.setCursor(u8g2.getCursorX() + 6, u8g2.getCursorY());
                u8g2.print("Connected!");
                preferences.end();
                return 1;
            }
        }
    }
    // 清空屏幕
    tft.fillScreen(TFT_BLACK);
    // 在屏幕上输出提示信息
    u8g2.setCursor(0, 11);
    u8g2.print("网络连接失败！请检查");
    u8g2.setCursor(0, u8g2.getCursorY() + 12);
    u8g2.print("网络设备，确认可用后");
    u8g2.setCursor(0, u8g2.getCursorY() + 12);
    u8g2.print("重启设备以建立连接！");
    displayWrappedText("或者连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！", 0, u8g2.getCursorY() + 12,
                       width);
    preferences.end();
    return 0;
}

void getTimeFromServer()
{
    String timeurl = "https://www.baidu.com"; // 定义用于获取时间的URL
    HTTPClient http; // 创建HTTPClient对象
    http.begin(timeurl); // 初始化HTTP连接
    const char* headerKeys[] = {"Date"}; // 定义需要收集的HTTP头字段
    http.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0])); // 设置要收集的HTTP头字段
    int httpCode = http.GET(); // 发送HTTP GET请求
    Date = http.header("Date"); // 从HTTP响应头中获取Date字段
    Serial.println(Date); // 输出获取到的Date字段到串口
    http.end(); // 结束HTTP连接

    // delay(50); // 根据实际情况可以添加延时，以便避免频繁请求
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
    http.setTimeout(20000); // 设置请求超时时间
    http.begin(base_url + "/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    String token_key = String("Bearer ") + openai_apiKey;
    http.addHeader("Authorization", token_key);

    // 向串口输出提示信息
    Serial.println("Send message to llm!");

    // 生成连接参数的JSON文档
    DynamicJsonDocument jsonData = generate_chat_json(content);

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsonData, jsonString);
    jsonData.clear();

    // 向串口输出生成的JSON字符串
    Serial.println(jsonString);
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

                StaticJsonDocument<400> jsonResponse;
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

DynamicJsonDocument generate_chat_json(String content)
{
    // 创建一个容量为1500字节的动态JSON文档
    DynamicJsonDocument data(1500);

    data["max_tokens"] = 1024;
    data["temperature"] = 0.7;
    data["presence_penalty"] = 1.5;
    data["stream"] = true;

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
    http.setTimeout(20000); // 设置请求超时时间
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
