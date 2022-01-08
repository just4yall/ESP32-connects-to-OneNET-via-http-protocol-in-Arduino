# ESP32 connects to OneNET via http protocol in Arduino
**Complete driving 0.96 oled screen, WIFI positioning, state machine, peripheral control, data upload and other functions**    
You may need installsupport package of the **ESP32 Arduino** development board, choose the **NodeMCU-32S**

![image](https://user-images.githubusercontent.com/39904013/148646891-4cc146cb-851f-4075-ba14-1218bd636e53.png)


And you will need header files:  
#include <U8g2lib.h>  
#include <Arduino.h>  
#include <Wire.h>  
#include <HTTPClient.h>  
#include <ArduinoJson.h>  
#include "WiFi.h"   
#include "Ticker.h"  
**And don’t forget to replace with your SSID, password, DviceId and API-Key**
