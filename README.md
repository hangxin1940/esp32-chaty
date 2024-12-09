ESP32-CHATY
=======

项目基于 __ESP_ChatX__

* [嘉立创 (Open01-X)](https://oshwhub.com/t-jupiter/esp_chatx_v2-2)
* [bilibili (Open01-X)](https://www.bilibili.com/video/BV16PzqYVEmQ)
* [说明文档](https://kdocs.cn/l/cfUhceO7OM0o)

## 硬件接线

![result.jpg](result.jpg)

### 麦克风

    VDD -> 3v3
    GND -> GND
    SD  -> GPIO14
    WS  -> GPIO2
    SCK -> GPIO1

### 音频放大模块

    VIN  -> V5IN
    GND  -> GND
    LRC  -> GPIO7
    BCLK -> GPIO6
    DIN  -> GPIO5

### 1.8寸OLED屏幕

    VDD -> V5IN
    GND -> GND
    SCL -> GPIO12
    SDA -> GPIO11
    RST -> GPIO15
    DC  -> GPIO16
    CS  -> GPIO10

### led灯

    正极 -> GPIO38
    负极 -> GND

## 构建前

将[User_Setup.h](User_Setup.h)覆盖至 `.pio/libdeps/esp32-s3-devkitm-1/TFT_eSPI/User_Setup.h`

