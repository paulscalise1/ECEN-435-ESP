// Wi-Fi JPEG/GPS/PRX Receiver Over ESP-NOW v1
// Outputs a stream of RGB565 in 16x8 tile rowmajor order. (See tjpg_render_callback for more info)
// ECEN 435 University of Nebraska–Lincoln
// Author: Paul Scalise

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <TJpg_Decoder.h>

#define DEBUG 1

static const size_t RX_BUFFER_SIZE = 15 * 1024;
static uint8_t  packetBuf[RX_BUFFER_SIZE];
static size_t   packetLen = 0;


// Each time this is invoked, it will print 16 by 8 (X x Y) rectangle tile, starting at the top left of the display, moving left to right, top to bottom.
bool tjpg_render_callback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (w == 0 || h == 0) {
    return true;
  }

  if (DEBUG == 1){
    //Serial.printf("x: %d | y: %d | w: %d | h: %d\n", x, y, w, h);
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
      Serial.print(bitmap[i], HEX);
      Serial.print(' ');
      ESP.wdtFeed();
    }
  } else {
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
      Serial.print(bitmap[i]);
      ESP.wdtFeed();
    }
  }
  return true;  // continue decoding
}

void decodeJPEGtoRGB565(const uint8_t* jpegBuf, size_t jpegLen) {
  if (DEBUG == 1){
      Serial.println(F("\n--- Attempting JPEG decode... ---"));
  }

  JRESULT result = TJpgDec.drawJpg(0, 0, jpegBuf, jpegLen);

  if (result == JDR_OK) {
    if (DEBUG == 1){
      Serial.println(F("\n--- JPEG DECODE SUCCESSFUL ---"));
    }
  } else {
    Serial.printf("\n*** JPEG DECODE FAILED! Error code: %d ***\n", result);
  }
}

void OnDataRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (packetLen + len > RX_BUFFER_SIZE) {
    packetLen = 0; return;
  }
  memcpy(packetBuf + packetLen, data, len);
  packetLen += len;

  // JPEG frames: start "IMG", end 'FFD9'
  if (packetLen >= 5 && strncasecmp((char*)packetBuf, "IMG", 3) == 0 && packetBuf[packetLen-2] == 0xFF && packetBuf[packetLen-1] == 0xD9) {
    //Serial.printf("\nComplete 'IMG' packet received. Total size: %u bytes.\n", packetLen);
    decodeJPEGtoRGB565(packetBuf + 3, packetLen - 3);
    packetLen = 0;
  } else if (packetLen >= 3 && strncasecmp((char*)packetBuf, "GPS", 3) == 0) {  // GPS frames: start "GPS", end '\n'
    if (packetBuf[packetLen-1] == '\n') {
      Serial.write(packetBuf, packetLen - 1);
      packetLen = 0;
    }
  } else if (packetLen >= 3 && strncasecmp((char*)packetBuf, "PRX", 3) == 0) {  // Proximity frames: start "PRX", end '\n'
    if (packetBuf[packetLen-1] == '\n') {
      Serial.write(packetBuf, packetLen - 1);
      packetLen = 0;
    }
  }
}

void ESP_NOW_Init(){
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) { ESP.restart(); }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(OnDataRecv);
}

void jpeg_decode_init(){
  TJpgDec.setSwapBytes(false);
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tjpg_render_callback);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  ESP_NOW_Init();
  jpeg_decode_init();
}

void loop() {

}