// Transmitter Sketch: Wi-Fi JPEG Transmitter Over TCP
// ECEN 435 University of Nebraska-Lincoln
// Network parameters come from config.h
// Author: Paul Scalise (modified for persistent TCP)

#include "../configuration/config.h"
#include <algorithm>
#undef swap
#include <WiFi.h>
#include <Wire.h>

#define DEST_IP       IPAddress(192, 168, 4, 1)
#define DEST_PORT     5001

#define ESP_UART        Serial2
#define ESP_UART_BAUD   115200
#define UART_RX_PIN     26
#define UART_TX_PIN     27

static const unsigned PACKET_GAP_US   = 6000;
constexpr size_t      CHUNK_SIZE      = 1460;
static const size_t   MAX_PACKET_SIZE = 50000;
static uint8_t        packetBuf[MAX_PACKET_SIZE];
static size_t         packetLen;

WiFiClient tcpClient;

static void readBytes(uint8_t *dst, size_t N) {
  size_t got = 0;
  while (got < N) {
    if (ESP_UART.available()) {
      dst[got++] = ESP_UART.read();
    }
  }
}

bool receivePacket() {
  readBytes(packetBuf, 3);
  packetLen = 3;

  if (memcmp(packetBuf, "IMG", 3) == 0) {
    // read until JPEG EOI
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
           memcmp(packetBuf, "PRO", 3) == 0) {
    // read until newline
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

bool sendBufferTCP(const uint8_t *buf, size_t len) {
  if (!tcpClient.connected()) {
    return false;
  }

  // send the 4-byte length header
  if (tcpClient.write((const uint8_t*)&len, 4) != 4) {
    return false;
  }
  delayMicroseconds(PACKET_GAP_US);

  // send image in chunks
  size_t off = 0;
  while (off < len) {
    size_t n = std::min(CHUNK_SIZE, len - off);
    if (tcpClient.write(buf + off, n) != (int)n) {
      return false;
    }
    off += n;
    delayMicroseconds(PACKET_GAP_US);
  }
  return true;
}

static void connectToAP() {
  WiFi.begin(AP_SSID, AP_PASS);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(200);
  }
  Serial.println(" Done");
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  ESP_UART.begin(ESP_UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  connectToAP();

  // open a single persistent TCP connection
  Serial.println("Connecting TCP to receiver...");
  if (!tcpClient.connect(DEST_IP, DEST_PORT)) {
    Serial.println("Initial TCP connect failed");
  } else {
    Serial.println("TCP connected!");
  }

}

void loop() {
  if (receivePacket()) {
    Serial.printf("Got %3.3s packet → %u bytes\n",
                  packetBuf, (unsigned)packetLen);

    if (!sendBufferTCP(packetBuf, packetLen)) {
      Serial.println("Error sending packet, closing socket");
      tcpClient.stop();
      // try to reconnect immediately
      if (tcpClient.connect(DEST_IP, DEST_PORT)) {
        Serial.println("Re-connected!");
      }
    }
  }
  delay(10);
}
