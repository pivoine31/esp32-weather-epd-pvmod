/* Client side utility declarations for esp32-weather-epd.
 * Copyright (C) 2022-2023  Luke Marzen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __CLIENT_UTILS_H__
#define __CLIENT_UTILS_H__

#include <Arduino.h>
#include "api_response.h"
#include "config.h"
#ifdef USE_HTTP
  #include <WiFiClient.h>
#else
  #include <WiFiClientSecure.h>
#endif

#ifdef WEB_SVR
/*
 * WEB server fct and vars called from outside
 */
void retrieve_config ( void );
void web_svr_setup   ( void );
int  net_loop        ( void );

extern String VLoc[];
extern String VLat[];
extern String VLon[];
extern float  PopTh;
extern long   SleepDly;
extern int    BedTime;
extern int    WakeTime;
extern int    HourlyNb;
extern int    MinRefTim;
extern int    MaxActTim;
extern int    defloc;

/*
 * External fonctions and vars called by WEB server
 *
 * - located in main.cpp
 */
void restart_wdg     ( void );
void do_deep_sleep   ( uint64_t );
void beginDeepSleep  ( unsigned long &startTime, tm *timeInfo );
extern unsigned long startTime;
extern int           SilentErr;
/*
 * - located in renderer.cpp
 */
void drawWebIcon     ( int active );
#endif //WEB_SVR

wl_status_t startWiFi(int &wifiRSSI, int web_mode);
void        killWiFi();

#ifdef USE_HTTP
  int getOWMonecall(WiFiClient &client, owm_resp_onecall_t &r);
  int getOWMairpollution(WiFiClient &client, owm_resp_air_pollution_t &r, int64_t curdt);
#else
  int getOWMonecall(WiFiClientSecure &client, owm_resp_onecall_t &r);
  int getOWMairpollution(WiFiClientSecure &client, owm_resp_air_pollution_t &r, int64_t curdt);
#endif

#endif

