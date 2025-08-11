// ESPNOWv2 JPEG/GPS/PRX Receiver
// ECEN 435 University of Nebraska–Lincoln
// Author: Paul Scalise

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <TJpg_Decoder.h>
#include <Arduino.h>

static const size_t RX_BUFFER_SIZE = 7 * 1024;
static uint8_t  packetBuf[RX_BUFFER_SIZE];
static size_t   packetLen = 0;


bool tjpg_render_callback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  size_t byteCount = size_t(w) * h * 2;
  //Serial.write((uint8_t*)bitmap, byteCount);

  for (size_t i = 0; i < byteCount; ++i) {
    Serial.printf("%02X ", bitmap[i]);
  }
  return true;
}

void decodeJPEGtoRGB565(const uint8_t* jpegBuf, size_t jpegLen) {
  TJpgDec.setCallback(tjpg_render_callback);
  if (!TJpgDec.drawJpg(0, 0, jpegBuf, jpegLen)) {
    Serial.println(F("*** JPEG decode failed!"));
  }
}

void printMAC() {
  Serial.println(WiFi.macAddress());
}

void OnDataRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  // Accumulate into packetBuf
  if (packetLen + len > RX_BUFFER_SIZE) {
    packetLen = 0;
    return;
  }
  memcpy(packetBuf + packetLen, data, len);
  packetLen += len;

  for (size_t i = 0; i < packetLen; ++i) {
        Serial.printf("%02X ", packetBuf[i]);
      }
      

  // JPEG frames: start "IMG", end 0xFF 0xD9
  if (packetLen >= 3 && strncasecmp((char*)packetBuf, "IMG", 3) == 0) {
    if (packetLen >= 2 && packetBuf[packetLen-2] == 0xFF && packetBuf[packetLen-1] == 0xD9) {
      // decode everything after the "IMG" header

      decodeJPEGtoRGB565(packetBuf + 3, packetLen - 3);
      packetLen = 0;
    }
  }
  // GPS frames: start "GPS", end '\n'
  else if (packetLen >= 3 && strncasecmp((char*)packetBuf, "GPS", 3) == 0) {
    if (packetBuf[packetLen-1] == '\n') {
      Serial.write(packetBuf, packetLen - 1);
      packetLen = 0;
    }
  }
  // Proximity frames: start "PRX", end '\n'
  else if (packetLen >= 3 && strncasecmp((char*)packetBuf, "PRX", 3) == 0) {
    if (packetBuf[packetLen-1] == '\n') {
      Serial.write(packetBuf, packetLen - 1);
      packetLen = 0;
    }
  }
  // else: keep buffering
}

void ESP_NOW_Init(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    while (true) {
      delay(1000); 
      Serial.println(F("*** Error initializing ESP-Now"));
    }
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(OnDataRecv);
}

void setup() {
  Serial.begin(1000000);
  while (!Serial);

  //printMAC();      // Run this once to obtain the MAC address.
  ESP_NOW_Init();
}

void loop() {
  yield();
}
