#include <M5StickC.h>
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

time_t tm_t;
void setAdvData(BLEAdvertising *, struct tm *);

void task_ble(void *pvParameters) {
  struct tm *now = (struct tm *)pvParameters;
  BLEDevice::init("AmbientEnv-02");                // デバイスを初期化
  BLEServer *pServer = BLEDevice::createServer();  // サーバーを生成
  BLEAdvertising *pAdvertising =
      pServer->getAdvertising();  // アドバタイズオブジェクトを取得
  setAdvData(pAdvertising, now);

  pAdvertising->start();   // アドバタイジングデーターをセット
  delay(T_PERIOD * 1000);  // T_PERIOD秒アドバタイズする
  pAdvertising->stop();
  seq++;  // シーケンス番号を更新

  M5.Axp.ScreenBreath(0);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_37, LOW);
  esp_deep_sleep_start();
}

void task_lcd(void *pvParameters) {
  struct tm *now = (struct tm *)pvParameters;

  M5.Axp.ScreenBreath(10);   // 画面の輝度を下げる
  M5.Lcd.setRotation(3);     // 左を上にする
  M5.Lcd.setTextSize(2);     // 文字サイズを2にする
  M5.Lcd.fillScreen(BLACK);  // 背景を黒にする
  M5.Lcd.setCursor(0, 0, 1);
  M5.Lcd.printf("seq: %d\r\n", seq);
  M5.Lcd.printf("tm: %d:%d:%d'\r\n", now->tm_hour, now->tm_min, now->tm_sec);
}

void setAdvData(BLEAdvertising *pAdvertising, struct tm *now) {
  tm_t = mktime(now);

  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

  oAdvertisementData.setFlags(
      0x06);  // BR_EDR_NOT_SUPPORTED | LE General Discoverable Mode

  std::string strServiceData = "";
  strServiceData += (char)8;     // 長さ
  strServiceData += (char)0xff;  // AD Type 0xFF: Manufacturer specific data
  strServiceData += (char)0xff;  // Test manufacture ID low byte
  strServiceData += (char)0xff;  // Test manufacture ID high byte
  strServiceData += (char)seq;
  strServiceData += (char)(tm_t & 0xff);
  strServiceData += (char)((tm_t >> 8) & 0xff);
  strServiceData += (char)((tm_t >> 16) & 0xff);
  strServiceData += (char)((tm_t >> 24) & 0xff);

  oAdvertisementData.addData(strServiceData);
  pAdvertising->setAdvertisementData(oAdvertisementData);
}

void setup() {
  M5.begin();
  Wire.begin();
  struct tm now;

  pinMode(GPIO_NUM_37, INPUT_PULLUP);
  getLocalTime(&now);
  xTaskCreatePinnedToCore(task_ble, "task_ble", 10000, &now, 1, NULL, 0);
  xTaskCreatePinnedToCore(task_lcd, "task_lcd", 10000, &now, 2, NULL, 0);
}

void loop() {}