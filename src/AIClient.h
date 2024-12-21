#ifndef AI_CLIENT_H
#define AI_CLIENT_H

#include <Arduino.h>
#include "HTTPClient.h"
#include <ArduinoJson.h>
#include <tiny_websockets/client.hpp>
#include "helper.h"
#include "CAudio.h"
#include "base64.h"

class AIClient
{
public:
    String base_url = "";
    String openai_apiKey = "";
    String system_role = "";
    bool wsconnected = false;
    websockets::WebsocketsClient webSocketClient;
    CAudio* audio;
    int http_timeout = 60000;
    std::vector<String> tts_lines;
    AIClient();
    bool chat_completions(String content, String& output);
    bool chat_completions_stream(String content, String& output);
    DynamicJsonDocument generate_chat_json(String content, bool stream);
    int audioTranscriptions(int frame_index, String audio_id, int is_finished, uint8_t* audio_payload, size_t size,
                            String& output);
    void add_speech(String content);
    void play_speech();
    bool need_speech();
    void clear_speech();
    bool audioTranscriptionsWs_connect(String audio_id, void (*onconnect)());
    int audioTranscriptionsWs_sendframe(int frame_index, int is_finished, uint8_t* audio_payload, size_t size);
    void audioTranscriptionsWs_poll();
    void audioTranscriptionsWs_close();
};


#endif //AI_CLIENT_H
