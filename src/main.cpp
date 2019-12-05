#include <M5StickC.h>
#include <WiFi.h>
#include <Wire.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "esp_sleep.h"
#include "sys/time.h"

#define T_PERIOD 5
// Transmission period
#define S_PERIOD 1                 // Silent period
RTC_DATA_ATTR static uint8_t seq;  // remember number of boots in RTC Memory

// struct tm now;
void setAdvData(BLEAdvertising *pAdvertising);

const char *SSID = "すずのiphone";
const char *PASSWORD = "ryo5502gga";
const char *URL_NTP_SERVER = "ntp.jst.mfeed.ad.jp";

void print_error(String str) {
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("%s", str.c_str());
  delay(1000);
}
void task_ble() {
  BLEDevice::init("M5StickC");                     // デバイスを初期化
  BLEServer *pServer = BLEDevice::createServer();  // サーバーを生成
  BLEAdvertising *pAdvertising =
      pServer->getAdvertising();  // アドバタイズオブジェクトを取得
  setAdvData(pAdvertising);
  pAdvertising->start();   // アドバタイジングデーターをセット
  delay(T_PERIOD * 1000);  // T_PERIOD秒アドバタイズする
  pAdvertising->stop();
}

void set_time() {
  if (seq != 0) return;

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.print("Wifi connected.");
  configTime(9 * 3600, 0, URL_NTP_SERVER);


  struct tm _tm;
  getLocalTime(&_tm);
  // if(!getLocalTime(&_tm, 100)) {
  //   print_error("getLocalTime faild >_<");
  //   return;
  // }

  RTC_TimeTypeDef _rtc_time_struct;
  _rtc_time_struct.Hours = _tm.tm_hour;
  _rtc_time_struct.Minutes = _tm.tm_min;
  _rtc_time_struct.Seconds = _tm.tm_sec;
  M5.Rtc.SetTime(&_rtc_time_struct);

  RTC_DateTypeDef _rtc_date_struct;
  _rtc_date_struct.WeekDay = _tm.tm_wday;
  _rtc_date_struct.Month = _tm.tm_mon + 1;
  _rtc_date_struct.Date = _tm.tm_mday;
  _rtc_date_struct.Year = _tm.tm_year + 1900;
  M5.Rtc.SetData(&_rtc_date_struct);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void setup_lcd() {
  M5.Axp.ScreenBreath(10);  // 画面の輝度を下げる
  M5.Lcd.setRotation(1);    // 左を上にする
  M5.Lcd.setTextSize(1);    // 文字サイズを2にする
}

struct tm get_time_rtc(){
  RTC_TimeTypeDef rtc_time_struct;
  RTC_DateTypeDef rtc_date_struct;
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
  return _tm;
}
void show_time() {
  struct tm _tm = get_time_rtc();;
  //getLocalTime(&_tm, 100);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("seq: %d\r\n", seq);
  M5.Lcd.printf("tm:\r\n");
  M5.Lcd.printf("%d/%d/%d %d:%d:%d'\r\n", _tm.tm_year, _tm.tm_mon,
                _tm.tm_mday, _tm.tm_hour, _tm.tm_min, _tm.tm_sec);
};

void setAdvData(BLEAdvertising *pAdvertising) {
  struct tm _tm = get_time_rtc();

  time_t _time = mktime(&_tm);  //&nowにするといい値?

  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

  oAdvertisementData.setFlags(
      0x06);  // BR_EDR_NOT_SUPPORTED | LE General Discoverable Mode

  std::string strServiceData = "";
  strServiceData += (char)8;     // 長さ
  strServiceData += (char)0xff;  // AD Type 0xFF: Manufacturer specific data
  strServiceData += (char)0xff;  // Test manufacture ID low byte
  strServiceData += (char)0xff;  // Test manufacture ID high byte
  strServiceData += (char)seq++;
  strServiceData += (char)(_time & 0xff);
  strServiceData += (char)((_time >> 8) & 0xff);
  strServiceData += (char)((_time >> 16) & 0xff);
  strServiceData += (char)((_time >> 24) & 0xff);

  oAdvertisementData.addData(strServiceData);
  pAdvertising->setAdvertisementData(oAdvertisementData);
}

void setup() {
  M5.begin();
  setup_lcd();
  pinMode(GPIO_NUM_37, INPUT_PULLUP);
  set_time();
  // getLocalTime(&now,100);
  show_time();
  task_ble();
  M5.Axp.ScreenBreath(0);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_37, LOW);
  esp_deep_sleep_start();
}

void loop() {}