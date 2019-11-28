#include <M5StickC.h>
#include <Wire.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "esp_sleep.h"
#include "sys/time.h"
#include <WiFi.h>

#define T_PERIOD 5
// Transmission period
#define S_PERIOD 1                 // Silent period
RTC_DATA_ATTR static uint8_t seq;  // remember number of boots in RTC Memory
RTC_TimeTypeDef rtc_time_struct;
RTC_DateTypeDef rtc_date_struct;

struct tm now;
time_t tm_t;
void setAdvData(BLEAdvertising *pAdvertising, struct tm now);

const char* SSID = "すずのiphone";
const char* PASSWORD = "ryo5502gga";
const char* URL_NTP_SERVER = "ntp.jst.mfeed.ad.jp";

void task_ble() {
  BLEDevice::init("AmbientEnv-02");                // デバイスを初期化
  BLEServer *pServer = BLEDevice::createServer();  // サーバーを生成
  BLEAdvertising *pAdvertising =
      pServer->getAdvertising();  // アドバタイズオブジェクトを取得
  setAdvData(pAdvertising, now);
  pAdvertising->start();   // アドバタイジングデーターをセット
  delay(T_PERIOD * 1000);  // T_PERIOD秒アドバタイズする
  pAdvertising->stop();
}

void set_time(){
  if(seq != 0) return;

  WiFi.begin(SSID,PASSWORD);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    M5.Lcd.print(".");
    
  }
  M5.Lcd.print("Wifi connected.");
  configTime(9 * 3600 , 0, URL_NTP_SERVER);

  struct tm _tm;
  if(!getLocalTime(&_tm)) return ;

  RTC_TimeTypeDef _time_struct;
  _time_struct.Hours = _tm.tm_hour;
  _time_struct.Minutes = _tm.tm_min;
  _time_struct.Seconds = _tm.tm_sec;
  M5.Rtc.SetTime(&_time_struct);

  RTC_DateTypeDef _date_struct;
  _date_struct.WeekDay = _tm.tm_wday;
  _date_struct.Month = _tm.tm_mon + 1;
  _date_struct.Date = _tm.tm_mday;
  _date_struct.Year = _tm.tm_year + 1900;
  M5.Rtc.SetData(&_date_struct);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void task_lcd() {
  M5.begin();
  M5.Axp.ScreenBreath(10);  // 画面の輝度を下げる
  M5.Lcd.setRotation(3);    // 左を上にする
  M5.Lcd.setTextSize(3);  // 文字サイズを2にする
  M5.Lcd.printf("seq: %d\r\n", seq);
  M5.Lcd.printf("tm:\r\n"); 
  M5.Lcd.printf("%d:%d:%d'\r\n", now.tm_hour, now.tm_min, now.tm_sec);
}

void setAdvData(BLEAdvertising *pAdvertising, struct tm now) {
  M5.Rtc.GetTime(&rtc_time_struct);
  M5.Rtc.GetData(&rtc_date_struct);

  struct tm _tm;
  _tm.tm_hour = rtc_time_struct.Hours;
  _tm.tm_mday = rtc_date_struct.Date;
  _tm.tm_min = rtc_time_struct.Minutes;
  _tm.tm_mon = rtc_date_struct.Month;
  _tm.tm_sec = rtc_time_struct.Seconds;
  _tm.tm_wday = rtc_date_struct.WeekDay;
  _tm.tm_year = rtc_date_struct.Year;

  tm_t = mktime(&_tm); //&nowにするといい値?


  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

  oAdvertisementData.setFlags(
      0x06);  // BR_EDR_NOT_SUPPORTED | LE General Discoverable Mode

  std::string strServiceData = "";
  strServiceData += (char)8;     // 長さ
  strServiceData += (char)0xff;  // AD Type 0xFF: Manufacturer specific data
  strServiceData += (char)0xff;  // Test manufacture ID low byte
  strServiceData += (char)0xff;  // Test manufacture ID high byte
  strServiceData += (char)seq++;
  strServiceData += (char)(tm_t & 0xff);
  strServiceData += (char)((tm_t >> 8) & 0xff);
  strServiceData += (char)((tm_t >> 16) & 0xff);
  strServiceData += (char)((tm_t >> 24) & 0xff);

  oAdvertisementData.addData(strServiceData);
  pAdvertising->setAdvertisementData(oAdvertisementData);
}

void setup() {
  pinMode(GPIO_NUM_37, INPUT_PULLUP);
  set_time();
  getLocalTime(&now,100);
  task_lcd();
  task_ble();
  M5.Axp.ScreenBreath(0);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_37, LOW);
  esp_deep_sleep_start();
}

void loop() {}