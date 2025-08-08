// Receiver Sketch: Wi-Fi JPEG Receiver Over TCP
// ECEN 435 University of Nebraska-Lincoln
// Network parameters come from config.h
// Author: Paul Scalise

#include "../configuration/config.h"
#include <WiFi.h>
#include <strings.h>

// 32 KB buffer for the incoming JPEG or other packet
static uint8_t packetBuf[32 * 1024];

WiFiServer tcpServer(DEST_PORT);

void setup() {
  Serial.begin(1000000);
  while (!Serial);

  // set up Soft-AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS, WIFI_CH, /* hidden */0, /* max connections */4);
  delay(500);

  tcpServer.begin();  // start listening

  Serial.printf("Team %d  AP IP = %s\n",
                TEAM_ID, WiFi.softAPIP().toString().c_str());
  Serial.printf("TCP server listening on port %u\n", DEST_PORT);
}

void loop() {
  WiFiClient client = tcpServer.available();
  if (!client) {
    delay(10);
    return;
  }

  Serial.println("Sender connected, keeping socket open…");

  while (client.connected()) {
    // 1) read 4-byte length header
    uint32_t expect = 0;
    size_t got = 0;
    while (got < 4 && client.connected()) {
      int n = client.read(((uint8_t*)&expect) + got, 4 - got);
      if (n > 0) got += n;
    }
    if (got < 4) break;  // peer hung up
    Serial.printf("Expecting %u bytes…\n", (unsigned)expect);

    // 2) read exactly that many bytes
    size_t received = 0;
    while (received < expect && client.connected()) {
      size_t avail = client.available();
      size_t toRead = std::min((size_t)(expect - received), avail);
      if (toRead == 0) continue;
      int n = client.read(packetBuf + received, toRead);
      if (n > 0) {
        received += n;
        Serial.printf("Got chunk %d bytes, %u/%u\n",
                      n, (unsigned)received, (unsigned)expect);
      }
    }
    if (received < expect) break;  // connection closed mid-frame

    // 3) process the packet in-place
    if (strncasecmp((const char*)packetBuf, "IMG", 3) == 0) {
      Serial.println(F("\n---- BEGIN JPEG HEX DUMP ----"));
      for (size_t i = 0; i < expect; i++) {
        Serial.printf("%02X ", packetBuf[i]);
      }
      Serial.println();
      Serial.println(F("----  END JPEG HEX DUMP  ----"));
    }
    else if (strncasecmp((const char*)packetBuf, "GPS", 3) == 0) {
      Serial.println("Received GPS data:");
      Serial.write(packetBuf, received);
    }
    else if (strncasecmp((const char*)packetBuf, "PRX", 3) == 0) {
      Serial.println("Received proximity data:");
      Serial.write(packetBuf, received);
    }
    Serial.println("Frame done, waiting for next…");
  }

  Serial.println("Sender disconnected, cleaning up.");
  client.stop();
}
