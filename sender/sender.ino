// Wi-Fi JPEG/GPS/PRX Transmitter Over ESP-NOW v1
// ECEN 435 University of Nebraska–Lincoln
// Author: Paul Scalise

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <algorithm>
//#include "jpeg_test.h"  // For debugging purposes. Found in ../validation/sender/
#include "stdbool.h"

#define _inline static inline __attribute__((always_inline))

#define ESP_UART_BAUD   115200
#define ESP_UART        Serial

static const unsigned      PACKET_GAP_US   = 6000;
static const size_t        MAX_PACKET_SIZE = 32 * 1024;
static uint8_t             packetBuf[MAX_PACKET_SIZE];
static size_t              packetLen;

// Replace with your receiver’s MAC
static uint8_t peerAddress[6] = {0x48, 0x3F, 0xDA, 0x9D, 0x43, 0x6C};
// ESPNOW v1 max payload per packet
static const size_t CHUNK_SIZE = 250;


void onDataSent(uint8_t *mac_addr, uint8_t status) {
  // optionally ACK back to STM32
  // ESP_UART.write(status);
}

_inline void readHeaderTypeBytes(uint8_t *dst, size_t N) {
  size_t got = 0;
  while (got < N) {
    if (ESP_UART.available()) {
      dst[got++] = ESP_UART.read();
    }
  }
}

bool receivePacket() {
  readHeaderTypeBytes(packetBuf, 3);
  packetLen = 3;

  if (memcmp(packetBuf, "IMG", 3) == 0) {
    // Read until JPEG EOI marker
    while (packetLen + 1 < MAX_PACKET_SIZE) {
      if (!ESP_UART.available()) continue;
      uint8_t b = ESP_UART.read();
      packetBuf[packetLen++] = b;
      if (packetBuf[packetLen - 2] == 0xFF && packetBuf[packetLen - 1] == 0xD9){
        return true;
      }
    }
    return false;
  }
  else if (memcmp(packetBuf, "GPS", 3) == 0 || memcmp(packetBuf, "PRX", 3) == 0) {
    // Read until newline
    while (packetLen < MAX_PACKET_SIZE) {
      if (!ESP_UART.available()) continue;
      uint8_t b = ESP_UART.read();
      packetBuf[packetLen++] = b;
      if (b == '\n') return true;
    }
    return false;
  }
  return false;
}

bool sendBufferESPNOW(uint8_t *buf, size_t len) {
  size_t off = 0;
  while (off < len) {
    size_t n = std::min(CHUNK_SIZE, len - off);
    if (esp_now_send(peerAddress, (uint8_t*)buf + off, n) != 0) {
      return false;
    }
       for (size_t i = 0; i < n; ++i) {
    Serial.printf("%02X ", buf[off + i]);
  }
    off += n;
  }
  return true;
}

void ESP_NOW_Init(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println(F("*** Error initializing ESP-NOW"));
    while (true) delay(1000);
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(onDataSent);

  if (esp_now_add_peer((uint8_t*)peerAddress, ESP_NOW_ROLE_COMBO, 0, NULL, 0) != 0) {
    while (true){
      Serial.println(F("*** Failed to add ESP-NOW peer"));
      delay(1000);
    }
  }
}

void setup() {
  ESP_UART.begin(ESP_UART_BAUD);
  while (!ESP_UART);
  ESP_NOW_Init();
}

void loop() {
  if (receivePacket()) {
    if (!sendBufferESPNOW(packetBuf, packetLen)) {
      Serial.println(F("*** Send error"));
    }
  }
  yield();
}
