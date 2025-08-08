/*************  config.h  ****************/
#pragma once

#define TEAM_ID      1

#define WIFI_CH   ((TEAM_ID % 3) == 1 ? 1 : ((TEAM_ID % 3) == 2 ? 6 : 11))

#define STRINGIFY_(x) #x
#define STRINGIFY(x)  STRINGIFY_(x)

#define AP_SSID   "CAM_AP_"  STRINGIFY(TEAM_ID)
#define AP_PASS   "class25_" STRINGIFY(TEAM_ID)
#define DEST_PORT (5000 + TEAM_ID)
