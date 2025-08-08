// Wi-Fi JPEG/GPS/PRX Transmitter Over ESP-NOW v2
// ECEN 435 University of Nebraska–Lincoln
// Author: Paul Scalise

//#include <algorithm>
#include <WiFi.h>
#include <esp_now.h>

#define ESP_UART        Serial2
#define ESP_UART_BAUD   115200
#define UART_RX_PIN     26
#define UART_TX_PIN     27

#define _inline static inline __attribute__((always_inline))

static const unsigned PACKET_GAP_US   = 6000;
static const size_t   MAX_PACKET_SIZE = 40000;
static uint8_t        packetBuf[MAX_PACKET_SIZE];
static size_t         packetLen;

// Your 8051 ESP MAC Address
static const uint8_t peerAddress[6] = {0xDC, 0xDA, 0x0C, 0x48, 0x96, 0xD0};
// ESPNOW v2 max payload per packet
static const size_t CHUNK_SIZE = ESP_NOW_MAX_DATA_LEN_V2;


// ESP-NOW send callback
void onDataSent(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  // (OPTIONAL) You can add communication back to the STM board
  // if you would like to acknowledge the sent image.
}

void ESP_NOW_Init(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("*** Error initializing ESP-NOW");
    while (true) { delay(1000); }
  }
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, peerAddress, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("*** Failed to add ESP-NOW peer");
    while (true) { delay(1000); }
  }
}

_inline void readBytes(uint8_t *dst, size_t N) {
  size_t got = 0;
  while (got < N) {
    if (ESP_UART.available()) {
      dst[got++] = ESP_UART.read();
    }
  }
  //Serial.printf("%c%c%c\n", dst[0], dst[1], dst[2]);
}

bool receivePacket() {
  readBytes(packetBuf, 3);
  packetLen = 3;

  if (memcmp(packetBuf, "IMG", 3) == 0) {
    // Read until JPEG EOI
    while (packetLen + 1 < MAX_PACKET_SIZE) {
      while (!ESP_UART.available());
      uint8_t b = ESP_UART.read();
      packetBuf[packetLen++] = b;
      if (packetBuf[packetLen-2] == 0xFF && packetBuf[packetLen-1] == 0xD9) {
        return true;
      }
    }
    return false;
  }
  else if (memcmp(packetBuf, "GPS", 3) == 0 ||
           memcmp(packetBuf, "PRX", 3) == 0) {
    // Read until newline
    while (packetLen < MAX_PACKET_SIZE) {
      while (!ESP_UART.available());
      uint8_t b = ESP_UART.read();
      packetBuf[packetLen++] = b;
      if (b == '\n') {
        return true;
      }
    }
    return false;
  }
  else {
    return false;
  }
}

bool sendBufferESPNOW(const uint8_t *buf, size_t len) {
  size_t off = 0;
  while (off < len) {
    size_t n = std::min(CHUNK_SIZE, len - off);
    if (esp_now_send(peerAddress, buf + off, n) != ESP_OK) {
      return false;
    }
    off += n;
    //delayMicroseconds(PACKET_GAP_US);
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  ESP_UART.begin(ESP_UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  while (!ESP_UART);

  ESP_NOW_Init();
}

void loop() {
  if (receivePacket()) {
    Serial.printf("Got %3.3s → %u bytes\n",
                  packetBuf, (unsigned)packetLen);
    if (!sendBufferESPNOW(packetBuf, packetLen)) {
      Serial.println("*** Error sending over ESP-NOW");
    }
  }
  delay(10);
}
