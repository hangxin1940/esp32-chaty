#ifndef AI_CLIENT_H
#define AI_CLIENT_H

#include <Arduino.h>
#include "HTTPClient.h"
#include <ArduinoJson.h>
#include "helper.h"

class AIClient
{
public:
    String base_url = "";
    String openai_apiKey = "";
    String system_role = "";
    int http_timeout = 60000;
    AIClient();
    int chat_completions(String content, String& output);
    int chat_completions_stream(String content, String& output);
    DynamicJsonDocument generate_chat_json(String content, bool stream);
    int audioTranscriptions(int frame_index, String audio_id, int is_finished, uint8_t* audio_payload, size_t size,
                            String& output);
};


#endif //AI_CLIENT_H
