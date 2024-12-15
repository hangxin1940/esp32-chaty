#ifndef TFT_SCREEN_H
#define TFT_SCREEN_H

#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
#define screen_width   128     //屏幕宽度
#define screen_height  160     //屏幕高度

class TFTScreen
{
public:
    TFT_eSPI tft = TFT_eSPI(); // 创建TFT对象
    U8g2_for_TFT_eSPI u8g2;

    TFTScreen();
    ~TFTScreen();
    void init();
    void screen_init();
    void fillScreen(uint32_t color);
    void screen_zh_println();
    void screen_zh_println(uint32_t color, String text); // Corrected function signature
};

#endif //TFT_SCREEN_H