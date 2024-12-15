#include "AIClient.h"

AIClient::AIClient()
{
}


// 问题发送给Chatgpt并接受回答，然后转成语音
int AIClient::chat_completions(String content, String& output)
{
    HTTPClient http;
    http.setTimeout(http_timeout); // 设置请求超时时间
    http.begin(base_url + "/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    String token_key = String("Bearer ") + openai_apiKey;
    http.addHeader("Authorization", token_key);

    Serial.printf("POST: %s/v1/chat/completions\n", base_url.c_str());
    Serial.printf("\t%s\n", content.c_str());

    DynamicJsonDocument jsonData = generate_chat_json(content, false);

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsonData, jsonString);
    jsonData.clear();

    // 向串口输出生成的JSON字符串
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode == 200)
    {
        output = http.getString();
        http.end();
        StaticJsonDocument<4096> jsonResponse;
        // 解析收到的数据
        DeserializationError error = deserializeJson(jsonResponse, output);
        if (!error)
        {
            const char* content = jsonResponse["choices"][0]["message"]["content"];
            jsonResponse.clear();
            // 将内容追加到Answer字符串中
            output = content;
            content = "";
            Serial.printf("\t OUT:\n%s\n", output.c_str());
            return 0;
        }
        else
        {
            Serial.printf("Error json: %s\n", output.c_str());
            output = "";
            return 1;
        }
    }
    else
    {
        Serial.printf("Error %i \n", httpResponseCode);
        Serial.println(http.getString());
        http.end();
        return 1;
    }
}


// 问题发送给Chatgpt并接受回答，然后转成语音
int AIClient::chat_completions_stream(String content, String& output)
{
    HTTPClient http;
    http.setTimeout(http_timeout); // 设置请求超时时间
    http.begin(base_url + "/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    String token_key = String("Bearer ") + openai_apiKey;
    http.addHeader("Authorization", token_key);

    Serial.printf("POST: %s/v1/chat/completions\n", base_url.c_str());
    Serial.printf("\t%s\n", content.c_str());

    // 生成连接参数的JSON文档
    DynamicJsonDocument jsonData = generate_chat_json(content, true);

    // 将JSON文档序列化为字符串
    String jsonString;
    serializeJson(jsonData, jsonString);
    jsonData.clear();

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

                StaticJsonDocument<4096> jsonResponse;
                // 解析收到的数据
                DeserializationError error = deserializeJson(jsonResponse, data);

                // 如果解析没有错误
                if (!error)
                {
                    if (jsonResponse["choices"][0]["finish_reason"] == "stop")
                    {
                        stream->stop();
                        return 0;
                    }

                    const char* content = jsonResponse["choices"][0]["delta"]["content"];
                    jsonResponse.clear();
                    // 将内容追加到Answer字符串中
                    output += content;
                    Serial.printf("%s", content);
                    content = "";
                }
                else
                {
                    Serial.printf("Error json: %s\n", data.c_str());
                    if (output != nullptr && strcmp(output.c_str(), "") != 0)
                    {
                        stream->stop();
                        return 0;
                    }
                    stream->stop();
                    return 1;

                }
            }
        }
    }
    else
    {
        Serial.printf("Error %i \n", httpResponseCode);
        Serial.println(http.getString());
        http.end();
        return 1;
    }
}

DynamicJsonDocument AIClient::generate_chat_json(String content, bool stream)
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


int AIClient::audioTranscriptions(int frame_index, String audio_id, int is_finished, uint8_t* audio_payload, size_t size,
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

    Serial.printf("POST: %s/v1/audio/transcriptions finished:%d\n", base_url.c_str(), is_finished);
    int httpResponseCode = http.POST(requestBody, totalLength);
    // Clean up
    delete[] requestBody;

    if (httpResponseCode == 200)
    {
        output = http.getString();
        http.end();
        if (is_finished == 1)
        {
            StaticJsonDocument<4096> doc;
            // 解析收到的JSON数据
            DeserializationError error = deserializeJson(doc, output);

            // 如果解析没有错误
            if (!error)
            {
                const char* stt_text = doc["text"];
                output = stt_text;
                stt_text = "";
                Serial.printf("\tSTT: %s\n", output.c_str());
                return 0;
            } else
            {
                Serial.printf("Error json: %s\n", output.c_str());
                output = "";
                return 1;
            }
        }
        return 1;
    }
    else
    {
        Serial.printf("Error %i \n", httpResponseCode);
        Serial.println(http.getString());
        http.end();
        return 1;
    }
}

