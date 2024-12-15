#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// 与AP模式和Web服务器有关的库
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

// AP模式的SSID和密码
extern const char *ap_ssid;
extern const char *ap_password;
// Web服务器和Preferences对象
extern AsyncWebServer server;
extern Preferences preferences;

void openWeb();
void handleRoot(AsyncWebServerRequest *request);
void handleWifiManagement(AsyncWebServerRequest *request);
void handleSave(AsyncWebServerRequest *request);
void handleDelete(AsyncWebServerRequest *request);
void handleList(AsyncWebServerRequest *request);


#endif