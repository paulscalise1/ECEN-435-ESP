// ESPNOWv2 JPEG/GPS/PRX Receiver
// ECEN 435 University of Nebraska–Lincoln
// Author: Paul Scalise

#include <WiFi.h>
//#include <ESP8266WiFi.h>
#include <esp_now.h>
//#include <espnow.h>
#include <strings.h>
#include <Arduino.h>
#include <esp_wifi.h>
#include <TJpg_Decoder.h>

static const size_t RX_BUFFER_SIZE = 32 * 1024;
static uint8_t  packetBuf[RX_BUFFER_SIZE];
static size_t   packetLen = 0;

bool tjpg_render_callback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  size_t byteCount = size_t(w) * h * 2;
  Serial.write((uint8_t*)bitmap, byteCount); // Serial.write
  return true;
}

void decodeJPEGtoRGB565(const uint8_t* jpegBuf, size_t jpegLen) { // row major, left to right, top to bottom
  if (!TJpgDec.drawJpg(0, 0, jpegBuf, jpegLen)) {
     Serial.println(F("JPEG decode failed!"));
  }
}

void printMAC(){
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  baseMac[0], baseMac[1], baseMac[2],
                  baseMac[3], baseMac[4], baseMac[5]);
  } else {
    Serial.println("*** Failed to read MAC address");
  }
}

void ESP_NOW_Init(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    while (true) { 
    	Serial.println("*** Error initializing ESP-NOW");
    	delay(1000);
    }
  }
  esp_now_register_recv_cb(onDataRecv);
}

void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len){
  if (packetLen + len > RX_BUFFER_SIZE) {
    packetLen = 0;
    return;
  }
  memcpy(packetBuf + packetLen, data, len);
  packetLen += len;

  // JPEG frames start “IMG” and end 0xFF 0xD9
  if (packetLen >= 3 && strncasecmp((const char*)packetBuf, "IMG", 3) == 0) {
    if (packetLen >= 2 && packetBuf[packetLen-2] == 0xFF && packetBuf[packetLen-1] == 0xD9) {
    	/*
      Serial.println(F("\n---- BEGIN JPEG HEX DUMP ----"));
      for (size_t i = 0; i < packetLen; i++) {
        Serial.printf("%02X ", packetBuf[i]);
      }
      Serial.println();
      Serial.println(F("----  END JPEG HEX DUMP  ----"));
      Serial.println("Frame done, waiting for next…");
      */
      decodeJPEGtoRGB565(packetBuf + 3, expect - 3);
      packetLen = 0;
    }
  }
  // GPS frames start “GPS” and end with '\n'
  else if (packetLen >= 3 && strncasecmp((const char*)packetBuf, "GPS", 3) == 0) {
    if (packetBuf[packetLen-1] == '\n') {
      Serial.println("Received GPS data:");
      Serial.write(packetBuf, packetLen);
      Serial.println("Frame done, waiting for next…");
      packetLen = 0;
    }
  }
  // Proximity frames start “PRX” and end with '\n'
  else if (packetLen >= 3 && strncasecmp((const char*)packetBuf, "PRX", 3) == 0) {
    if (packetBuf[packetLen-1] == '\n') {
      Serial.println("Received proximity data:");
      Serial.write(packetBuf, packetLen);
      Serial.println("Frame done, waiting for next…");
      packetLen = 0;
    }
  }
  // else: still waiting for more chunks
}

void setup() {
  Serial.begin(1000000);
  while (!Serial);
  ESP_NOW_Init();
  //printMAC(); // You only need to record this once.
}

void loop() {
  yield();
}

