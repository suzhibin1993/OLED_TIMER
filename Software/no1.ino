//Free and open source 
/* 代码修改：冰凌影子BL   bilibili UID:177810096
*   感谢UP：Hans叫泽涵  bilibili UID:15481541
 *  感谢UP:会飞的阿卡林  bilibili UID:751219
 *  感谢UP:黄埔皮校大校长  bilibili UID:403928979
 *  感谢UP:F_KUN  bilibili UID:8515147
 *  感谢UP:啊泰_  bilibili UID:23106193
 *  感谢UP:硬核拆解  bilibili UID:427494870
 *  
 * 本代码适用于ESP8266 NodeMCU + 12864 SSD1306 OLED显示屏
 * 
 * 7pin SPI引脚，正面看，从左到右依次为GND、VCC、D0、D1、RES、DC、CS
 *    ESP8266 ---  OLED
 *      3V    ---  VCC
 *      G     ---  GND
 *      D7    ---  D1
 *      D5    ---  D0
 *      D2orD8---  CS
 *      D1    ---  DC
 *      RST   ---  RES
 *      
 * 4pin IIC引脚，正面看，从左到右依次为GND、VCC、SCL、SDA
 *      ESP8266  ---  OLED
 *      3.3V     ---  VCC
 *      G (GND)  ---  GND
 *      D1(GPIO5)---  SCL
 *      D2(GPIO4)---  SDA
 */
 
#include <TimeLib.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>    //json  V6版本
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <ArduinoFont.h> 
#include <PubSubClient.h>  //安装PubSubClient库
#include "aliyun_mqtt.h"    //需要安装crypto库

#define Project "WIFI时钟气象站"
#define Edition "Ver1.0"
#define KEY_FLASH 0

//================函数声明=====================
bool WifiAutoConfig();
void WifiSmartConfig();
void key_process();
void Time_Show();

void sendNTPpacket(IPAddress& address);
void oledClockDisplay();
void sendCommand(int command, int value);
void initdisplay();
void WeatherClockMirror();
void Time_Show();
void key_process();
void CurrentTime_update();
bool GetWeatherJson();
void shangxiajingxiang(unsigned char *Buff,unsigned char Row,unsigned char Column);
void ParseNowJson(String json);
void WeatherRollClock();
//void ParseDailyJson();
//void WeatherDisplay();
void FansDisplay();
void WeatherClockDisplay();
void RollClockDisplay();
void ParseNowJson();

int GetDays(int iYear1, int iMonth1, int iDay1, int iYear2, int iMonth2, int iDay2);
bool IsLeapYear(int iYear);

void ParseDailyJson(String json);
void WeatherClockDisplay(char times);
void WeatherDisplay(char Day,char code,char Temp,char Temp_HIGH,char Temp_LOW,char Humi,char Wind_scale,String Date,String Wind_direction);
void LED_Rolling(unsigned char *Buff,unsigned char *aBuff,unsigned char *bBuff,unsigned char row,unsigned char Column,unsigned char type,unsigned char t);
//---------------------------------系统时间---------------------------------
typedef struct __SYSTEMTIME__
{
  int Second;  //
  int Minute;  //
  int Hour;    //
  int Week;    //
  int Day;     //
  int Month;   //
  int Year;    //
//int  days;    //
}SYSTEMTIME;
SYSTEMTIME CurrentTime;
int LastUpdateTimeSecond=0;//时钟更新参数
int LastSecond=0;
///-------------------------------------结构体定义-------------------------------------
typedef struct
{
  String  Date;
  String  text_day;
  String  Wind_direction;
  char    code;
  char    Temp_HIGH;
  char    Temp_LOW;
  char    Wind_scale;
  char    Humi;  
}WeatherForecastInfo; //天气预报结构体
WeatherForecastInfo Day0,Day1,Day2;

typedef struct
{
  char    code;
  char    Temp; 
  String  last_update;
  String  Location;
}NowWeatherInfo; //天气实况结构体
NowWeatherInfo Today;

typedef struct
{
   long uid;
   int follower;//粉丝数
   int view;    //视频观看次数
   int likes;   //点赞数
}biliInfo; //天气实况结构体
biliInfo Bilibili;

ADC_MODE(ADC_VCC);
//////---------------------------------------按键定义----------------------------------------
#define LED     13       //蜂鸣器

const int fun1 = 12;     // the number of the pushbutton pin1      右键
const int fun2 = 14;     // the number of the pushbutton pin2      左键
//const int buzzer =  13;      // the number of the LED pin          蜂鸣器
int button1State = 0;         // variable for reading the pushbutton status      右键
int button2State = 0;         // variable for reading the pushbutton status      左键
int LEDState = 0;         // variable for reading the pushbutton status      蜂鸣器
int button1State_old= 0;//
int button2State_old= 0;//

int Ldays = 0;


#define PRODUCT_KEY      "***************" //替换自己的PRODUCT_KEY
#define DEVICE_NAME      "***************" //替换自己的DEVICE_NAME
#define DEVICE_SECRET    "***************"//替换自己的DEVICE_SECRET

#define DEV_VERSION       "S-TH-WIFI-v1.0-20190220"        //固件版本信息

#define ALINK_BODY_FORMAT         "{\"id\":\"123\",\"version\":\"1.0\",\"method\":\"%s\",\"params\":%s}"
#define ALINK_TOPIC_PROP_POST     "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"
#define ALINK_TOPIC_PROP_POSTRSP  "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post_reply"
#define ALINK_TOPIC_PROP_SET      "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/service/property/set"
#define ALINK_METHOD_PROP_POST    "thing.event.property.post"
#define ALINK_TOPIC_DEV_INFO      "/ota/device/inform/" PRODUCT_KEY "/" DEVICE_NAME ""    
#define ALINK_VERSION_FROMA      "{\"id\": 123,\"params\": {\"version\": \"%s\"}}"
unsigned long lastMs = 0;

WiFiClient   espClient;
PubSubClient mqttClient(espClient);
//////-------------------------------OLED型号选择-------------------------------------------------
//U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/4, /* dc=*/5, /* reset=*/3);
//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

//-------------------------------修改此处WIFI内的信息---------------------------------------------
const char ssid[] = "***************";                       //WiFi名
const char pass[] = "***************";                   //WiFi密码
String biliuid = "***************";         //bilibili UID

String City = "ip";                                //城市 天气城市 ip根据ip归属地获取天气 wuhan根据城市名获取
String My_Key = "***************";               //禁止泄露 私人密钥 填入自己注册知心天气后得到的免费秘钥
String Language = "en";   //语言：英语en 简体中文zh-Hans https://docs.seniverse.com/api//common.html#%E8%AF%AD%E8%A8%80-language

//天气预报请求地址   实况天气地址
String DailyURL ="http://api.seniverse.com/v3/weather/daily.json?key="+My_Key+"&location="+City+"&language="+Language+"&unit=c&start=0&days=3";
String NowURL ="http://api.seniverse.com/v3/weather/now.json?key="+My_Key+"&location="+City+"&language="+Language+"&unit=c";

bool WeatherFlag = false;     //天气请求标志
bool FansFlag    = true;      //Bili数据请求标志

//-----------------------------------------------------------------------------

static const char ntpServerName[] = "ntp1.aliyun.com"; //NTP服务器，阿里云
const int timeZone = 8;                                //时区，北京时间为+8

char ClockMode = 0;

const unsigned long HTTP_TIMEOUT = 5000;
WiFiClient client;
HTTPClient http;
WiFiUDP Udp;
unsigned int localPort = 8888; // 用于侦听UDP数据包的本地端口
//-------------------------------时间滚动显示相关参数-------------------------------
//滚动
#define NO_Roll    0
#define UP_Roll    1
#define DOWN_Roll  2
#define LEFT_Roll  3
#define RIGHT_Roll 4
char f0 =0,f1 = 0,f2=0, f3 = 0,f4=0,f5 = 0; 
char Timetemp[6]={10,10,10,10,10,10};
char timeroll[6];
char Roll_mode = 0;  //
char TimeFont_mode = 0;
unsigned char hcbuff[72];
unsigned long LastShowTime = 0;  
bool RollTimeFlag = true;

time_t getNtpTime();



char GetFansFlag = 0;
boolean isNTPConnected = false;

void(* resetFunc) (void) = 0;





bool getJson();
bool parseJson(String json);
void parseJson1(String json);



const unsigned char xing[] U8X8_PROGMEM = {
  0x00, 0x00, 0xF8, 0x0F, 0x08, 0x08, 0xF8, 0x0F, 0x08, 0x08, 0xF8, 0x0F, 0x80, 0x00, 0x88, 0x00,
  0xF8, 0x1F, 0x84, 0x00, 0x82, 0x00, 0xF8, 0x0F, 0x80, 0x00, 0x80, 0x00, 0xFE, 0x3F, 0x00, 0x00
};  /*星*/
const unsigned char liu[] U8X8_PROGMEM = { 
  0x40, 0x00, 0x80, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00,
  0x20, 0x02, 0x20, 0x04, 0x10, 0x08, 0x10, 0x10, 0x08, 0x10, 0x04, 0x20, 0x02, 0x20, 0x00, 0x00
};  /*六*/


//24*24小电视
const unsigned char bilibilitv_24u[] U8X8_PROGMEM = {0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x30, 0x00, 0x01, 0xe0, 0x80, 0x01,
                                                     0x80, 0xc3, 0x00, 0x00, 0xef, 0x00, 0xff, 0xff, 0xff, 0x03, 0x00, 0xc0, 0xf9, 0xff, 0xdf, 0x09, 0x00, 0xd0, 0x09, 0x00, 0xd0, 0x89, 0xc1,
                                                     0xd1, 0xe9, 0x81, 0xd3, 0x69, 0x00, 0xd6, 0x09, 0x91, 0xd0, 0x09, 0xdb, 0xd0, 0x09, 0x7e, 0xd0, 0x0d, 0x00, 0xd0, 0x4d, 0x89, 0xdb, 0xfb,
                                                     0xff, 0xdf, 0x03, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x78, 0x00, 0x1e, 0x30, 0x00, 0x0c
                                                    };

const unsigned char bilibili_16x16[][32] U8X8_PROGMEM = {
{0x00,0x00,0x18,0x18,0x30,0x0C,0x60,0x06,0xFE,0x7F,0xFF,0xFF,0x03,0xC0,0x33,0xCC,
0x1B,0xD8,0x03,0xC0,0x03,0xC0,0xA3,0xC5,0x43,0xC2,0x03,0xC0,0xFF,0xFF,0xFE,0x7F},//0电视1

{0x00,0x00,0x18,0x18,0x30,0x0C,0x60,0x06,0xFE,0x7F,0xFF,0xFF,0x03,0xC0,0x1B,0xD8,
0x33,0xCC,0x03,0xC0,0x03,0xC0,0x43,0xC2,0xA3,0xC5,0x03,0xC0,0xFF,0xFF,0xFE,0x7F},//1电视2

{0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x0C,0x06,0x0C,0x06,0x0C,0x02,0x0C,0x02,0x08,
0x82,0xAA,0x9E,0xAA,0x3E,0x08,0x66,0x49,0x66,0x49,0x3E,0x49,0x1E,0x49,0x00,0x00},//2bibi

{0x00,0x00,0x08,0x14,0x2A,0x14,0x2A,0x22,0x2A,0x22,0x08,0x41,0x7F,0x00,0x08,0x3F,
0x2A,0x48,0x2A,0x48,0x2A,0x44,0x49,0x44,0x49,0x42,0x49,0x31,0x00,0x00,0x00,0x00},//3粉丝

{0xE0,0x07,0x50,0x0A,0x88,0x11,0x04,0x20,0x84,0x21,0x44,0x23,0xC4,0x23,0x84,0x21,
0x04,0x20,0x08,0x10,0x10,0x08,0xE0,0x07,0x10,0x08,0x08,0x10,0x04,0x20,0xFE,0x7F},//4观看

{0x00,0x00,0x00,0x00,0xC0,0x00,0xC0,0x01,0xE0,0x01,0xE0,0x01,0xF7,0x00,0xF7,0x7F,
0xF7,0x7F,0xF7,0x3F,0xF7,0x3F,0xF7,0x1F,0xF7,0x0F,0xF7,0x07,0x00,0x00,0x00,0x00},//5点赞
};                                                   
const unsigned char AirKiss[] U8X8_PROGMEM ={//二维码
0x00,0x00,0x00,0x00,0x00,0xFE,0x90,0x08,0xF8,0x03,0x86,0xCA,0xC6,0x08,0x03,0xBA,
0x7E,0xBE,0xE8,0x02,0xBA,0x2A,0x11,0xEB,0x02,0xBA,0x54,0x49,0xEA,0x02,0x82,0xFC,
0xAE,0x08,0x02,0xFE,0xAA,0xAA,0xFA,0x03,0x00,0x8A,0x00,0x01,0x00,0x82,0xFA,0x85,
0xCE,0x01,0x54,0xAB,0xBB,0xF7,0x01,0xA6,0x39,0x4B,0xA0,0x01,0x76,0x9E,0x90,0xFE,
0x03,0x98,0xF9,0xFE,0x1A,0x00,0x06,0xAD,0xE6,0xD9,0x03,0x94,0xB9,0x54,0x64,0x01,
0x62,0xE6,0x91,0xD6,0x02,0xFE,0xF4,0xB4,0x37,0x02,0x4A,0x9F,0x7F,0xCD,0x02,0xF6,
0xDF,0x13,0xC1,0x03,0x7E,0x2C,0x6C,0xFE,0x00,0xB0,0x00,0xE3,0x76,0x01,0x2A,0x8A,
0xEE,0x21,0x00,0xB6,0xB5,0x63,0xCA,0x01,0x32,0x3B,0x80,0xE1,0x00,0xEE,0x95,0x39,
0x7F,0x00,0x00,0x66,0x3B,0xA2,0x02,0xFE,0x4C,0x6B,0xAB,0x01,0x82,0xB4,0x32,0xA3,
0x02,0xBA,0xC0,0x59,0x7E,0x03,0xBA,0x94,0xE6,0xE0,0x02,0xBA,0xA0,0x22,0x74,0x03,
0x86,0xAA,0xB3,0xEC,0x00,0xFE,0xBA,0x94,0x3B,0x01,0x00,0x00,0x00,0x00,0x00,
};

/*************************************************************************************
 * 使用取模软件：PCTOLCD 2002完美版 
 * 取模方式为：阴码，逐行式，逆向（低位在前）  
 * 字体：32X32 仿宋
 * 图标：32X32
**************************************************************************************/
static const unsigned char Weather_tubiao_index[38]={0,0,0,0,1,2,2,3,3,4,5,6,7,8,9,10,10,10,10,11,12,13,13,14,15,15,16,16,16,16,17,18,19,19,20,20,20,21};
static const unsigned char Weather_tubiao[][128] U8X8_PROGMEM={//心知天气 天气图标
  {0x00,0x80,0x01,0x00,0x00,0x80,0x01,0x00,0x00,0x80,0x01,0x00,0x00,0x80,0x01,0x00,
0x00,0x00,0x00,0x00,0x60,0x00,0x00,0x0C,0xE0,0x00,0x00,0x0E,0xC0,0x01,0x00,0x07,
0x80,0xE1,0x07,0x03,0x00,0xF8,0x1F,0x00,0x00,0xFC,0x3F,0x00,0x00,0xFE,0x7F,0x00,
0x00,0xFE,0x7F,0x00,0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x0F,0xFF,0xFF,0xF0,
0x0F,0xFF,0xFF,0xF0,0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0xFE,0x7F,0x00,
0x00,0xFE,0x7F,0x00,0x00,0xFC,0x3F,0x00,0x00,0xF8,0x1F,0x00,0x00,0xE0,0x07,0x00,
0x80,0x01,0x00,0x03,0xC0,0x01,0x00,0x07,0xE0,0x00,0x00,0x0E,0x60,0x00,0x00,0x0C,
0x00,0x80,0x01,0x00,0x00,0x80,0x01,0x00,0x00,0x80,0x01,0x00,0x00,0x80,0x01,0x00,},//0 0-3晴
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0x00,0x30,0x0C,0x00,0x00,0x08,0x10,0x00,0x00,0x04,0x20,0x00,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x1C,0x20,0x08,0x00,0x22,0x10,
0xF0,0xFF,0xC1,0x0F,0x00,0x80,0x80,0x00,0x00,0x78,0x00,0x0F,0x00,0x04,0x00,0x10,
0x10,0x02,0x00,0x20,0x28,0x02,0x00,0x20,0x6C,0x02,0x00,0x20,0x82,0x04,0x00,0x10,
0x82,0xF8,0xFF,0x0F,0x7C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//1 4多云
  {0x00,0x00,0x40,0x00,0x00,0x00,0x41,0x10,0x00,0x00,0x02,0x08,0x00,0x00,0xF0,0x01,
0x00,0x00,0x08,0x02,0x00,0x00,0x04,0x04,0x00,0x00,0x04,0x04,0x00,0xC0,0x47,0x34,
0x00,0x30,0x0C,0x04,0x00,0x08,0x10,0x04,0x00,0x04,0x20,0x02,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//2 5-6晴间多云
  {0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x01,0x00,0x00,0x08,0x02,0x00,0x00,0x04,0x04,
0x00,0x00,0x02,0x08,0x00,0x00,0x02,0x08,0x00,0x00,0x02,0x08,0x00,0xC0,0x07,0x08,
0x00,0x30,0x08,0x08,0x00,0x08,0x10,0x04,0x00,0x04,0x20,0x02,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//3 7-8大部多云
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0x00,0x30,0x0C,0x00,0x00,0x08,0x10,0x00,0x00,0x04,0x20,0x00,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//4 9阴
  {0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x01,0x00,0x00,0x08,0x02,0x00,0x00,0x04,0x04,
0x00,0x00,0x02,0x08,0x00,0x00,0x02,0x08,0x00,0x00,0x02,0x08,0x00,0xC0,0x07,0x08,
0x00,0x30,0x08,0x08,0x00,0x08,0x10,0x04,0x00,0x04,0x20,0x02,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x01,0x04,
0x20,0x00,0x01,0x04,0x30,0x80,0x01,0x06,0x18,0xC0,0x00,0x03,0x08,0x40,0x00,0x01,
0x0C,0x60,0x80,0x01,0x04,0x20,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//5 10阵雨
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0x00,0x30,0x0C,0x00,0x00,0x08,0x10,0x00,0x00,0x04,0x20,0x00,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0xC0,0x03,0x04,
0x20,0xE0,0x01,0x04,0x30,0xF0,0x00,0x06,0x18,0xF8,0x07,0x03,0x08,0x80,0x03,0x01,
0x0C,0xC0,0x81,0x01,0x04,0x60,0x80,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,},//6 11雷阵雨
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0x00,0x30,0x0C,0x00,0x00,0x08,0x10,0x00,0x00,0x04,0x20,0x00,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0xA8,0xE0,0x41,0x05,0x70,0xF0,0x80,0x03,0xF8,0xF8,0xC7,0x07,0x70,0x80,0x83,0x03,
0xA8,0xC0,0x41,0x05,0x00,0x60,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,},//7 12雷阵雨伴有冰雹
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0x00,0x30,0x0C,0x00,0x00,0x08,0x10,0x00,0x00,0x04,0x20,0x00,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x01,0x04,
0x20,0x00,0x01,0x04,0x30,0x80,0x01,0x06,0x18,0xC0,0x00,0x03,0x08,0x40,0x00,0x01,
0x0C,0x60,0x80,0x01,0x04,0x20,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//8 13小雨
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0x00,0x30,0x0C,0x00,0x00,0x08,0x10,0x00,0x00,0x04,0x20,0x00,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x10,0x08,0x04,
0x20,0x10,0x08,0x04,0x30,0x18,0x0C,0x06,0x18,0x0C,0x06,0x03,0x08,0x04,0x02,0x01,
0x0C,0x06,0x83,0x01,0x04,0x02,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//9 14中雨
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0x00,0x30,0x0C,0x00,0x00,0x08,0x10,0x00,0x00,0x04,0x20,0x00,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x08,0x21,0x04,
0x40,0x08,0x21,0x04,0x60,0x8C,0x31,0x06,0x30,0xC6,0x18,0x03,0x10,0x42,0x08,0x01,
0x18,0x63,0x8C,0x01,0x08,0x21,0x84,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//10 15-18大雨暴雨大暴雨特大暴雨
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0x00,0x30,0x0C,0x00,0x00,0x08,0x10,0x00,0x00,0x04,0x20,0x00,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x01,0x04,
0xE0,0x80,0x03,0x0E,0x40,0x00,0x01,0x04,0x00,0x00,0x00,0x00,0x00,0x08,0x20,0x00,
0x00,0x1C,0x70,0x00,0x00,0x08,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//11 19冻雨
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0x00,0x30,0x0C,0x00,0x00,0x08,0x10,0x00,0x00,0x04,0x20,0x00,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x01,0x00,
0x20,0x00,0x81,0x0A,0x30,0x80,0x01,0x07,0x18,0xC0,0x80,0x0F,0x08,0x40,0x00,0x07,
0x0C,0x60,0x80,0x0A,0x04,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//12 20雨夹雪
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0x00,0x30,0x0C,0x00,0x00,0x08,0x10,0x00,0x00,0x04,0x20,0x00,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x15,0x54,0x00,0x00,0x0E,0x38,0x00,0x00,0x1F,0x7C,0x00,0x00,0x0E,0x38,0x00,
0x00,0x15,0x54,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//13 21-22小雪
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0x00,0x30,0x0C,0x00,0x00,0x08,0x10,0x00,0x00,0x04,0x20,0x00,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xA8,0xA0,0x82,0x0A,0x70,0xC0,0x01,0x07,0xF8,0xE0,0x83,0x0F,0x70,0xC0,0x01,0x07,
0xA8,0xA0,0x82,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//14 23中雪
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,
0x00,0x30,0x0C,0x00,0x00,0x08,0x10,0x00,0x00,0x04,0x20,0x00,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x2A,0x2A,0x2A,0x2A,0x1C,0x1C,0x1C,0x1C,0x3E,0x3E,0x3E,0x3E,0x1C,0x1C,0x1C,0x1C,
0x2A,0x2A,0x2A,0x2A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//15 24-25大雪
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xD8,0xB6,0x39,0x00,
0xD8,0xB6,0x45,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,
0x33,0xFF,0x47,0x00,0x33,0xFF,0x3B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x66,0xE6,0xBF,0x03,0x66,0xE6,0x7F,0x04,
0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x08,0xB0,0x6D,0x5B,0x04,
0xB0,0x6D,0x9B,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//16 26-29沙尘暴
  {0x00,0x00,0x40,0x00,0x00,0x00,0x41,0x10,0x00,0x00,0x02,0x08,0x00,0x00,0xF0,0x01,
0x00,0x00,0x08,0x02,0x00,0x00,0x04,0x04,0x00,0x00,0x04,0x04,0x00,0xC0,0x07,0x34,
0x00,0x30,0x0C,0x04,0x00,0x08,0x10,0x04,0x00,0x04,0x20,0x02,0xF0,0x03,0xC0,0x0F,
0x08,0x00,0x00,0x10,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,
0x02,0x00,0x00,0x40,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,
0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x3F,0xFC,0x0F,
0xF0,0x3F,0xFC,0x0F,0x00,0x00,0x00,0x00,0xF0,0x3F,0xFC,0x0F,0xF0,0x3F,0xFC,0x0F,
0x00,0x00,0x00,0x00,0xF0,0x3F,0xFC,0x0F,0xF0,0x3F,0xFC,0x0F,0x00,0x00,0x00,0x00,},//17 30雾
  {0x00,0x00,0x00,0x00,0x06,0x80,0x01,0x60,0x06,0x80,0x01,0x60,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x0F,0xF0,0x0F,0xF8,0x1F,0xF8,0x1F,
0x1C,0x38,0x1C,0x38,0x0C,0x60,0x06,0x30,0x06,0xC0,0x03,0x60,0x06,0x80,0x01,0x60,
0x06,0x80,0x01,0x60,0x06,0xC0,0x03,0x60,0x0C,0x60,0x06,0x30,0x1C,0x38,0x1C,0x38,
0xF8,0x1F,0xF8,0x1F,0xF0,0x0F,0xF0,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x06,0x80,0x01,0x60,0x06,0x80,0x01,0x60,0x00,0x00,0x00,0x00,},//18 31霾
  {0x00,0x00,0x60,0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0xF0,0x00,0x00,0x80,0x60,0x00,
0x00,0x7C,0x61,0x00,0xC0,0x43,0x61,0x00,0x38,0x22,0x66,0x00,0x24,0x21,0x6A,0x00,
0x24,0x21,0x72,0x00,0x24,0x21,0x6A,0x00,0x38,0x22,0x66,0x00,0xC0,0x43,0x61,0x00,
0x00,0x7C,0x61,0x00,0x00,0x80,0x60,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x60,0x00,
0x00,0x00,0x60,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x60,0x00,
0x00,0x00,0x60,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x60,0x00,
0x00,0x00,0x60,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x60,0x00,
0x00,0x00,0xFC,0x03,0x00,0x00,0xFC,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//19 32-33风
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xF0,0xFF,0xFF,0x0F,0xF0,0xFF,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0xFC,0xFF,0x0F,0x00,0xFC,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xC0,0xFF,0xFF,0x00,0xC0,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0xFC,0xFF,0x0F,0x00,0xFC,0xFF,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0xC0,0xFF,0x01,0x00,0xC0,0xFF,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x3F,0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x07,0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//20 34-36暴风
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0E,0x0E,0x08,0x02,0x1E,0x0E,0x0C,0x07,
0x1E,0x0E,0x0C,0x07,0x3E,0x0E,0x86,0x0D,0x2E,0x0E,0x86,0x0D,0x6E,0x0E,0xC3,0x18,
0x4E,0x0E,0xC3,0x18,0xCE,0x8E,0xE1,0x3F,0x8E,0x8E,0xE1,0x3F,0x8E,0xCF,0x30,0x60,
0x0E,0xCF,0x30,0x60,0x0E,0x6F,0x18,0xC0,0x0E,0x6E,0x18,0xC0,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},//21 99未知
};


static const unsigned char HZ16x16[][32] U8X8_PROGMEM={
// 今明后天晴多云阴雷阵小中大暴雨雪冰雹沙尘雾霾风冻微未知周日一二三四五六
//  (0) 今(1) 明(2) 后(3) 天(4) 晴(5) 多(6) 云(7) 阴(8) 雷(9) 阵(10) 小(11) 中(12) 大(13) 暴(14) 雨(15)
// 雪(16) 冰(17) 雹(18) 沙(19) 尘(20) 雾(21) 霾(22) 风(23) 冻(24) 微(25) 未(26) 知(27) 周(28) 日(29) 一(30) 二(31)
// 三(32) 四(33) 五(34) 六(35)
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},/*" ",0*/

{0x00,0x00,0x80,0x00,0x80,0x00,0xC0,0x01,0x40,0x02,0x20,0x04,0x30,0x08,0x90,0x39,
0x08,0x70,0x06,0x03,0xF0,0x02,0x00,0x02,0x00,0x01,0x00,0x01,0x80,0x00,0x00,0x00},/*"今",1*/

{0x00,0x00,0x00,0x00,0x00,0x3D,0x20,0x33,0x5C,0x21,0x44,0x1E,0x58,0x11,0x44,0x11,
0x44,0x2F,0x7C,0x21,0x04,0x21,0x80,0x20,0x80,0x30,0x40,0x18,0x20,0x10,0x00,0x00},/*"明",2*/

{0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x03,0xF0,0x00,0x30,0x00,0x10,0x3C,0xF0,0x03,
0x10,0x00,0x10,0x1C,0xE8,0x1B,0x28,0x08,0x44,0x08,0xC2,0x0F,0x00,0x00,0x00,0x00},/*"后",3*/

{0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x03,0x80,0x00,0x80,0x00,0x80,0x0E,0xF8,0x01,
0x80,0x00,0xC0,0x00,0x40,0x01,0x20,0x02,0x10,0x0C,0x0C,0x38,0x00,0x00,0x00,0x00},/*"天",4*/

{0x00,0x04,0x00,0x04,0xDE,0x7F,0x12,0x04,0x92,0x3F,0x12,0x04,0xD2,0x7F,0x1E,0x00,
0x92,0x3F,0x92,0x20,0x92,0x3F,0x92,0x20,0x9E,0x3F,0x92,0x20,0x80,0x28,0x80,0x10},/*"晴",5*/

{0x40,0x00,0x40,0x00,0xE0,0x0F,0x10,0x04,0x1C,0x02,0x20,0x01,0xC0,0x02,0x30,0x01,
0x8E,0x1F,0x40,0x10,0x30,0x08,0x4C,0x04,0x80,0x02,0x80,0x01,0x70,0x00,0x0E,0x00},/*"多",6*/

{0x00,0x00,0xFC,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x7F,0x40,0x00,
0x20,0x00,0x20,0x00,0x10,0x02,0x08,0x04,0x04,0x08,0xFE,0x1F,0x04,0x10,0x00,0x10},/*"云",7*/

{0x00,0x00,0xBE,0x3F,0xA2,0x20,0x92,0x20,0x92,0x20,0x8A,0x3F,0x92,0x20,0x92,0x20,
0xA2,0x20,0xA2,0x3F,0xA2,0x20,0x96,0x20,0x4A,0x20,0x42,0x20,0x22,0x28,0x12,0x10},/*"阴",8*/

{0x00,0x00,0xFC,0x1F,0x80,0x00,0xFE,0x7F,0x82,0x40,0xB9,0x2E,0x80,0x00,0xB8,0x0E,
0x00,0x00,0xFC,0x1F,0x84,0x10,0x84,0x10,0xFC,0x1F,0x84,0x10,0x84,0x10,0xFC,0x1F},/*"雷",9*/

{0x00,0x02,0x3E,0x02,0x22,0x02,0xD2,0x7F,0x12,0x01,0x0A,0x05,0x92,0x04,0x92,0x3F,
0x22,0x04,0x22,0x04,0x22,0x04,0xD6,0x7F,0x0A,0x04,0x02,0x04,0x02,0x04,0x02,0x04},/*"阵",10*/

{0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x88,0x08,0x88,0x10,0x88,0x20,
0x84,0x20,0x84,0x40,0x82,0x40,0x81,0x40,0x80,0x00,0x80,0x00,0xA0,0x00,0x40,0x00},/*"小",11*/

{0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0xFC,0x1F,0x84,0x10,0x84,0x10,0x84,0x10,
0x84,0x10,0x84,0x10,0xFC,0x1F,0x84,0x10,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00},/*"中",12*/

{0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0xFF,0x7F,0x80,0x00,0x80,0x00,
0x40,0x01,0x40,0x01,0x20,0x02,0x20,0x02,0x10,0x04,0x08,0x08,0x04,0x10,0x03,0x60},/*"大",13*/

{0xF8,0x0F,0x08,0x08,0xF8,0x0F,0x08,0x08,0xF8,0x0F,0x10,0x04,0xFE,0x3F,0x10,0x04,
0xFF,0x7F,0x10,0x04,0x8C,0x18,0x93,0x64,0xA0,0x02,0x90,0x04,0xA8,0x08,0x44,0x10},/*"暴",14*/

{0x00,0x00,0xFF,0x7F,0x80,0x00,0x80,0x00,0x80,0x00,0xFE,0x3F,0x82,0x20,0x82,0x20,
0x92,0x22,0xA2,0x24,0x82,0x20,0x92,0x22,0xA2,0x24,0x82,0x20,0x82,0x28,0x02,0x10},/*"雨",15*/

{0xFC,0x1F,0x80,0x00,0xFE,0x7F,0x82,0x40,0xB9,0x2E,0x80,0x00,0xB8,0x0E,0x00,0x00,
0xFC,0x1F,0x00,0x10,0x00,0x10,0xF8,0x1F,0x00,0x10,0x00,0x10,0xFC,0x1F,0x00,0x10},/*"雪",16*/

{0x00,0x02,0x02,0x02,0x04,0x02,0x04,0x22,0x00,0x16,0xE0,0x0E,0x88,0x06,0x88,0x0A,
0x84,0x0A,0x47,0x12,0x44,0x12,0x24,0x22,0x14,0x42,0x04,0x02,0x84,0x02,0x00,0x01},/*"冰",17*/

{0xFC,0x1F,0x80,0x00,0xFE,0x7F,0x82,0x40,0xB9,0x2E,0x80,0x00,0xB8,0x0E,0x10,0x00,
0xF8,0x0F,0x04,0x08,0xFA,0x09,0x08,0x09,0xF8,0x0B,0x08,0x04,0x08,0x20,0xF0,0x3F},/*"雹",18*/

{0x00,0x02,0x04,0x02,0x08,0x02,0x88,0x12,0x81,0x22,0x42,0x42,0x42,0x42,0x28,0x12,
0x08,0x12,0x04,0x12,0x07,0x08,0x04,0x08,0x04,0x04,0x04,0x02,0x84,0x01,0x60,0x00},/*"沙",19*/

{0x80,0x00,0x80,0x00,0x90,0x04,0x90,0x08,0x88,0x10,0x84,0x20,0x82,0x20,0x00,0x00,
0x80,0x00,0x80,0x00,0xFC,0x1F,0x80,0x00,0x80,0x00,0x80,0x00,0xFF,0x7F,0x00,0x00},/*"尘",20*/

{0xFC,0x1F,0x80,0x00,0xFE,0x7F,0x82,0x40,0xB9,0x2E,0x80,0x00,0xB8,0x0E,0x20,0x00,
0xF0,0x07,0x28,0x02,0xC0,0x01,0x38,0x0E,0x47,0x70,0xF0,0x07,0x20,0x04,0x18,0x06},/*"雾",21*/

{0xFC,0x1F,0x80,0x00,0xFE,0x7F,0x82,0x40,0xB9,0x2E,0x0C,0x00,0xD3,0x3F,0xB4,0x24,
0x8C,0x3F,0x93,0x24,0xA8,0x3F,0x26,0x04,0xB0,0x3F,0x2C,0x04,0xA3,0x7F,0x18,0x00},/*"霾",22*/

{0x00,0x00,0xFC,0x0F,0x04,0x08,0x04,0x08,0x14,0x0A,0x24,0x0A,0x44,0x09,0x44,0x09,
0x84,0x08,0x84,0x08,0x44,0x09,0x44,0x49,0x24,0x52,0x12,0x52,0x02,0x60,0x01,0x40},/*"风",23*/

{0x00,0x02,0x02,0x02,0x04,0x02,0xE4,0x7F,0x00,0x01,0x90,0x04,0x90,0x04,0x48,0x04,
0xC8,0x3F,0x07,0x04,0x84,0x14,0x84,0x24,0x44,0x44,0x24,0x44,0x04,0x05,0x00,0x02},/*"冻",24*/

{0x88,0x10,0xA8,0x12,0xA4,0x12,0xA2,0x0A,0xE9,0x7B,0x08,0x24,0x04,0x28,0xF6,0x2B,
0x05,0x28,0xE4,0x29,0x24,0x29,0x24,0x15,0x24,0x13,0x24,0x29,0x14,0x28,0x0C,0x44},/*"微",25*/

{0x80,0x00,0x80,0x00,0x80,0x00,0xFC,0x1F,0x80,0x00,0x80,0x00,0x80,0x00,0xFF,0x7F,
0xC0,0x01,0xA0,0x02,0x90,0x04,0x88,0x08,0x84,0x10,0x83,0x60,0x80,0x00,0x80,0x00},/*"未",26*/

{0x04,0x00,0x04,0x00,0x04,0x3E,0x7E,0x22,0x12,0x22,0x11,0x22,0x10,0x22,0x10,0x22,
0xFF,0x22,0x10,0x22,0x28,0x22,0x28,0x22,0x44,0x3E,0x44,0x22,0x42,0x00,0x01,0x00},/*"知",27*/

{0x00,0x00,0x00,0x00,0x80,0x1F,0x70,0x11,0x10,0x11,0xD0,0x13,0x10,0x11,0xF0,0x13,
0x10,0x12,0xD0,0x15,0x48,0x12,0xC8,0x13,0x48,0x10,0x04,0x1C,0x02,0x18,0x00,0x00},/*"周",28*/

{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xF0,0x0C,0x10,0x04,0x20,0x04,0xA0,0x05,
0x60,0x04,0x10,0x04,0x10,0x04,0x90,0x05,0x60,0x06,0x00,0x04,0x00,0x00,0x00,0x00},/*"日",29*/

{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7E,
0xFE,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},/*"一",30*/

{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xE0,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x30,0xF8,0x7F,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00},/*"二",31*/

{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xE0,0x01,0x00,0x00,0x00,0x00,0x00,0x06,
0xE0,0x01,0x00,0x00,0x00,0x00,0x00,0x38,0xFC,0x67,0x00,0x00,0x00,0x00,0x00,0x00},/*"三",32*/

{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0xC0,0x3F,0x7C,0x21,0x44,0x21,0x24,0x21,
0x28,0x1E,0x18,0x10,0x08,0x16,0xF8,0x19,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00},/*"四",33*/

{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xE0,0x00,0x80,0x00,0x80,0x00,0x80,0x06,
0x70,0x0D,0x40,0x04,0x40,0x04,0x20,0x02,0xE0,0x7F,0x1E,0x00,0x00,0x00,0x00,0x00},/*"五",34*/

{0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x01,0x00,0x00,0x00,0x7E,0xFC,0x01,
0x00,0x00,0x00,0x02,0x60,0x0C,0x30,0x18,0x10,0x10,0x0C,0x00,0x00,0x00,0x00,0x00},/*"六",35*/
};
//   (0) 今(1) 明(2) 后(3) 天(4) 晴(5) 多(6) 云(7) 阴(8) 雷(9) 阵(10) 小(11) 中(12) 大(13) 暴(14) 雨(15)
//雪(16) 冰(17) 雹(18) 沙(19) 尘(20) 雾(21) 霾(22) 风(23) 冻(24) 微(25) 未(26) 知(27)
static const unsigned char HZcode_index[38][2]={
    {5,0},{5,0},{5,0},{5,0},//0-3晴
    {6,7},{6,7},{6,7},{6,7},{6,7},//4-8多云
    {8,0},// 9阴天
    {10,15},//10阵雨
    {9,15},//11雷雨
    {17,18},//12冰雹
    {11,15},//13小雨
    {12,15},//14中雨
    {13,15},//15大雨
    {14,15},{14,15},{14,15},//16-18暴雨
    {24,15},//19冻雨
    {15,16},//20雨雪
    {10,16},//21阵雪
    {11,16},//22小雪
    {12,16},//23中雪
    {13,16},//24大雪
    {14,16},//25暴雪
    {19,20},{19,20},{19,20},{19,20},//26-29沙尘
    {21,0},//30雾
    {22,0},//31霾
    {25,23},//32微风
    {13,23},//33大风
    {14,23},//34-36暴风
    {26,27},//37未知
    };
static const unsigned char pic08x16[][16] U8X8_PROGMEM={
{0x00,0x02,0x05,0x05,0x02,0x38,0x44,0x04,0x04,0x04,0x04,0x04,0x04,0x44,0x38,0x00},//℃  
};


unsigned char butterfly_computation(unsigned char data)
{
         data = (data << 4) | (data >> 4);
         data = ((data << 2)&0xcc) | ((data >> 2)&0x33);
         data = ((data << 1)&0xaa) | ((data >> 1)&0x55);

         return data;
}

String response;
String responseview;
int follower = 0;
int view = 0;
const int slaveSelect = 5;
const int scanLimit = 7;

int times = 0;
int cycle = 0;

int gun=0;
int h1=14;//首行的y
int h2=61;//末行的y

int years, months, days, hours, minutes, seconds=0, weekdays;

//-------------------------------------setup()-------------------------------------------------
void setup()
{
  //pinMode(buzzer, OUTPUT);
  pinMode(fun1, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  //pinMode(fun1, OUTPUT);

  pinMode(fun2, INPUT_PULLUP);

  Serial.begin(115200);
  while (!Serial)
    continue;
  Serial.println("bilibili fans NTP Clock version 1.0");
  Serial.println("by BL");
  initdisplay();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  u8g2.print("Connect to WiFi");
  u8g2.setCursor(0, 30);
  u8g2.printf("SSID:%s\r\n", WiFi.SSID().c_str());
  u8g2.setCursor(0, 47);
  u8g2.printf("PSWD:%s\r\n", WiFi.psk().c_str());
  u8g2.setCursor(0, 64);
  //u8g2.print("fans Clock v1.0");
  u8g2.sendBuffer();
  
  if(WifiAutoConfig()== false){      
      WifiSmartConfig();
  }

  u8g2.setCursor(0, 64);
  u8g2.printf("IP:");
  u8g2.setCursor(24, 64);
  u8g2.print(WiFi.localIP());
  u8g2.sendBuffer();


  

  /*
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  u8g2.print("Waiting for WiFi");
  u8g2.setCursor(0, 30);
  u8g2.print("connection...");
  u8g2.setCursor(0, 47);
  u8g2.print("By BL bilibili");
  u8g2.setCursor(0, 64);
  u8g2.print("fans Clock v1.0");
  u8g2.sendBuffer();

  Serial.print("Connecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
  }
  */
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(5); 
  isNTPConnected = true;
  mqttClient.setCallback(mqtt_callback);
  getJson();
}

//-------------------------------------loop()-------------------------------------------------

void loop()
{
  button1State = digitalRead(fun1);//右键
  button2State = digitalRead(fun2);//左键

    if (millis() - lastMs >= 200)  //5S
    {
        lastMs = millis();
        mqtt_check_connect();
        /* Post */        
        mqtt_interval_post();
    }

    mqttClient.loop();
  
    if ( button2State == LOW && button2State_old == HIGH)       //左键按下
  {
    if(cycle < 3 )
    {
      cycle ++;                                //翻页
      if (cycle == 3)
      {cycle = 0;}
      u8g2.clearBuffer();
    }
  }
  button2State_old = button2State;
 
     
  if (button2State == HIGH)       //左键释放
  {


  }

  if ( button1State == LOW && button1State_old == HIGH )       //右键按下
  {
    //digitalWrite(buzzer, HIGH);
    //delay(30);
    //digitalWrite(buzzer, LOW);
  }
     button1State_old = button1State;

  if (button1State == HIGH)       //右键释放
  {
    //digitalWrite(buzzer, LOW);
  }

  if (timeStatus() != timeNotSet)
    {
        oledClockDisplay();
    }
  //delay(200); 

}




bool getJson()
{
    bool r = false;
    http.setTimeout(HTTP_TIMEOUT);

    //followe
    http.begin("http://api.bilibili.com/x/relation/stat?vmid=" + biliuid);
    int httpCode = http.GET();
    if (httpCode > 0){
        if (httpCode == HTTP_CODE_OK){
            response = http.getString();
            //Serial.println(response);
            r = true;
        }
    }else{
        //Serial.printf("[HTTP] GET JSON failed, error: %s\n", http.errorToString(httpCode).c_str());
        r = false;
    }
    http.end();
    //return r;

    //view
    http.begin("http://api.bilibili.com/x/space/upstat?mid=" + biliuid);
    int httpCodeview = http.GET();
    if (httpCodeview > 0){
        if (httpCodeview == HTTP_CODE_OK){
            responseview = http.getString();
            //Serial.println(responseview);
            r = true;
        }
    }else{
        Serial.printf("[HTTP] GET JSON failed, error: %s\n", http.errorToString(httpCode).c_str());
        r = false;
    }
    http.end();
    return r;
}

bool parseJson(String json)
{
    const size_t capacity = JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 70;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, json);

    int code = doc["code"];
    const char *message = doc["message"];

    if (code != 0){
        //Serial.print("[API]Code:");
        //Serial.print(code);
        //Serial.print(" Message:");
        //Serial.println(message);

        return false;
    }

    JsonObject data = doc["data"];
    unsigned long data_mid = data["mid"];
    int data_follower = data["follower"];
    if (data_mid == 0){
        Serial.println("[JSON] FORMAT ERROR");
        return false;
    }
    //Serial.print("UID: ");
    //Serial.print(data_mid);
    //Serial.print(" follower: ");
    //Serial.println(data_follower);

    follower = data_follower;
    return true;
}

void parseJson1(String json)
{
    const size_t capacity = JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 70;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, json);

    int code = doc["code"];
    const char *message = doc["message"];

    JsonObject data = doc["data"];
    JsonObject archive = data["archive"];
    int data_view = archive["view"];

    //Serial.print("vive: ");
    //Serial.println(data_view);

    view = data_view;
}



void initdisplay()
{
  u8g2.begin();
  u8g2.enableUTF8Print();
}

void oledClockDisplay()
{


///////////////////////////////////////////////////////////////////////////////////////////////////////cycle=0

  if(cycle == 0)
  {
  years = year();
  months = month();
  days = day();
  hours = hour();
  minutes = minute();
  seconds = second();
  weekdays = weekday();
  Ldays=GetDays(2019,8,4,years,months,days);
  Serial.printf("%d/%d/%d %d:%d:%d Weekday:%d\n", years, months, days, hours, minutes, seconds, weekdays);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  getJson();  //60s获取一次B站数据
    if(seconds%30 == 0)          //-------------------------------30s时切换上下
  {
     h1=14;
     h2=61;
     getJson();
  }
    if(seconds%60 == 0)          //-------------------------------60s时切换上下
  {
    h1=61;
    h2=14;
    getJson();
 
  }


  if(   hours%7 ==0 && minutes%60 ==0&& seconds%60 ==0                  //整点报时
     || hours%8 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%9 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%10 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%11 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%12 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%13 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%14 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%15 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%16 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%17 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%18 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%19 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%20 ==0 && minutes%60 ==0 && seconds%60 ==0
     || hours%21 ==0 && minutes%60 ==0 && seconds%60 ==0
  )
{
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
}



  if (isNTPConnected)
  {
    //u8g2.print("当前时间 (UTC+8)");
      if (WiFi.status() == WL_CONNECTED)
      {
         //if (getJson())
         //{
            if (parseJson(response))
            {
                  parseJson1(responseview);
                  u8g2.setCursor(0, h1);
                  u8g2.print("F:");
                  ///u8g2.println(Ldays);
                  u8g2.println(follower);
                  u8g2.setCursor(64, h1 );
                  u8g2.print("V:");
                  u8g2.println(view);
             } 
         //}
        
      }
  }
  else
  {
    u8g2.setCursor(0, h1);
    u8g2.setFont(u8g2_font_unifont_t_chinese2);
    u8g2.print("无网络!"); //如果上次对时失败，则会显示无网络
  }
  String currentTime = "";
  if (hours < 10)
    currentTime += 0;
  currentTime += hours;
  currentTime += ":";
  if (minutes < 10)
    currentTime += 0;
  currentTime += minutes;
  currentTime += ":";
  if (seconds < 10)
    currentTime += 0;
  currentTime += seconds;
  String currentDay = "";
  currentDay += years;
  currentDay += "/";
  if (months < 10)
    currentDay += 0;
  currentDay += months;
  currentDay += "/";
  if (days < 10)
    currentDay += 0;
  currentDay += days;

  u8g2.setFont(u8g2_font_logisoso24_tr);
  u8g2.setCursor(6, 44);
  u8g2.print(currentTime);
  u8g2.setCursor(0, h2);
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.print(currentDay);
  u8g2.drawXBM(80, h2-13, 16, 16, xing);
  u8g2.setCursor(95, h2+1);
  u8g2.print("期");
  if (weekdays == 1)
    u8g2.print("日");
  else if (weekdays == 2)
    u8g2.print("一");
  else if (weekdays == 3)
    u8g2.print("二");
  else if (weekdays == 4)
    u8g2.print("三");
  else if (weekdays == 5)
    u8g2.print("四");
  else if (weekdays == 6)
    u8g2.print("五");
  else if (weekdays == 7)
    u8g2.drawXBM(111, h2-13, 16, 16, liu);
  u8g2.sendBuffer();
  
  }

///////////////////////////////////////////////////////////////////////////////////////////////////////cycle=1


  if(cycle == 1)
  {
    
  key_process();
  CurrentTime_update();
  Time_Show();
  
  if(CurrentTime.Second==15 ||CurrentTime.Second==30 ||CurrentTime.Second==45 ||CurrentTime.Second==60  ){//更新天气
      if((f5>26)&&WeatherFlag)
      {
        GetWeatherJson();
        WeatherFlag = false;
       }

    }
  else WeatherFlag=true;

  
  if(CurrentTime.Second==45)
  {//更新bili
           if((f5>26)&&FansFlag)
           { 
        //getBiliJson();
               FansFlag =false;
           }
  }
  else FansFlag=true;

     if(
       CurrentTime.Hour == 7 && CurrentTime.Minute == 0 && CurrentTime.Second == 0    //-------整点提示
    || CurrentTime.Hour == 8 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 9 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 10 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 11 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 12 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 13 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 14 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 15 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 16 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 17 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 18 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 19 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 20 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
    || CurrentTime.Hour == 21 && CurrentTime.Minute == 0 && CurrentTime.Second == 0
     
     )
  {
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
  }

  }

///////////////////////////////////////////////////////////////////////////////////////////////////////cycle=2
if(cycle == 2)
 {
  years = year();
  months = month();
  days = day();
  hours = hour();
  minutes = minute();
  seconds = second();
  weekdays = weekday();
  Ldays=GetDays(2019,8,4,years,months,days);

       //getJson();
       if (WiFi.status() == WL_CONNECTED)
       {  
        if (isNTPConnected)
          { 
        //if (getJson())
            //if (parseJson(response))
            //{
                  parseJson1(responseview);
                  u8g2.clearBuffer();
                  u8g2.setFont(u8g2_font_unifont_t_chinese2);
                  u8g2.drawXBMP( 52 , 0 , 24 , 24 , bilibilitv_24u );     //根据你的图片尺寸修改
                  u8g2.setCursor(0, 40);
                  //u8g2.print("");
                  u8g2.println("We get together ");
                  u8g2.setFont(u8g2_font_unifont_t_chinese2);
                  u8g2.setCursor(0, 55);
                  u8g2.println("already");
                  u8g2.setCursor(63, 55);
                  u8g2.println(Ldays);
                  u8g2.setCursor(95, 55);
                  u8g2.println("days");
                  u8g2.sendBuffer();
                  /*
                  u8g2.setFont(u8g2_font_unifont_t_chinese2);
                  u8g2.setCursor(0, 60);
                  u8g2.print("view:");
                  u8g2.setFont(u8g2_font_unifont_t_chinese2);
                  u8g2.setCursor(45, 60);
                  u8g2.println(view);
                  u8g2.sendBuffer();
                  */
            //}
           }
         }
    }
}

  



/*-------- NTP 代码 ----------*/

const int NTP_PACKET_SIZE = 48;     // NTP时间在消息的前48个字节里
byte packetBuffer[NTP_PACKET_SIZE]; // 输入输出包的缓冲区


time_t getNtpTime()
{
  IPAddress ntpServerIP;          // NTP服务器的地址

  while (Udp.parsePacket() > 0);  // 丢弃以前接收的任何数据包
  Serial.println("Transmit NTP Request");
  // 从池中获取随机服务器
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500)
  {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE)
    {
      Serial.println("Receive NTP Response");
      isNTPConnected = true;
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // 将数据包读取到缓冲区
      unsigned long secsSince1900;
      // 将从位置40开始的四个字节转换为长整型，只取前32位整数部分
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      Serial.println(secsSince1900);
      Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-("); //无NTP响应
  isNTPConnected = false;
  return 0; //如果未得到时间则返回0
}


// 向给定地址的时间服务器发送NTP请求
void sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123); //NTP需要使用的UDP端口号为123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

//-----------------------------------自动配网-----------------------------------------·
bool WifiAutoConfig()
{
  char count=30;

  WiFi.begin();
  Serial.println("\r\nAutoConfig Start");  
  Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
  Serial.printf("PSWD:%s\r\n", WiFi.psk().c_str());
  while (WiFi.status() != WL_CONNECTED)
    {         
      //WiFi.printDiag(Serial);//打印wifi连接信息
      //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));//led闪烁
      //oled 倒计时显示
      //u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setCursor(112, 47);
      u8g2.printf("%02d",count);
      u8g2.sendBuffer();
      //
      Serial.print("?");
      delay(1000);
      count--;
      if(count == 0)
      {
        Serial.println("\r\nAutoConfig fail!");
        //digitalWrite(LED_BUILTIN, HIGH);
        return false;
        break;
      }
    }
    Serial.println("\r\nAutoConfig succeed!");
    //digitalWrite(LED_BUILTIN, HIGH);
    return true;
}


void WifiSmartConfig()
 {
   //信息提示
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_unifont_t_chinese2);
      u8g2.setCursor(0, 14);
      u8g2.print("关注安信可科技");
      u8g2.setCursor(0, 30);
      u8g2.print("AirKiss");
      u8g2.setCursor(0, 47);
      u8g2.print("扫码配网");
      u8g2.drawXBM(64, 20, 40, 35, AirKiss);
      u8g2.sendBuffer();
   //
    WiFi.mode(WIFI_STA);            //设置WIFI模块为STA模式
    Serial.println("\r\nSmartConfig已开启,请微信关注 安信可科技 公众号进行WIFI配置");    
    WiFi.beginSmartConfig();        //smartconfig进行初始化
    while (1)                       //等待连接成功 ，如果未连接成功LED就每隔500ms闪烁
    {
      Serial.print(">");
      delay(1000);
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      //如果连接成功后就打印出连接的WIFI信息
      if (WiFi.smartConfigDone())
      {
        Serial.println("\r\nSmartConfig Success");
        Serial.printf("\r\nSSID:%s", WiFi.SSID().c_str());
        Serial.printf("\r\nPSWD:%s", WiFi.psk().c_str());     //打印出密码
        //WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str()); //开始连接WiFi
        WiFi.setAutoConnect(true);
         
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);          
          Serial.print(".");
          digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        }
        digitalWrite(LED_BUILTIN, HIGH);
        break;
      }
    }
 }

//----------------------------------天气时钟显示----------------------------------
 void Time_Show()
{
   if (millis() - LastShowTime >= 20 ) //如果和前次时间大于等于时间间隔
   {
       LastShowTime = millis();     
       if (timeStatus() != timeNotSet)
       {
          //u8g2.clearBuffer();
          if(ClockMode ==0)
              WeatherClockMirror();
          else
              WeatherRollClock();
         /**/       
       }
    }
}


//Free and open source 
/* 代码修改：冰凌影子BL   bilibili UID:177810096

//滚动时钟
void RollClock(unsigned char LCDX0,unsigned char LCDY0)
{ 
  if(Timetemp[0]!=CurrentTime.Hour/10 || RollTimeFlag) {//小时 十位
    f0=0;
    Timetemp[0]=CurrentTime.Hour/10;
    if(Timetemp[0]==0) timeroll[0] = 2;
    else timeroll[0] = Timetemp[0]-1;
  }
  if(f0<200) f0++;
  switch (Roll_mode)
  {
  case UP_Roll:
  case DOWN_Roll:
      if(f0<25)
      {
        LED_Rolling(hcbuff,(unsigned char *)num19x24[timeroll[0]],(unsigned char *)num19x24[Timetemp[0]],24,3,Roll_mode,f0);
        //UG_PutChar(0,0,5,8,hcbuff,1);
        u8g2.drawXBM(2+LCDX0, LCDY0, 19, 24, hcbuff);
      }
  break;
  default:
      if(f0<2) u8g2.drawXBM(2+LCDX0, LCDY0, 19, 24, num19x24[CurrentTime.Hour/10]);
  break;
  }

  if(Timetemp[1]!=CurrentTime.Hour%10 || RollTimeFlag) {//小时 个位
    f1=0;
    Timetemp[1]=CurrentTime.Hour%10;
    if(Timetemp[1]==0) timeroll[1] = 9;
    else timeroll[1] = Timetemp[1]-1;
  }
  if(f1<200) f1++;
  switch (Roll_mode)
  {
  case UP_Roll:
  case DOWN_Roll:
      if(f1<25)
      {
        LED_Rolling(hcbuff,(unsigned char *)num19x24[timeroll[1]],(unsigned char *)num19x24[Timetemp[1]],24,3,Roll_mode,f1);
        u8g2.drawXBM(24+LCDX0, LCDY0, 19, 24, hcbuff);
      }
  break;
  default:
      if(f1<2) u8g2.drawXBM(24+LCDX0, LCDY0, 19, 24, num19x24[CurrentTime.Hour%10]);
  break;
  }
  //----------------------
  if(Timetemp[2]!=CurrentTime.Minute/10 || RollTimeFlag) {
    f2=0;
    Timetemp[2]=CurrentTime.Minute/10;
    if(Timetemp[2]==0) timeroll[2] = 5;
    else timeroll[2] = Timetemp[2]-1;
  }
  if(f2<200) f2++;
  switch (Roll_mode)
  {
  case UP_Roll:
  case DOWN_Roll:
      if(f2<25)
      {
        LED_Rolling(hcbuff,(unsigned char *)num19x24[timeroll[2]],(unsigned char *)num19x24[Timetemp[2]],24,3,Roll_mode,f2);
        u8g2.drawXBM(57+LCDX0, LCDY0, 19, 24, hcbuff);
      }
  break;
  default:
      if(f2<2) u8g2.drawXBM(57+LCDX0, LCDY0, 19, 24, num19x24[CurrentTime.Minute/10]);
  break;
  }
  //-----------------------
  if(Timetemp[3]!=CurrentTime.Minute%10 || RollTimeFlag) {
    f3=0;
    Timetemp[3]=CurrentTime.Minute%10;
    if(Timetemp[3]==0) timeroll[3] = 9;
    else timeroll[3] = Timetemp[3]-1;
    Roll_mode = CurrentTime.Hour%3;
  }
  if(f3<200) f3++;
  switch (Roll_mode)
  {
  case UP_Roll:
  case DOWN_Roll:
      if(f3<25)
      {
        LED_Rolling(hcbuff,(unsigned char *)num19x24[timeroll[3]],(unsigned char *)num19x24[Timetemp[3]],24,3,Roll_mode,f3);
        u8g2.drawXBM(79+LCDX0, LCDY0, 19, 24, hcbuff);
      }
  break;
  default:
      if(f3<2) u8g2.drawXBM(79+LCDX0, LCDY0, 19, 24, num19x24[CurrentTime.Minute%10]);
  break;
  }
  //---------------
  if(Timetemp[4]!=CurrentTime.Second/10 || RollTimeFlag) {
    f4=0;
    Timetemp[4]=CurrentTime.Second/10;
    if(Timetemp[4]==0) timeroll[4] = 5;
    else timeroll[4] = Timetemp[4]-1;
  }
  if(f4<200) f4++;
  switch (Roll_mode)
  {
  case NO_Roll:
  case UP_Roll:
  case DOWN_Roll:
      if(f4<17)
      {
        LED_Rolling(hcbuff,(unsigned char *)num11x16[timeroll[4]],(unsigned char *)num11x16[Timetemp[4]],16,2,Roll_mode,f4);
        u8g2.drawXBM(102+LCDX0, LCDY0-4, 11, 16, hcbuff);
        shangxiajingxiang(hcbuff,16,2);
        u8g2.drawXBM(102+LCDX0, LCDY0+12, 11, 16, hcbuff);
      }
  break;
  default:
      if(f4<2) u8g2.drawXBM(102+LCDX0, LCDY0-4, 11, 16, num11x16[CurrentTime.Second/10]);
  break;
  }
  //--------
  if(Timetemp[5]!=CurrentTime.Second%10 || RollTimeFlag) {
    f5=0;
    Timetemp[5]=CurrentTime.Second%10;
    if(Timetemp[5]==0) timeroll[5] = 9;
    else timeroll[5] = Timetemp[5]-1;
    RollTimeFlag = false;
    //每秒钟处理的任务
    u8g2.drawXBM(2 +LCDX0, LCDY0, 19, 24, num19x24[CurrentTime.Hour/10]);
    u8g2.drawXBM(24+LCDX0, LCDY0, 19, 24, num19x24[CurrentTime.Hour%10]);
    u8g2.drawXBM(46+LCDX0, LCDY0, 8 , 24, TimeDot8x24[CurrentTime.Second%2]);//显示小时分钟间隔
    u8g2.drawXBM(57+LCDX0, LCDY0, 19, 24, num19x24[CurrentTime.Minute/10]);
    u8g2.drawXBM(79+LCDX0, LCDY0, 19, 24, num19x24[CurrentTime.Minute%10]);
    u8g2.drawXBM(102+LCDX0,LCDY0-4, 11, 16, num11x16[CurrentTime.Second/10]);
  }
  if(f5<200) f5++;
  switch (Roll_mode)
  {
  case NO_Roll:
  case UP_Roll:
  case DOWN_Roll:
       if(f5<17)
        {
          LED_Rolling(hcbuff,(unsigned char *)num11x16[timeroll[5]],(unsigned char *)num11x16[Timetemp[5]],16,2,Roll_mode,f5);
          u8g2.drawXBM(115+LCDX0, LCDY0-4, 11, 16, hcbuff);
          shangxiajingxiang(hcbuff,16,2);
          u8g2.drawXBM(115+LCDX0, LCDY0+12, 11, 16, hcbuff);
        }    
  break;
  default:
      if(f5<2) u8g2.drawXBM(115+LCDX0, LCDY0-4, 11, 16, num11x16[Timetemp[5]]);
  break;
  }
}


void shangxiajingxiang(unsigned char *Buff,unsigned char Row,unsigned char Column)
{
  unsigned char ch;
   for(char i=0; i<Row/2;i++) {//行扫描
      for(char j=0;j<Column;j++){ //列
          ch = Buff[i*Column+j];
          Buff[i*Column+j]= Buff[(Row-1-i)*Column+j];
          Buff[(Row-1-i)*Column+j]=ch;
      }
   }
 }

void WeatherRollClock()
{
   if(CurrentTime.Second >5 && CurrentTime.Second <20){
     FansDisplay();
     RollTimeFlag = true;
   }
   else if(CurrentTime.Second >25 && CurrentTime.Second <46){
     WeatherClockDisplay((CurrentTime.Second-26)/5);               //oledClockDisplay();
     RollTimeFlag = true;
     //FansFlag = true;
  }
  else {
    RollClockDisplay(); 
  }
}

void FansDisplay()
{       
         u8g2.clearBuffer();
         u8g2.setFont(u8g2_font_unifont_t_chinese2);
         u8g2.drawXBM(0, 16, 16, 16, bilibili_16x16[3]);
         u8g2.drawXBM(0, 32, 16, 16, bilibili_16x16[4]);
         u8g2.drawXBM(0, 48, 16, 16, bilibili_16x16[5]);
         //u8g2.drawXBMP( 0 , 16 , 24 , 24 , bilibilitv_24u );     //根据你的图片尺寸修改
         u8g2.setCursor(24, 29);
         u8g2.println(Bilibili.follower);
         u8g2.setCursor(24, 45);
         u8g2.println(Bilibili.view);
         u8g2.setCursor(24, 61);
         u8g2.println(Bilibili.likes); 
         u8g2.drawXBM(48, 0, 16, 16, bilibili_16x16[CurrentTime.Second%2]);
         u8g2.drawXBM(64, 0, 16, 16, bilibili_16x16[2]);
         u8g2.drawXBM(80, 0, 16, 16, bilibili_16x16[2]);
                 
         u8g2.sendBuffer(); 
}

void WeatherClockMirror()
{
   RollClock (0,4);
    if(LastSecond != CurrentTime.Second){
        LastSecond = CurrentTime.Second;
        //if(CurrentTime.Second%30<10){
         //滚动时间   
         //滚动时间
         //图标 今天04月16日 今天周四阵雨
         //图标 18/12℃78% 、东南风7级
         if(CurrentTime.Second<20)
          WeatherDisplay(CurrentTime.Second/10,Today.code,Today.Temp,Day0.Temp_HIGH,Day0.Temp_LOW,Day0.Humi,Day0.Wind_scale,Day0.Date,Day0.Wind_direction);
         else if(CurrentTime.Second<40) 
          WeatherDisplay(CurrentTime.Second/10,Day1.code,Today.Temp,Day1.Temp_HIGH,Day1.Temp_LOW,Day1.Humi,Day1.Wind_scale,Day1.Date,Day1.Wind_direction);
         else 
          WeatherDisplay(CurrentTime.Second/10,Day2.code,Today.Temp,Day2.Temp_HIGH,Day2.Temp_LOW,Day2.Humi,Day2.Wind_scale,Day2.Date,Day2.Wind_direction);
            /*u8g2.drawXBM(0, 32, 32, 32, Weather_tubiao[Weather_tubiao_index[Today.code]]);
            //u8g2.drawXBM(32,32, 11, 16, num11x16[Today.Temp/10]);
            //u8g2.drawXBM(44,32, 11, 16, num11x16[Today.Temp%10]);
            
            u8g2.drawXBM(32,32, 8, 16, num8x16[Today.Temp/10]);
            u8g2.drawXBM(40,32, 8, 16, num8x16[Today.Temp%10]);
            u8g2.drawXBM(48,32, 8, 16, num8x16[27]);
            
            u8g2.drawXBM(56, 32, 16, 16, HZ16x16[1]);//今
            u8g2.drawXBM(72,32, 16, 16, HZ16x16[4]);//天
            if(CurrentTime.Second%10<5){
              u8g2.drawXBM(88,33, 8, 15, num8x16[CurrentTime.Month/10+10]);
              u8g2.drawXBM(96,33, 8, 15, num8x16[CurrentTime.Month%10+10]);
              u8g2.drawXBM(104,33, 8, 15, num8x16[29]);
              u8g2.drawXBM(112,33, 8, 15, num8x16[CurrentTime.Day/10+10]);
              u8g2.drawXBM(120,33, 8, 15, num8x16[CurrentTime.Day%10+10]);
            }
            else
            {
              u8g2.drawXBM(88,32, 8, 16, num8x16[30]);
              u8g2.drawXBM(96, 32, 16, 16, HZ16x16[28]);
              u8g2.drawXBM(112, 32, 16, 16, HZ16x16[(CurrentTime.Week-1)%7 + 29]);
            }
            
            u8g2.drawXBM(32, 48, 16, 16, HZ16x16[HZcode_index[Today.code][0]]);//
            u8g2.drawXBM(48, 48, 16, 16, HZ16x16[HZcode_index[Today.code][1]]);//天气

            u8g2.drawXBM(64,48, 8, 16, num8x16[Day0.Humi/10+10]);
            u8g2.drawXBM(72,48, 8, 16, num8x16[Day0.Humi%10+10]);
            u8g2.drawXBM(80,48, 8, 16, num8x16[28]); 
            
            u8g2.drawXBM(120,48, 8, 16, num8x16[27]);
            u8g2.setFont(u8g2_font_unifont_t_chinese2);
            u8g2.setCursor(88, 63);
            u8g2.printf("%d/%d",Day0.Temp_HIGH,Day0.Temp_LOW);*/
      }
    
    u8g2.sendBuffer();  
}

void WeatherDisplay(char Day,char code,char Temp,char Temp_HIGH,char Temp_LOW,char Humi,char Wind_scale,String Date,String Wind_direction)
{//Day 今天：0 1  明天：2 3  后天：4 5
    u8g2.drawXBM(0, 32, 32, 32, Weather_tubiao[Weather_tubiao_index[code]]);
  //u8g2.drawXBM(32,32, 11, 16, num11x16[Today.Temp/10]);
  //u8g2.drawXBM(44,32, 11, 16, num11x16[Today.Temp%10]);
    if(Day <2) {       
      u8g2.drawXBM(32,32, 8, 16, num8x16[Today.Temp/10]);
      u8g2.drawXBM(40,32, 8, 16, num8x16[Today.Temp%10]);
      u8g2.drawXBM(48,32, 8, 16, num8x16[27]);
    }
    else{
      
    }       
   u8g2.drawXBM(56, 32, 16, 16, HZ16x16[Day/2+1]);//今
   u8g2.drawXBM(72,32, 16, 16, HZ16x16[4]);//天
   if(Day%2==0){
       u8g2.drawXBM(88,33, 8, 15, num8x16[Date.charAt(0)-'0'+10]);
       u8g2.drawXBM(96,33, 8, 15, num8x16[Date.charAt(1)-'0'+10]);
       u8g2.drawXBM(104,33, 8, 15, num8x16[29]);
       u8g2.drawXBM(112,33, 8, 15, num8x16[Date.charAt(3)-'0'+10]);
       u8g2.drawXBM(120,33, 8, 15, num8x16[Date.charAt(4)-'0'+10]);    
   }
   else {
     u8g2.drawXBM(88,32, 8, 16, num8x16[30]);
     u8g2.drawXBM(96, 32, 16, 16, HZ16x16[28]);
     u8g2.drawXBM(112, 32, 16, 16, HZ16x16[(CurrentTime.Week-1+Day/2)%7 + 29]);
   }
            
  u8g2.drawXBM(32, 48, 16, 16, HZ16x16[HZcode_index[code][0]]);//
  u8g2.drawXBM(48, 48, 16, 16, HZ16x16[HZcode_index[code][1]]);//天气

   //u8g2.drawXBM(64,49, 8, 16, num8x16[Humi/10+10]);
   //u8g2.drawXBM(72,49, 8, 16, num8x16[Humi%10+10]);
   //u8g2.drawXBM(80,49, 8, 16, num8x16[28]); 

   u8g2.drawXBM(88,49, 8, 16, num8x16[Temp_HIGH/10+10]);
   u8g2.drawXBM(96,49, 8, 16, num8x16[Temp_HIGH%10+10]);
   u8g2.drawXBM(104,49, 8, 16, num8x16[31]);
   if(Temp_LOW<10){
      u8g2.drawXBM(112,49, 8, 16, num8x16[Temp_LOW%10+10]);         
      u8g2.drawXBM(120,49, 8, 16, num8x16[27]);
   }
   else {
      u8g2.drawXBM(112,49, 8, 16, num8x16[Temp_LOW/10+10]);
      u8g2.drawXBM(120,49, 8, 16, num8x16[Temp_LOW%10+10]);
   }
   
   /*u8g2.setFont(u8g2_font_unifont_t_chinese2);
   u8g2.setCursor(88, 63);
   u8g2.printf("%d/%d",Temp_HIGH,Temp_LOW);*/
}


void WeatherClockDisplay(char times)
{
  u8g2.clearBuffer();
  /*u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  u8g2.printf("%02d/%02d/%02d %02d:%02d",CurrentTime.Year, CurrentTime.Month, CurrentTime.Day, CurrentTime.Hour, CurrentTime.Minute);
  u8g2.setCursor(0, 61);
  u8g2.printf("%.5s%.5s %.5s",Day0.text_day.c_str(),Day1.text_day.c_str(),Day2.text_day.c_str());*/
  if(Day0.code>37) Day0.code = 37;
  if(Day1.code>37) Day1.code = 37;
  if(Day2.code>37) Day2.code = 37;
  switch(times)
  {
     case 0:
         //今天 明天 后天   
         //晴  小雨 大雨
         /*u8g2.setFont(u8g2_font_unifont_t_chinese2);
         u8g2.setCursor(24, 14);
         u8g2.print("今天 明天 后天");*/        
         u8g2.drawXBM(4, 0, 16, 16, HZ16x16[1]);
         u8g2.drawXBM(20, 0, 16, 16, HZ16x16[4]);
         u8g2.drawXBM(48, 0, 16, 16, HZ16x16[2]);
         u8g2.drawXBM(64, 0, 16, 16, HZ16x16[4]);
         u8g2.drawXBM(92, 0, 16, 16, HZ16x16[3]);
         u8g2.drawXBM(108, 0, 16, 16, HZ16x16[4]);
         u8g2.drawXBM(4, 48, 16, 16, HZ16x16[HZcode_index[Day0.code][0]]);
         u8g2.drawXBM(20, 48, 16, 16, HZ16x16[HZcode_index[Day0.code][1]]);
         u8g2.drawXBM(48, 48, 16, 16, HZ16x16[HZcode_index[Day1.code][0]]);
         u8g2.drawXBM(64, 48, 16, 16, HZ16x16[HZcode_index[Day1.code][1]]);
         u8g2.drawXBM(92, 48, 16, 16, HZ16x16[HZcode_index[Day2.code][0]]);
         u8g2.drawXBM(108, 48, 16, 16, HZ16x16[HZcode_index[Day2.code][1]]);
         break;
     case 1:
         //4-16 4-17 4-18 日期 
         //18/7 15/7 19/9 高低温
         /*Day0.Date           = Datestring.substring(5,10);
  Day0.text_day       = results_0_daily_0_text_day;
  Day0.Wind_direction = results_0_daily_0_wind_direction;
  Day0.code       = atoi(results_0_daily_0_code_day);
  Day0.Temp_HIGH  = atoi(results_0_daily_0_high);
  Day0.Temp_LOW   = atoi(results_0_daily_0_low);
  Day0.Wind_scale = atoi(results_0_daily_0_wind_scale);
  Day0.Humi       = atoi(results_0_daily_0_humidity);*/
         u8g2.setFont(u8g2_font_unifont_t_chinese2);
         u8g2.setCursor(0, 12);
         u8g2.print(Day0.Date);
         u8g2.setCursor(44, 12);
         u8g2.print(Day1.Date);
         u8g2.setCursor(88, 12);
         u8g2.print(Day2.Date);
         
         u8g2.setCursor(0, 63);
         u8g2.printf("%d/%d",Day0.Temp_HIGH,Day0.Temp_LOW);
         u8g2.setCursor(44, 63);
         u8g2.printf("%d/%d",Day1.Temp_HIGH,Day1.Temp_LOW);
         u8g2.setCursor(88, 63);
         u8g2.printf("%d/%d",Day2.Temp_HIGH,Day2.Temp_LOW);

         u8g2.drawLine(0, 14, 127, 14);
         u8g2.drawLine(0, 49, 127, 49);
         break;
     case 2:
         // 周四 周五 周六   
         //   7   8   4   风向 风力等级
         u8g2.setFont(u8g2_font_unifont_t_chinese2);
         u8g2.drawXBM(5, 0, 16, 16, HZ16x16[28]);
         u8g2.drawXBM(21, 0, 16, 16, HZ16x16[(CurrentTime.Week-1)%7 + 29]);
         u8g2.drawXBM(48, 0, 16, 16, HZ16x16[28]);
         u8g2.drawXBM(64, 0, 16, 16, HZ16x16[CurrentTime.Week%7 + 29]);
         u8g2.drawXBM(91, 0, 16, 16, HZ16x16[28]);
         u8g2.drawXBM(107,0, 16, 16, HZ16x16[(CurrentTime.Week+1)%7 + 29]);
         u8g2.setCursor(0, 63);
         u8g2.printf("%s %d",Day0.Wind_direction.c_str(),Day0.Wind_scale);
         u8g2.setCursor(44, 63);
         u8g2.printf("%s %d",Day1.Wind_direction.c_str(),Day1.Wind_scale);
         u8g2.setCursor(88, 63);
         u8g2.printf("%s %d",Day0.Wind_direction.c_str(),Day0.Wind_scale);
         break;
     case 3:
         //更新时间：14:30   
         //82% 56% 73%  //湿度
          u8g2.setFont(u8g2_font_unifont_t_chinese2);
          u8g2.setCursor(0, 14);
          u8g2.printf("%s %s更新",Today.Location.c_str(),Today.last_update.c_str());
          u8g2.setCursor(9, 63);
          //u8g2.printf("%d%%",Day0.Humi);
          u8g2.setCursor(52, 63);
          //u8g2.printf("%d%%",Day1.Humi);
          u8g2.setCursor(95, 63);
         // u8g2.printf("%d%%",Day2.Humi);
         break;

     default:
         
         break;
  }
  u8g2.drawXBM(5, 16, 32, 32, Weather_tubiao[Weather_tubiao_index[Day0.code]]);
  u8g2.drawXBM(48, 16, 32, 32, Weather_tubiao[Weather_tubiao_index[Day1.code]]);
  u8g2.drawXBM(91, 16, 32, 32, Weather_tubiao[Weather_tubiao_index[Day2.code]]);  
  if(times==3){
    u8g2.drawLine(42, 16, 42, 63);
    u8g2.drawLine(86, 16, 86, 63);  
  }
  else{
    u8g2.drawLine(42, 0, 42, 63);
    u8g2.drawLine(86, 0, 86, 63);  
  }

  u8g2.sendBuffer();
}
//---------------------------------------CurrentTime 更新---------------------------------------
void CurrentTime_update()
{
  if(LastUpdateTimeSecond != second())
  {
    LastUpdateTimeSecond = second();
    CurrentTime.Year   = year();
    CurrentTime.Month  = month();
    CurrentTime.Day    = day();
    CurrentTime.Hour   = hour();
    CurrentTime.Minute = minute();
    CurrentTime.Second = second();
    CurrentTime.Week   = weekday();
    Serial.printf("[Now time] %04d/%02d/%02d %02d:%02d:%02d Weekday:%d\n", CurrentTime.Year, CurrentTime.Month, CurrentTime.Day, CurrentTime.Hour, CurrentTime.Minute, CurrentTime.Second, CurrentTime.Week);
  }
}

//---------------------------------------WEATHER JSON---------------------------------------
bool GetWeatherJson()
{
    bool r = false;
    String json;
    http.setTimeout(HTTP_TIMEOUT);
    
    //now
    http.begin(NowURL);
    //http.begin("http://api.seniverse.com/v3/weather/now.json?key=h1apwd3vjkgrsvtc&location=ip&language=zh-Hans&unit=c");
    int httpCode = http.GET();
    Serial.printf("Now.json HTTP Response Status Code:%d\r\n",httpCode);
    if (httpCode == HTTP_CODE_OK){
        json = http.getString();
        //Serial.println(json);
        r = true;
    }
    else{
        Serial.printf("[HTTP] GET JSON failed, error: %s\n", http.errorToString(httpCode).c_str());
        r = false;
    }
    http.end();
    if(r) ParseNowJson(json);
    
    http.begin(DailyURL);
    httpCode = http.GET();
    Serial.printf("Daily.json HTTP Response Status Code:%d\r\n",httpCode);
    if (httpCode == HTTP_CODE_OK){
        json = http.getString();
        //Serial.println(json);
        r = true;
    }
    else{
        Serial.printf("[HTTP] GET JSON failed, error: %s\n", http.errorToString(httpCode).c_str());
        r = false;
    }
    http.end();
    if(r) ParseDailyJson(json);
        
    return r;
}

void ParseNowJson(String json)
{
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 240;
  DynamicJsonDocument doc(capacity);  
  //const char* json = "{\"results\":[{\"location\":{\"id\":\"WWYMRT0VRMUG\",\"name\":\"Dalian\",\"country\":\"CN\",\"path\":\"Dalian,Dalian,Liaoning,China\",\"timezone\":\"Asia/Shanghai\",\"timezone_offset\":\"+08:00\"},\"now\":{\"text\":\"Clear\",\"code\":\"1\",\"temperature\":\"9\"},\"last_update\":\"2020-04-15T19:29:00+08:00\"}]}";
  
  deserializeJson(doc, json);
  
  JsonObject results_0 = doc["results"][0];
  
  JsonObject results_0_location = results_0["location"];
  //const char* results_0_location_id = results_0_location["id"]; // "WWYMRT0VRMUG"
  const char* results_0_location_name = results_0_location["name"]; // "Dalian"
  //const char* results_0_location_country = results_0_location["country"]; // "CN"
  //const char* results_0_location_path = results_0_location["path"]; // "Dalian,Dalian,Liaoning,China"
  //const char* results_0_location_timezone = results_0_location["timezone"]; // "Asia/Shanghai"
  //const char* results_0_location_timezone_offset = results_0_location["timezone_offset"]; // "+08:00"
  
  JsonObject results_0_now = results_0["now"];
  const char* results_0_now_text = results_0_now["text"]; // "Clear"
  const char* results_0_now_code = results_0_now["code"]; // "1"
  const char* results_0_now_temperature = results_0_now["temperature"]; // "9"
  
  const char* results_0_last_update = results_0["last_update"]; // "2020-04-15T19:29:00+08:00"
  String WeatherLastUpdate = results_0_last_update;
  Today.Location = results_0_location_name;
  Today.code =atoi(results_0_now_code);
  Today.Temp =atoi(results_0_now_temperature);
  Today.last_update = WeatherLastUpdate.substring(11,16);

  Serial.printf("City:%s   Update:%s \r\n",Today.Location.c_str(),Today.last_update.c_str());
  Serial.printf("Code:%d 温度:%d\r\n",Today.code,Today.Temp);
  
}
void ParseDailyJson(String json)
{
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 3*JSON_OBJECT_SIZE(14) + 1000;//870;
  DynamicJsonDocument doc(capacity);
  
  //const char* json = "{\"results\":[{\"location\":{\"id\":\"WWYMRT0VRMUG\",\"name\":\"Dalian\",\"country\":\"CN\",\"path\":\"Dalian,Dalian,Liaoning,China\",\"timezone\":\"Asia/Shanghai\",\"timezone_offset\":\"+08:00\"},\"daily\":[{\"date\":\"2020-04-15\",\"text_day\":\"Sunny\",\"code_day\":\"0\",\"text_night\":\"Cloudy\",\"code_night\":\"4\",\"high\":\"16\",\"low\":\"8\",\"rainfall\":\"0.0\",\"precip\":\"\",\"wind_direction\":\"SE\",\"wind_direction_degree\":\"135\",\"wind_speed\":\"34.20\",\"wind_scale\":\"5\",\"humidity\":\"59\"},{\"date\":\"2020-04-16\",\"text_day\":\"Moderate rain\",\"code_day\":\"14\",\"text_night\":\"Moderate rain\",\"code_night\":\"14\",\"high\":\"14\",\"low\":\"8\",\"rainfall\":\"12.3\",\"precip\":\"\",\"wind_direction\":\"SE\",\"wind_direction_degree\":\"135\",\"wind_speed\":\"57.60\",\"wind_scale\":\"7\",\"humidity\":\"67\"},{\"date\":\"2020-04-17\",\"text_day\":\"Cloudy\",\"code_day\":\"4\",\"text_night\":\"Clear\",\"code_night\":\"1\",\"high\":\"15\",\"low\":\"9\",\"rainfall\":\"0.0\",\"precip\":\"\",\"wind_direction\":\"CLM\",\"wind_direction_degree\":\"1\",\"wind_speed\":\"45.00\",\"wind_scale\":\"6\",\"humidity\":\"67\"}],\"last_update\":\"2020-04-15T17:25:53+08:00\"}]}";
  
  deserializeJson(doc, json);
  
  JsonObject results_0 = doc["results"][0];
  
  JsonObject results_0_location = results_0["location"];
  //const char* results_0_location_id = results_0_location["id"]; // "WWYMRT0VRMUG"
  const char* results_0_location_name = results_0_location["name"]; // "Dalian"
  //const char* results_0_location_country = results_0_location["country"]; // "CN"
  //const char* results_0_location_path = results_0_location["path"]; // "Dalian,Dalian,Liaoning,China"
  //const char* results_0_location_timezone = results_0_location["timezone"]; // "Asia/Shanghai"
  //const char* results_0_location_timezone_offset = results_0_location["timezone_offset"]; // "+08:00"
  
  JsonArray results_0_daily = results_0["daily"];
  
  JsonObject results_0_daily_0 = results_0_daily[0];
  const char* results_0_daily_0_date = results_0_daily_0["date"]; // "2020-04-15"==========今天日期
  const char* results_0_daily_0_text_day = results_0_daily_0["text_day"]; // "Sunny"=======白天天气
  const char* results_0_daily_0_code_day = results_0_daily_0["code_day"]; // "0"===========图标编号
  const char* results_0_daily_0_text_night = results_0_daily_0["text_night"]; // "Cloudy"==晚上天气
  const char* results_0_daily_0_code_night = results_0_daily_0["code_night"]; // "4"=======图标编号
  const char* results_0_daily_0_high = results_0_daily_0["high"]; // "16"==================最高温度
  const char* results_0_daily_0_low = results_0_daily_0["low"]; // "8"=====================最低温度
  const char* results_0_daily_0_rainfall = results_0_daily_0["rainfall"]; // "0.0"=========雨量
  const char* results_0_daily_0_precip = results_0_daily_0["precip"]; // ""================降水概率
  const char* results_0_daily_0_wind_direction = results_0_daily_0["wind_direction"]; // "SE"=风向
  const char* results_0_daily_0_wind_direction_degree = results_0_daily_0["wind_direction_degree"]; // "135"=风向
  const char* results_0_daily_0_wind_speed = results_0_daily_0["wind_speed"]; // "34.20"===风速
  const char* results_0_daily_0_wind_scale = results_0_daily_0["wind_scale"]; // "5"=======风力等级
  const char* results_0_daily_0_humidity = results_0_daily_0["humidity"]; // "59"==========湿度
  
  JsonObject results_0_daily_1 = results_0_daily[1];
  const char* results_0_daily_1_date = results_0_daily_1["date"]; // "2020-04-16"
  const char* results_0_daily_1_text_day = results_0_daily_1["text_day"]; // "Moderate rain"
  const char* results_0_daily_1_code_day = results_0_daily_1["code_day"]; // "14"
  const char* results_0_daily_1_text_night = results_0_daily_1["text_night"]; // "Moderate rain"
  const char* results_0_daily_1_code_night = results_0_daily_1["code_night"]; // "14"
  const char* results_0_daily_1_high = results_0_daily_1["high"]; // "14"
  const char* results_0_daily_1_low = results_0_daily_1["low"]; // "8"
  const char* results_0_daily_1_rainfall = results_0_daily_1["rainfall"]; // "12.3"
  const char* results_0_daily_1_precip = results_0_daily_1["precip"]; // ""
  const char* results_0_daily_1_wind_direction = results_0_daily_1["wind_direction"]; // "SE"
  const char* results_0_daily_1_wind_direction_degree = results_0_daily_1["wind_direction_degree"]; // "135"
  const char* results_0_daily_1_wind_speed = results_0_daily_1["wind_speed"]; // "57.60"
  const char* results_0_daily_1_wind_scale = results_0_daily_1["wind_scale"]; // "7"
  const char* results_0_daily_1_humidity = results_0_daily_1["humidity"]; // "67"
  
  JsonObject results_0_daily_2 = results_0_daily[2];
  const char* results_0_daily_2_date = results_0_daily_2["date"]; // "2020-04-17"
  const char* results_0_daily_2_text_day = results_0_daily_2["text_day"]; // "Cloudy"
  const char* results_0_daily_2_code_day = results_0_daily_2["code_day"]; // "4"
  const char* results_0_daily_2_text_night = results_0_daily_2["text_night"]; // "Clear"
  const char* results_0_daily_2_code_night = results_0_daily_2["code_night"]; // "1"
  const char* results_0_daily_2_high = results_0_daily_2["high"]; // "15"
  const char* results_0_daily_2_low = results_0_daily_2["low"]; // "9"
  const char* results_0_daily_2_rainfall = results_0_daily_2["rainfall"]; // "0.0"
  const char* results_0_daily_2_precip = results_0_daily_2["precip"]; // ""
  const char* results_0_daily_2_wind_direction = results_0_daily_2["wind_direction"]; // "CLM"
  const char* results_0_daily_2_wind_direction_degree = results_0_daily_2["wind_direction_degree"]; // "1"
  const char* results_0_daily_2_wind_speed = results_0_daily_2["wind_speed"]; // "45.00"
  const char* results_0_daily_2_wind_scale = results_0_daily_2["wind_scale"]; // "6"
  const char* results_0_daily_2_humidity = results_0_daily_2["humidity"]; // "67"
  
  const char* results_0_last_update = results_0["last_update"]; // "2020-04-15T17:25:53+08:00" 
  /*String  Date;
  String  text_day;
  String  Wind_direction;
  char    code;
  char    Temp_HIGH;
  char    Temp_LOW;
  char    Wind_scale;
  char    Humi; */
  
  String Datestring = results_0_daily_0_date;
  Day0.Date           = Datestring.substring(5,10);
  Day0.text_day       = results_0_daily_0_text_day;
  Day0.Wind_direction = results_0_daily_0_wind_direction;
  Day0.code       = atoi(results_0_daily_0_code_day);
  Day0.Temp_HIGH  = atoi(results_0_daily_0_high);
  Day0.Temp_LOW   = atoi(results_0_daily_0_low);
  Day0.Wind_scale = atoi(results_0_daily_0_wind_scale);
  Day0.Humi       = atoi(results_0_daily_0_humidity);
  
  
  Datestring = results_0_daily_1_date;
  Day1.Date           = Datestring.substring(5,10);
  Day1.text_day       = results_0_daily_1_text_day;
  Day1.Wind_direction = results_0_daily_1_wind_direction;
  Day1.code       = atoi(results_0_daily_1_code_day);
  Day1.Temp_HIGH  = atoi(results_0_daily_1_high);
  Day1.Temp_LOW   = atoi(results_0_daily_1_low);
  Day1.Wind_scale = atoi(results_0_daily_1_wind_scale);
  Day1.Humi       = atoi(results_0_daily_1_humidity);
  
  Datestring = results_0_daily_2_date;
  Day2.Date           = Datestring.substring(5,10);
  Day2.text_day       = results_0_daily_2_text_day;
  Day2.Wind_direction = results_0_daily_2_wind_direction;
  Day2.code       = atoi(results_0_daily_2_code_day);
  Day2.Temp_HIGH  = atoi(results_0_daily_2_high);
  Day2.Temp_LOW   = atoi(results_0_daily_2_low);
  Day2.Wind_scale = atoi(results_0_daily_2_wind_scale);
  Day2.Humi       = atoi(results_0_daily_2_humidity);
  
  Serial.printf("今天:%s 明天:%s 后天:%s \r\n",Day0.text_day.c_str(),Day1.text_day.c_str(),Day2.text_day.c_str());
}


//=================时钟显示=======================
void RollClockDisplay()
{ 
  if(RollTimeFlag) u8g2.clearBuffer();
  //
  /*u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  if (isNTPConnected)
    u8g2.print("当前时间 (UTC+8)");
  else
    u8g2.print("NTP无响应!"); //如果上次对时失败，则会显示无网络*/

  //// 2+ 19+3+19 +5+4+5+ 19+3+19 +4+ 11+2+11+2
  
  /*u8g2.drawXBM(2, 20, 19, 24, num19x24[hours/10]);
  u8g2.drawXBM(24, 20, 19, 24, num19x24[hours%10]);
  u8g2.drawXBM(46, 20, 8, 24, TimeDot8x24[seconds%2]);
  u8g2.drawXBM(57, 20, 19, 24, num19x24[minutes/10]);
  u8g2.drawXBM(79, 20, 19, 24, num19x24[minutes%10]);
  u8g2.drawXBM(102, 28, 11, 16, num11x16[seconds/10]);
  u8g2.drawXBM(115, 28, 11, 16, num11x16[seconds%10]);*/
  if(Timetemp[0]!=CurrentTime.Hour/10 || RollTimeFlag) {//小时 十位
    f0=0;
    Timetemp[0]=CurrentTime.Hour/10;
    if(Timetemp[0]==0) timeroll[0] = 2;
    else timeroll[0] = Timetemp[0]-1;
  }
  if(f0<200) f0++;
  switch (Roll_mode)
  {
  case UP_Roll:
  case DOWN_Roll:
      if(f0<25)
      {
        LED_Rolling(hcbuff,(unsigned char *)num19x24[timeroll[0]],(unsigned char *)num19x24[Timetemp[0]],24,3,Roll_mode,f0);
        //UG_PutChar(0,0,5,8,hcbuff,1);
        u8g2.drawXBM(2, 20, 19, 24, hcbuff);
      }
  break;
  default:
      if(f0<2) u8g2.drawXBM(2, 20, 19, 24, num19x24[CurrentTime.Hour/10]);
  break;
  }
//---------------
  if(Timetemp[1]!=CurrentTime.Hour%10 || RollTimeFlag) {//小时 个位
    f1=0;
    Timetemp[1]=CurrentTime.Hour%10;
    if(Timetemp[1]==0) timeroll[1] = 9;
    else timeroll[1] = Timetemp[1]-1;
  }
  if(f1<200) f1++;
  switch (Roll_mode)
  {
  case UP_Roll:
  case DOWN_Roll:
      if(f1<25)
      {
        LED_Rolling(hcbuff,(unsigned char *)num19x24[timeroll[1]],(unsigned char *)num19x24[Timetemp[1]],24,3,Roll_mode,f1);
        u8g2.drawXBM(24, 20, 19, 24, hcbuff);
      }
  break;
  default:
      if(f1<2) u8g2.drawXBM(24, 20, 19, 24, num19x24[CurrentTime.Hour%10]);
  break;
  }
  //----------------------
  if(Timetemp[2]!=CurrentTime.Minute/10 || RollTimeFlag) {
    f2=0;
    Timetemp[2]=CurrentTime.Minute/10;
    if(Timetemp[2]==0) timeroll[2] = 5;
    else timeroll[2] = Timetemp[2]-1;
  }
  if(f2<200) f2++;
  switch (Roll_mode)
  {
  case UP_Roll:
  case DOWN_Roll:
      if(f2<25)
      {
        LED_Rolling(hcbuff,(unsigned char *)num19x24[timeroll[2]],(unsigned char *)num19x24[Timetemp[2]],24,3,Roll_mode,f2);
        u8g2.drawXBM(57, 20, 19, 24, hcbuff);
      }
  break;
  default:
      if(f2<2) u8g2.drawXBM(57, 20, 19, 24, num19x24[CurrentTime.Minute/10]);
  break;
  }
  //-----------------------
  if(Timetemp[3]!=CurrentTime.Minute%10 || RollTimeFlag) {
    f3=0;
    Timetemp[3]=CurrentTime.Minute%10;
    if(Timetemp[3]==0) timeroll[3] = 9;
    else timeroll[3] = Timetemp[3]-1;
    Roll_mode = CurrentTime.Hour%3;
  }
  if(f3<200) f3++;
  switch (Roll_mode)
  {
  case UP_Roll:
  case DOWN_Roll:
      if(f3<25)
      {
        LED_Rolling(hcbuff,(unsigned char *)num19x24[timeroll[3]],(unsigned char *)num19x24[Timetemp[3]],24,3,Roll_mode,f3);
        u8g2.drawXBM(79, 20, 19, 24, hcbuff);
      }
  break;
  default:
      if(f3<2) u8g2.drawXBM(79, 20, 19, 24, num19x24[CurrentTime.Minute%10]);
  break;
  }
  //---------------
  if(Timetemp[4]!=CurrentTime.Second/10 || RollTimeFlag) {
    f4=0;
    Timetemp[4]=CurrentTime.Second/10;
    if(Timetemp[4]==0) timeroll[4] = 5;
    else timeroll[4] = Timetemp[4]-1;
  }
  if(f4<200) f4++;
  switch (Roll_mode)
  {
  case UP_Roll:
  case DOWN_Roll:
      if(f4<17)
      {
        LED_Rolling(hcbuff,(unsigned char *)num11x16[timeroll[4]],(unsigned char *)num11x16[Timetemp[4]],16,2,Roll_mode,f4);
        u8g2.drawXBM(102, 28, 11, 16, hcbuff);
      }
  break;
  default:
      if(f4<2) u8g2.drawXBM(102, 28, 11, 16, num11x16[CurrentTime.Second/10]);
  break;
  }
  //--------
  if(Timetemp[5]!=CurrentTime.Second%10 || RollTimeFlag) {
    f5=0;
    Timetemp[5]=CurrentTime.Second%10;
    if(Timetemp[5]==0) timeroll[5] = 9;
    else timeroll[5] = Timetemp[5]-1;
    RollTimeFlag = false;
    //每秒钟处理的任务
    u8g2.drawXBM(2, 20, 19, 24, num19x24[CurrentTime.Hour/10]);
    u8g2.drawXBM(24, 20, 19, 24, num19x24[CurrentTime.Hour%10]);
    u8g2.drawXBM(46, 20, 8, 24, TimeDot8x24[CurrentTime.Second%2]);
    u8g2.drawXBM(57, 20, 19, 24, num19x24[CurrentTime.Minute/10]);
    u8g2.drawXBM(79, 20, 19, 24, num19x24[CurrentTime.Minute%10]);
    u8g2.drawXBM(102, 28, 11, 16, num11x16[CurrentTime.Second/10]);
    //u8g2.drawXBM(115, 28, 11, 16, num11x16[seconds%10]);
    //u8g2.drawXBM(46, 20, 8, 24, TimeDot8x24[CurrentTime.Second%2]);//显示小时分钟间隔
    //更新日期星期
     String currentDay = "";
        currentDay += CurrentTime.Year;
        currentDay += "/";
        if (CurrentTime.Month < 10)
          currentDay += 0;
        currentDay += CurrentTime.Month;
        currentDay += "/";
        if (CurrentTime.Day < 10)
          currentDay += 0;
        currentDay += CurrentTime.Day;
        
      u8g2.setFont(u8g2_font_unifont_t_chinese2);
      if(isNTPConnected) 
        {
          u8g2.setCursor(0, 12);
          u8g2.print(currentDay);
        }
      else
      {
        u8g2.setCursor(0, 14);
        u8g2.print("NTP无响应!"); //如果上次对时失败，则会显示无网络
      }
      u8g2.drawXBM(80, 0, 16, 16, xing);
      u8g2.setCursor(95, 14);
      u8g2.print("期");
      switch (CurrentTime.Week)
      {
      case 1:
        u8g2.print("日");
      break;
      case 2:
        u8g2.print("一");
      break;
      case 3:
        u8g2.print("二");
      break;
      case 4:
        u8g2.print("三");
      break;
      case 5:
        u8g2.print("四");
      break;
      case 6:
        u8g2.print("五");
      break;
      case 7:
        u8g2.drawXBM(111, 0, 16, 16, liu);
      break;
      default:
      break;
      }
   //更新温湿度
   char temperature = 25;
   char humidity = 63;
   u8g2.setCursor(0, 63);
   u8g2.print("26.5 62%");
  }
  if(f5<200) f5++;
  switch (Roll_mode)
  {
  case UP_Roll:
  case DOWN_Roll:
       if(f5<17)
        {
          LED_Rolling(hcbuff,(unsigned char *)num11x16[timeroll[5]],(unsigned char *)num11x16[Timetemp[5]],16,2,Roll_mode,f5);
          u8g2.drawXBM(115, 28, 11, 16, hcbuff);
        }    
  break;
  default:
      if(f5<2) u8g2.drawXBM(115, 28, 11, 16, num11x16[Timetemp[5]]);
  break;
  }
  
  u8g2.sendBuffer();
}

void LED_Rolling(unsigned char *Buff,unsigned char *aBuff,unsigned char *bBuff,unsigned char row,unsigned char Column,unsigned char type,unsigned char t)
{
  unsigned char  i,j;
  switch (type)
  {
    case NO_Roll://上滚动
       for(i=0; i<row;i++) {//行扫描
          for(j=0;j<Column;j++) //列
            Buff[i*Column+j] = bBuff[i*Column+j];
        }
      break;
    case UP_Roll://上滚动
      for(i=0; i<row-t;i++) {//行扫描
          for(j=0;j<Column;j++) //列
            Buff[i*Column+j] = aBuff[(i+t)*Column+j];
        }
      for(i=row-t; i<row;i++) {
          for(j=0;j<Column;j++) //列
            Buff[i*Column+j] = bBuff[(i+t-row)*Column+j]; 
        }
      break;
    case DOWN_Roll://下滚动
      for(i=0; i<row-t;i++) //行扫描
          for(j=0;j<Column;j++) //列
            Buff[(i+t)*Column+j] = aBuff[i*Column+j];
                
        for(i=row-t; i<row;i++) 
          for(j=0;j<Column;j++)   
            Buff[(i+t-row)*Column+j] = bBuff[i*Column+j];
      break;
    case LEFT_Roll://左滚动  Column=1
      for(i=0; i<row;i++) 
        Buff[i] = aBuff[i]<<t|bBuff[i]>>(8-t);
      break;
    case RIGHT_Roll://右滚动 Column=1
      for(i=0; i<row;i++) 
        Buff[i] = aBuff[i]>>t|bBuff[i]<<(8-t);
      break;
    default:
      break;
  }
}

void key_process()
{
   unsigned int counter = 0;
   if(digitalRead(KEY_FLASH)==0)
    {
       while(digitalRead(KEY_FLASH)==0)          //计数，用来判断是长按还是短按
         {
          if( (counter++) > 5000) //counter>65534:长按，
            {
              counter = 0;
              WifiSmartConfig();
                //Serial.println(" long press");
              return;
            }
           delay(1);
         }
         if(counter<=5000 && counter>30)   //counter<=65534:短按，counter>20:消抖动
         {   
          //Key_Code=KEY_FP_UP;             //KEY1短按时返回的键值
          if(ClockMode ==0)
              ClockMode = 1;
          else
              ClockMode = 0;
          u8g2.clearBuffer();
          counter = 0;
          return;       
         }
    } 
 }





 ////////// ////////////////////////////////日期间隔//////////////////////////////////////////////////
 
 int GetDays(int iYear1, int iMonth1, int iDay1, int iYear2, int iMonth2, int iDay2)   //1. 确保 日期1 < 日期2
{
  unsigned int iDate1 = iYear1 * 10000 + iMonth1 * 100 + iDay1;
  unsigned int iDate2 = iYear2 * 10000 + iMonth2 * 100 + iDay2;
  if (iDate1 > iDate2)
  {
    iYear1 = iYear1 + iYear2 - (iYear2 = iYear1);
    iMonth1 = iMonth1 + iMonth2 - (iMonth2 = iMonth1);
    iDay1 = iDay1 + iDay2 - (iDay2 = iDay1);
  }
  
  //2. 开始计算天数
  //计算 从 iYear1年1月1日 到 iYear2年1月1日前一天 之间的天数
  int iDays = 0;
  for (int i = iYear1; i < iYear2; i++)
  {
    iDays += (IsLeapYear(i) ? 366 : 365);
  }
  
  //减去iYear1年前iMonth1月的天数
  for (int i = 1; i < iMonth1; i++)
  {
    switch (i)
    {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
      iDays -= 31;
      break;
    case 4: case 6: case 9: case 11:
      iDays -= 30;
      break;
    case 2:
      iDays -= (IsLeapYear(iYear1) ? 29 : 28);
      break;
    }
  }
  //减去iYear1年iMonth1月前iDay1的天数
  iDays -= (iDay1 - 1);
 //加上iYear2年前iMonth2月的天数
 for (int i = 1; i < iMonth2; i++)
  {
    switch (i)
    {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
      iDays += 31;
      break;
    case 4: case 6: case 9: case 11:
      iDays += 30;
      break;
    case 2:
      iDays += (IsLeapYear(iYear2) ? 29 : 28);
      break;
    }
  }
  //加上iYear2年iMonth2月前iDay2的天数
  iDays += (iDay2 - 1);
  return iDays;
}
 
  //判断给定年是否闰年
bool IsLeapYear(int iYear)
{
  if (iYear % 100 == 0)
    return ((iYear % 400 == 0));
  else
    return ((iYear % 4 == 0));
}

void mqtt_callback(char *topic, byte *payload, unsigned int length) //mqtt回调函数
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    payload[length] = '\0';
    Serial.println((char *)payload);
   // https://arduinojson.org/v5/assistant/  json数据解析网站

    
    Serial.println("   ");
    Serial.println((char *)payload);
    Serial.println("   ");

    if (strstr(topic, ALINK_TOPIC_PROP_SET))
    {
        StaticJsonDocument<100> doc;
        DeserializationError error = deserializeJson(doc,payload);
        int params_LightSwitch = doc["params"]["LightSwitch"];
        int bli = doc["LightSwitch"]["value"];
        
        
        if(params_LightSwitch==1)            //收到1，蜂鸣器就滴一下
        {Serial.println("led on");
        digitalWrite(LED, HIGH);    
        delay(50) ;
        digitalWrite(LED, LOW);
        }

        if(bli ==0)            //收到按键1按下0，蜂鸣器就滴一下
        {Serial.println("led on");
        digitalWrite(LED, HIGH);    
        delay(50) ;
        digitalWrite(LED, LOW);
        }
        
        //else
        //{Serial.println("led off");
        //digitalWrite(LED, LOW); }
        
        if (error)
        {
            Serial.print("deserializeJson() failed with code ");
            Serial.println(error.c_str());
            return;
        }
    }
}
void mqtt_version_post()
{
    char param[512];
    char jsonBuf[1024];

    //sprintf(param, "{\"MotionAlarmState\":%d}", digitalRead(13));

    
    sprintf(param, "{\"id\": esp02,\"params\": {\"version\": \"%s\"}}", DEV_VERSION);

    
   // sprintf(jsonBuf, ALINK_BODY_FORMAT, ALINK_METHOD_PROP_POST, param);
    Serial.println(param);
    mqttClient.publish(ALINK_TOPIC_DEV_INFO, param);
}
void mqtt_check_connect()
{
    while (!mqttClient.connected())//
    {
        while (connect_aliyun_mqtt(mqttClient, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET))
        {
            Serial.println("MQTT connect succeed!");
            //client.subscribe(ALINK_TOPIC_PROP_POSTRSP);
            mqttClient.subscribe(ALINK_TOPIC_PROP_SET);
            
            Serial.println("subscribe done");
            mqtt_version_post();
        }
    }
    
}

void mqtt_interval_post()
{
    char param[512];
    char jsonBuf[1024];

    //sprintf(param, "{\"MotionAlarmState\":%d}", digitalRead(13));
    
    sprintf(param, "{\"LightSwitch\":%d,\"CurrentTemperature\":%d}", button1State ,random(0,55));
    
    sprintf(jsonBuf, ALINK_BODY_FORMAT, ALINK_METHOD_PROP_POST, param);
    Serial.println(jsonBuf);
    mqttClient.publish(ALINK_TOPIC_PROP_POST, jsonBuf);
}
