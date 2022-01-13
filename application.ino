#include <U8g2lib.h>
#include <Arduino.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "Ticker.h"
#define LED 2
#define AnalogRead 39
#define TTLL 36
#define TTLH 34
#define BLUE 19
#define RED 17
#define YELLOW 4
//SCL = GPIO 22
//SDA = GPIO 21
Ticker ticker1;
HTTPClient http;

// Access to WIFI
const char *ssid = "OCEAN";
const char *password = "123456789";

//Replace with your deviceid
String DeviceId = "879099838"; 

//Replace with your API-key
String Api_Key = "dcURB1GOmN4MZ0jk3=pzyuaZ5WA=";
String Url_Post="http://api.heclouds.com/devices/"+DeviceId+"/datapoints";

//url for positioning
String Url_Post_Mac="http://api.heclouds.com/devices/"+DeviceId+"/datapoints?type=3";
String Url_Get_Location="http://api.heclouds.com/devices/"+DeviceId+"/lbs/latestWifiLocation";

String Url_Get_datapoints="http://api.heclouds.com/devices/"+DeviceId+"/datastreams/";

//Global variables
char AnalogC[25];
boolean TTL_L;
boolean TTL_H;
int PressFlag = 0;
double maxAnalog = 0;
double minAnalog = 5;
unsigned long StartTime = 0;
unsigned long EndTime = 0;
unsigned long LastTime1 = 0;
unsigned long MiddleTime = 0;
unsigned long LastTime2 = 0;
unsigned long tempTime = 0;
bool TTL_Flag = false;
bool Second_TTL_Flag = false;
bool Mode_Continue_Flag = true;
int StateFlag = 0;
bool DeviceStatus = false;
bool SetUpOkFlag = false;


//Different types of arrays for uploading
String  id_bool[5]={"TTL_L","TTL_H","Lights","Fans","Speakers"};
boolean  value_bool[5];

String  id_double[3]={"Analog","maxAnalog","minAnalog"};
double  value_double[3];

String  id_mac[1]={"$OneNET_LBS_WIFI"};
String  value_mac[1];

String  id_lbs[1]={"OneNET_LBS_WIFI"};
double  value_lbs[2];

//For ssd1306 0.96 oled  
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0,SCL,SDA, /* reset=*/U8X8_PIN_NONE);


//Scan the surrounding WiFi for mac and RSSI and follow the ONENET format
void scanWiFi()
{ 
  String mac;
  Serial.println("scaning");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println("networks found");    
        for(int i=0;i<3;i++)
        {
          
          mac += WiFi.BSSIDstr(i);
          mac += ",";
          mac += (String)WiFi.RSSI(i);
          if(i<2)
          {
            mac += "|";
          }
        }
        //Serial.println(mac);
        value_mac[0] = mac;
    }
}

//Get the value of datapoints or calculated latitude and longitude of the address through the API 
void getInformation(String choice)
{
  if(choice == "location")
  {
    http.begin(Url_Get_Location);
  }
  else
  {
    String Url = Url_Get_datapoints;
    Url += choice;
    http.begin(Url);
  }
  http.addHeader("api-key",Api_Key);
  int httpcode = http.sendRequest("GET");
  Serial.println(httpcode);
  String json;
  if(httpcode > 0)
  {
    json = http.getString();
    //Serial.println(json);
  }
  else 
  {
    Serial.printf("[HTTP] GET... failed, error: %s\n",http.errorToString(httpcode).c_str());
  }

  http.end();
  
  //Parse through json format
  StaticJsonDocument<512> revDoc;
  DeserializationError error = deserializeJson(revDoc, json);
  if (error) 
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  //Stored in the array
  if(choice == "location")
  {
    value_lbs[0]= revDoc["data"]["lon"];
    value_lbs[1]= revDoc["data"]["lat"]; 
  }
  if(choice =="dev1")
  {
    digitalWrite(BLUE,revDoc["data"]["current_value"]);
  }
  else if(choice =="dev2")
  {
    digitalWrite(RED,revDoc["data"]["current_value"]);
  }
  else if(choice =="dev3")
  {
    digitalWrite(YELLOW,revDoc["data"]["current_value"]);
  }
}

//Initialize WIFI and connect to hotspot
void setupWifi()
{
 delay(10);
 WiFi.disconnect();
 delay(10);
 Serial.println("连接WiFi");
 WiFi.begin(ssid,password);
 while(!WiFi.isConnected())
 {
  Serial.println(".");
  delay(500);
 }
 Serial.println("连接成功");
}


//Refresh and display the desired content on the oled screen
void refresh(char* analog,boolean TTL_H,boolean TTL_L)
{
  u8g2.clearBuffer();//
  u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
  if(value_bool[2]==true)
  {
    u8g2.drawGlyph(95, 16, 0x0056);
  }
  if(value_bool[3]==true)
  {
    u8g2.drawGlyph(105, 16, 0x0047);
  }
  if(value_bool[4]==true)
  {
    u8g2.drawGlyph(115, 16, 0x00E1);
  }

  u8g2.setFont(u8g2_font_chargen_92_me);
  u8g2.drawLine(0,21,127,21);
  if(TTL_H == true || PressFlag == 2)
  {
    u8g2.drawStr(0,16,"Left.");
    PressFlag = 2; //To keep showing
   }
   if(TTL_L == false || PressFlag == 1)
  {
   u8g2.drawStr(0,16,"Pressed.");
   PressFlag = 1;//To keep showing
   }
   u8g2.drawStr(50,63,analog);
   u8g2.drawStr(0,40,"Voltage:");
   u8g2.drawStr(114,63,"V");
   u8g2.sendBuffer();
}

//To find the maximum value over a period of time
double Max(double a,double b)
{
  if(a>b)
  {
    return a;
  }
  else
  return b;
}

//To find the minimum value over a period of time
double Min(double a,double b)
{
  if(a<b)
  {
    return a;
  }
  else
  return b;
}

//Control external devices
//For now, it's just a light bulb
//param: Device options
void DeviceControl(String choice)
{
  if(choice == "BLUE")
  {
    digitalWrite(BLUE,!digitalRead(BLUE));
  }
  else if(choice == "RED")
  {
    digitalWrite(RED,!digitalRead(RED));
  }
  else if(choice == "YELLOW")
  {
    digitalWrite(YELLOW,!digitalRead(YELLOW));
  }
  else if(choice == "ALL")
  { 
    //To switch equipment together
    if(DeviceStatus == true)
    {
      digitalWrite(BLUE,DeviceStatus);
      digitalWrite(RED,DeviceStatus);
      digitalWrite(YELLOW,DeviceStatus);
      DeviceStatus = false;
    }
    else
    {
      digitalWrite(BLUE,DeviceStatus);
      digitalWrite(RED,DeviceStatus);
      digitalWrite(YELLOW,DeviceStatus);
      DeviceStatus = true;
    }
    
  }
}
//The state machine generates four control states through the state of two TTL high and low levels,
//namely long press, single click, double click, and triple click, so as to control different devices.
//One port is low level to indicate pressing, and the other port's High level means leave.
//params: Boolean variable for two ports
void modeChoice(bool L,bool H)
{
  if(L == false)
  {
    StartTime = millis();
    delay(20);
  }
  if(H == true)
  {
    
    MiddleTime = millis();    
    LastTime1 = MiddleTime-StartTime;//Calculate the time between high and low level signals in two ports
    if(LastTime1>1000)
    {
      //control all
      DeviceControl("ALL");
      Serial.println("control all");
    }
    else
    {
      StateFlag += 1; //status flag
      Mode_Continue_Flag = true;//Anti-early judgment flag
    }
    delay(35);//Avoid reading twice
    EndTime = millis();//Record the last high level generation time, if the time is too long, the state needs to be cleared
    Second_TTL_Flag = true;//Record the last high level generation flag to prevent the state from being judged when there is no signal
   }

  //Watchdog, logging and clearing status
  tempTime = millis();
  if((tempTime-EndTime)>1000 && Second_TTL_Flag == true)
  {
    Serial.println(StateFlag);
    Mode_Continue_Flag = false;
    Second_TTL_Flag = false;
  }
  //Compare status
  if(Mode_Continue_Flag == false)
    {
      switch(StateFlag)
      {
//        case 0:
//          //control all
//          DeviceControl("ALL");
//          Serial.println("control all");
//          break;
        case 1:
          //touch once
          DeviceControl("BLUE");
          Serial.println("touch once");
         break;
        case 2:
          //touch twice
          DeviceControl("RED");
          Serial.println("touch twice");
          break;
        case 3:
          //touch third
          DeviceControl("YELLOW");
          Serial.println("touch third");
          break;
       }
       StateFlag = 0;
       Mode_Continue_Flag = true;
    }
}

//Read the port status cyclically and assign it to the data upload array
void readInfor()
{
  double Analog;
  Analog = analogRead(AnalogRead); 
  Analog=(Analog/4096)*3.3;
  maxAnalog = Max(maxAnalog,Analog);//find the maximum
  minAnalog = Min(minAnalog,Analog);//find the minimum
  dtostrf(Analog,1,3,AnalogC);//Converted to a character array for use by u8g2 functions

  TTL_L=digitalRead(TTLL);
  TTL_H=digitalRead(TTLH);
  modeChoice(TTL_L,TTL_H);
  value_double[0]= Analog;
  value_double[1]= maxAnalog;
  value_double[2]= minAnalog;
  value_bool[0]= TTL_L;
  value_bool[1]= TTL_H;
  value_bool[2]=digitalRead(BLUE);
  value_bool[3]=digitalRead(RED);
  value_bool[4]=digitalRead(YELLOW);
}

//Control the onboard LED lights
void ledControl(bool led)
{
   if(led == true)
    {
      digitalWrite(LED, HIGH);
    }  // turn the LED on (HIGH is the voltage level)
   else  
   {
    digitalWrite(LED, LOW);
   }

}

//Upload the data in the array to its server in ONENET format
//param：options
int sendData(String choice) 
{
  
  String values;
  int r;
  //The upload mac address is different from the url of other data
  if(choice=="sendmac")
  {
    http.begin(Url_Post_Mac);
  }
  else
  {
    http.begin(Url_Post);
  }
  http.addHeader("api-key",Api_Key);
  //Create json files to facilitate data integration
  StaticJsonDocument<512> sendDoc;
  //Different types of data need to exist in different arrays
  if(choice == "bool")
  { 
    JsonArray datastreams = sendDoc.createNestedArray("datastreams");
    int len=sizeof(id_bool)/sizeof(id_bool[0]);
    for(int i=0;i<len;i++)
    {
      JsonObject datastream = datastreams.createNestedObject();
      datastream["id"]=id_bool[i];
      JsonArray datapoints = datastream.createNestedArray("datapoints");
      JsonObject datapoint = datapoints.createNestedObject();
      datapoint["value"]=value_bool[i];
    }
  }
  else if(choice == "double")
  { 
    JsonArray datastreams = sendDoc.createNestedArray("datastreams");
    int len=sizeof(id_double)/sizeof(id_double[0]);
    for(int i=0;i<len;i++)
    {
      JsonObject datastream = datastreams.createNestedObject();
      datastream["id"]=id_double[i];
      JsonArray datapoints = datastream.createNestedArray("datapoints");
      JsonObject datapoint = datapoints.createNestedObject();
      datapoint["value"]=value_double[i];
    }
  }
  else if(choice == "sendlbs")
  { 
    JsonArray datastreams = sendDoc.createNestedArray("datastreams");
    int len=sizeof(id_double)/sizeof(id_double[0]);
    for(int i=0;i<len;i++)
    {
      JsonObject datastream = datastreams.createNestedObject();
      datastream["id"]=id_lbs[i];
      JsonArray datapoints = datastream.createNestedArray("datapoints");
      JsonObject datapoint = datapoints.createNestedObject();
      JsonObject values = datapoint.createNestedObject("value");
      values["lon"] = value_lbs[i];
      values["lat"] = value_lbs[i+1];
    }
  } 
  else if(choice == "sendmac")
  { 
    JsonObject OneNET_LBS_WIFI = sendDoc.createNestedObject("$OneNET_LBS_WIFI");
    int len=sizeof(id_mac)/sizeof(id_mac[0]);
    for(int i=0;i<len;i++)
    {
      //JsonObject datastream = datastreams.createNestedObject();
      OneNET_LBS_WIFI["macs"]=value_mac[i];
    }
  }
  //Convert json file to string
  serializeJson(sendDoc, values);
  //Serial.println(values);
  r = http.sendRequest("POST",values);
  http.end();
  return r;
}
//Timer callback function, automatically called when time is up
void callback()
{
   
   if(SetUpOkFlag == true)
   {
    
    ledControl(true); 
    getInformation("dev1");
    getInformation("dev2");
    getInformation("dev3");
    sendData("bool");
    sendData("double");
    getInformation("location");
    sendData("sendlbs");
    ledControl(false);
    maxAnalog = 0;
    minAnalog = 5;
    PressFlag = 0; 
   }
   
}
void setup() {
    //put your setup code here, to run once:
    u8g2.begin();
    u8g2.enableUTF8Print();
    ticker1.attach(10,callback);
    Serial.begin(115200);
    delay(3000);
    setupWifi();
    //Scan nearby wifi, and upload mac to calculate latitude and longitude, open when WiFi positioning is required
    scanWiFi();
    sendData("sendmac");
    
    pinMode(LED, OUTPUT);
    pinMode(BLUE, OUTPUT);
    pinMode(YELLOW, OUTPUT);
    pinMode(RED, OUTPUT);
    pinMode(TTLL, INPUT);
    pinMode(TTLH, INPUT); 
    SetUpOkFlag = true; 
}


void loop() {

  if(!WiFi.isConnected())
  {
    setupWifi();
  }
  readInfor();
  refresh(AnalogC,TTL_H,TTL_L);
  //delay(10);     
}
