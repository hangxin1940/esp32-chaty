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

### 修改`TFT_eSPI`的引脚配置

修改 [lib/TFT_eSPI_Setups/User_Setup_Select.h](lib/TFT_eSPI_Setups/User_Setup_Select.h)

```
///////////////////////////////////////////////////////
//   User configuration selection lines are below    //
///////////////////////////////////////////////////////

// Only ONE line below should be uncommented to define your setup.  Add extra lines and files as needed.

#include <User_Setup.h>           // Default setup is root library folder
```

为

```
///////////////////////////////////////////////////////
//   User configuration selection lines are below    //
///////////////////////////////////////////////////////

// Only ONE line below should be uncommented to define your setup.  Add extra lines and files as needed.

#include <../TFT_eSPI_Setups/User_Setup.h>           // Default setup is root library folder
```

[lib/TFT_eSPI_Setups/User_Setup_Select.h](lib/TFT_eSPI_Setups/User_Setup_Select.h)
文件为指向[.pio/libdeps/esp32-s3-devkitm-1/TFT_eSPI/User_Setup_Select.h](.pio/libdeps/esp32-s3-devkitm-1/TFT_eSPI/User_Setup_Select.h)
的软连接，在项目初始化或`TFT_eSPI`库有变动时都需要修改。
