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

void clear_screen() {
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0, 0);
}

void task_ble() {
  BLEDevice::init("takuya");                       // デバイスを初期化
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
  M5.Lcd.print("Wifi connected.(^_^)");
  configTime(9 * 3600, 0, URL_NTP_SERVER);

  struct tm _tm;
  getLocalTime(&_tm);

  RTC_TimeTypeDef _rtc_struct_time;
  _rtc_struct_time.Hours = _tm.tm_hour;
  _rtc_struct_time.Minutes = _tm.tm_min;
  _rtc_struct_time.Seconds = _tm.tm_sec;
  M5.Rtc.SetTime(&_rtc_struct_time);

  RTC_DateTypeDef _rtc_struct_date;
  _rtc_struct_date.WeekDay = _tm.tm_wday;
  _rtc_struct_date.Month = _tm.tm_mon + 1;
  _rtc_struct_date.Date = _tm.tm_mday;
  _rtc_struct_date.Year = _tm.tm_year + 1900;
  M5.Rtc.SetData(&_rtc_struct_date);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void setup_lcd() {
  M5.Axp.ScreenBreath(10);  // 画面の輝度を下げる
  M5.Lcd.setRotation(1);    // 左を上にする
  M5.Lcd.setTextSize(1);    // 文字サイズを2にする
}

struct tm get_time_rtc() {
  RTC_TimeTypeDef rtc_struct_time;
  RTC_DateTypeDef rtc_struct_date;
  M5.Rtc.GetTime(&rtc_struct_time);
  M5.Rtc.GetData(&rtc_struct_date);

  struct tm _tm;
  _tm.tm_hour = rtc_struct_time.Hours;
  _tm.tm_mday = rtc_struct_date.Date;
  _tm.tm_min = rtc_struct_time.Minutes;
  _tm.tm_mon = rtc_struct_date.Month - 1;
  _tm.tm_sec = rtc_struct_time.Seconds;
  _tm.tm_year = rtc_struct_date.Year - 1900;
  _tm.tm_isdst = -1;
  return _tm;
}
void show_time() {
  struct tm _tm = get_time_rtc();
  clear_screen();
  M5.Lcd.printf("seq: %d\n", seq);
  M5.Lcd.printf("time_t:%ld\n", mktime(&_tm));
  M5.Lcd.printf("%d/%d/%d %d:%d:%d'\n", _tm.tm_year + 1900, _tm.tm_mon + 1,
                _tm.tm_mday, _tm.tm_hour, _tm.tm_min, _tm.tm_sec);
};

void setAdvData(BLEAdvertising *p_advertising) {
  struct tm _tm = get_time_rtc();
  if (seq == 0) {
    _tm.tm_hour += 9;  // TODO この時だけなぜかUTC?になる｡
  }

  time_t _time = mktime(&_tm);

  BLEAdvertisementData advertisement_data = BLEAdvertisementData();

  advertisement_data.setFlags(
      0x06);  // BR_EDR_NOT_SUPPORTED | LE General Discoverable Mode
  std::string user_name = "takuya";

  std::string str_service_data = "";

  str_service_data += (uint8_t)8;  // 長さ
  str_service_data +=
      (uint8_t)0xff;  // AD Type 0xFF: Manufacturer specific data
  str_service_data += (uint8_t)0xff;  // Test manufacture ID low byte
  str_service_data += (uint8_t)0xff;  // Test manufacture ID high byte
  str_service_data += (uint8_t)seq++;
  str_service_data += (uint8_t)(_time & 0xff);
  str_service_data += (uint8_t)((_time >> 8) & 0xff);
  str_service_data += (uint8_t)((_time >> 16) & 0xff);
  str_service_data += (uint8_t)((_time >> 24) & 0xff);
  str_service_data += (uint8_t)user_name.length();
  
  for (int i = 0; i < user_name.length(); i++) {
    str_service_data += (uint8_t)user_name[i];
  }

  advertisement_data.addData(str_service_data);
  p_advertising->setAdvertisementData(advertisement_data);
}

void setup() {
  M5.begin();
  setup_lcd();
  pinMode(GPIO_NUM_37, INPUT_PULLUP);
  set_time();
  show_time();
  task_ble();
  M5.Axp.ScreenBreath(0);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_37, LOW);
  esp_deep_sleep_start();
}

void loop() {}