#include "TFTScreen.h"

TFTScreen::TFTScreen()
{
}

TFTScreen::~TFTScreen()
{
}

void TFTScreen::init()
{
    tft.init();
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);
    tft.setTextWrap(true);

    u8g2.begin(tft);
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.setFontMode(1);
    u8g2.setForegroundColor(0x7E7B);
}

void TFTScreen::fillScreen(uint32_t color)
{
    tft.fillScreen(color);
    tft.setCursor(0, 0);
    u8g2.setCursor(0, 0);
}

void TFTScreen::screen_zh_println()
{
    u8g2.setCursor(0, u8g2.getCursorY() + 12);
}

void TFTScreen::screen_zh_println(uint32_t color, String text)
{
    u8g2.setForegroundColor(color); // Use the color parameter
    int cursorX = 0;
    int cursorY = u8g2.getCursorY() + 12;
    int lineHeight = u8g2.getFontAscent() - u8g2.getFontDescent() + 2;
    int start = 0;
    const char* text_char = text.c_str();
    int num = strlen(text_char);
    int i = 0;

    while (start < num)
    {
        u8g2.setCursor(cursorX, cursorY);
        int wid = 0;
        int numBytes = 0;

        while (i < num)
        {
            int size = 1;
            if (text_char[i] & 0x80)
            {
                char temp = text_char[i];
                temp <<= 1;
                do
                {
                    temp <<= 1;
                    ++size;
                }
                while (temp & 0x80);
            }
            char subWord[size];
            memcpy(subWord, &text_char[i], size);
            subWord[size] = '\0';
            int charBytes = size;

            int charWidth = charBytes == 3 ? 12 : 6;
            if (wid + charWidth > screen_width - cursorX)
            {
                break;
            }
            numBytes += charBytes;
            wid += charWidth;

            i += size;
        }

        if (cursorY <= screen_height - 10)
        {
            char subWord[numBytes];
            memcpy(subWord, &text_char[start], numBytes);
            subWord[numBytes] = '\0';

            u8g2.print(subWord);
            cursorY += lineHeight;
            cursorX = 0;
            start += numBytes;
        }
        else
        {
            break;
        }
    }
    if (cursorX > 0)
    {
        // u8g2.setCursor(cursorX, cursorY + 12);
    }
}