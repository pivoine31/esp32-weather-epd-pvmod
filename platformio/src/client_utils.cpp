/* Client side utilities for esp32-weather-epd.
 * Copyright (C) 2022-2024  Luke Marzen
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

// built-in C++ libraries
#include <cstring>
#include <vector>

// arduino/esp32 libraries
#include <Arduino.h>
#include <esp_sntp.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <time.h>
#include <WiFi.h>

// additional libraries
#include <Adafruit_BusIO_Register.h>
#include <ArduinoJson.h>

// header files
#include "config.h"  // before "_locale.h"
#include "_locale.h"
#include "api_response.h"
#include "aqi.h"
#include "client_utils.h"
#include "display_utils.h"
#include "renderer.h"
#ifndef USE_HTTP
  #include <WiFiClientSecure.h>
#endif

#ifdef USE_HTTP
  static const uint16_t OWM_PORT = 80;
#else
  static const uint16_t OWM_PORT = 443;
#endif

#ifdef WEB_SVR
/*****************************************************/
/* WEB server management                             */
/*****************************************************/

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#ifdef WEB_HIDE_PWD
// Password not displayed in WIFI WEB page
#define TAGPWD "password"
#else
// Password displayed in WIFI WEB page
#define TAGPWD "text"
#endif

AsyncWebServer server(80);
Preferences    preferences;

/*
 * Parameters referenced from Web pages
 */
#define MX_LOC 9
#define MX_SSI 6

// Current Weather parameters (Nxxx = parm name, Vxxx = parm value)
String Lchecked[MX_LOC] = { "checked", "", "", "", "", "", "", "", "" };
String VLoc[MX_LOC]     = { DEFCITY, "", "", "", "", "", "", "", "" };
String VLat[MX_LOC]     = { DEFLAT, "", "", "", "", "", "", "", "" };
String VLon[MX_LOC]     = { DEFLON, "", "", "", "", "", "", "", "" };
String NLoc[MX_LOC]     = { "loc1", "loc2", "loc3", "loc4", "loc5", "loc6", "loc7", "loc8", "loc9" };
String NLat[MX_LOC]     = { "lat1", "lat2", "lat3", "lat4", "lat5", "lat6", "lat7", "lat8", "lat9" };
String NLon[MX_LOC]     = { "lon1", "lon2", "lon3", "lon4", "lon5", "lon6", "lon7", "lon8", "lon9" };

// Current Wifi parameters (Nxxx = parm name, Lxxx = parm value)
String Schecked[MX_SSI] = { "checked", "", "", "", "", "" };
String VSsi[MX_SSI]     = { WIFI_SSI1, "", "", "", "", "" };
String VPwd[MX_SSI]     = { WIFI_PWD1, "", "", "", "", "" };
String NSsi[MX_SSI]     = { "ssi1", "ssi2", "ssi3", "ssi4", "ssi5", "ssi6" }; 
String NPwd[MX_SSI]     = { "pwd1", "pwd2", "pwd3", "pwd4", "pwd5", "pwd6" }; 

// Current location
String NDloc = "DefLoc";
String DefLoc; 
int    defloc;

// POP threshold
#define NM_THR "PopTh"
float  PopTh = PRECIP_THRESHOLD;

// BED time
#define NM_BED "BedTim"
int    BedTime = DEFBED;

// WAKE time
#define NM_WAK "WakTim"
int    WakeTime = DEFWAKE;

// SLEEP delay
#define NM_SLP "SlpDly"
long   SleepDly = DEFSLEEP;

// Hourly Number in graph
#define NM_HNB "HourNb"
int    HourlyNb = DEFHOURNB;

// Minimum refresh interval
#define NM_MRT "MinRef"
int    MinRefTim = DEF_MINREF_TIM;

// Maximum Web server active time
#define NM_MAT "MajAct"
int    MaxActTim = DEF_MAXACT_TIM;

// WIFI Per AP timeout
#define NM_WAT "WifiAPto"
unsigned int  WifiAPto = DEF_AP_TIMEOUT;

// WIFI global timeout
#define NM_WGT "WifiGLto"
unsigned long WifiTimeout = DEF_WIFI_TIMEOUT;

// WIFI global timeout
#define NM_HTO "HttpTo"
unsigned int  HttpTimeout = DEF_HTTP_TIMEOUT;

// Init flag
#define NM_INIT "Inited"

/*
 * CLEAR_NVS
 *
 * Used for testing
 */
void clear_nvs ( void )
{
  // Clear all data
  preferences.clear();
}

/*
 * CLEAN_NVS
 *
 * Clear entries that are not in use / no longer exist
 * and recreates the others
 */
void clean_nvs ( void )
{
  int i;

  // Clear all data
  preferences.clear();

  // And restore what is in use
  for (i=0 ; i<MX_LOC ; i++)
  {
    preferences.putString(NLoc[i].c_str(), VLoc[i]);
    preferences.putString(NLat[i].c_str(), VLat[i]);
    preferences.putString(NLon[i].c_str(), VLon[i]);
  }
  preferences.putString(NDloc.c_str(), DefLoc);

  for (i=0 ; i<MX_SSI ; i++)
  {
    preferences.putString(NSsi[i].c_str(), VSsi[i]);
    preferences.putString(NPwd[i].c_str(), VPwd[i]);
  }

  preferences.putString(NM_THR, String(PopTh));
  preferences.putString(NM_BED, String(BedTime));
  preferences.putString(NM_WAK, String(WakeTime));
  preferences.putString(NM_SLP, String(SleepDly));
  preferences.putString(NM_HNB, String(HourlyNb));
  preferences.putString(NM_MRT, String(MinRefTim));
  preferences.putString(NM_MAT, String(MaxActTim));
  preferences.putString(NM_WAT, String(WifiAPto));
  preferences.putString(NM_WGT, String(WifiTimeout));

  preferences.putString(NM_INIT, "yes");
}

/*
 * CHECK_CONFIG
 *
 * Check and rectify parameters :
 * - When fetched from NVS
 * - When modified through Web
 * The entry 0 shall always be valid :
 * - Read Only for Wifi
 * - Restored from default values for Location when invalid/unset
 */
int check_config ( void )
{
  int rc = 0;

  // In entry not suitable default to entry 0
  if ( (VLoc[defloc] == "") ||
       (VLat[defloc] == "") ||
       (VLon[defloc] == "")  )
  {
    Lchecked[defloc] = "";
    defloc = 0;
    DefLoc = "0";
    preferences.putString(NDloc.c_str(), DefLoc);
    Lchecked[defloc] = "checked";
    rc = 1;
  }

  // In entry 0 not suitable, reinit
  if ( (VLoc[0] == "") ||
       (VLat[0] == "") ||
       (VLon[0] == "")  )
  {
    VLoc[0] = DEFCITY;
    VLat[0] = DEFLAT;
    VLon[0] = DEFLON;
    preferences.putString(NLoc[0].c_str(), VLoc[0]);
    preferences.putString(NLat[0].c_str(), VLat[0]);
    preferences.putString(NLon[0].c_str(), VLon[0]);
    rc = 1;
  }

  // In entry 0 not suitable, reinit
  if ( (VSsi[0] == "") ||
       (VPwd[0] == "")  )
  {
    VSsi[0] = WIFI_SSI1;
    VPwd[0] = WIFI_PWD1;
    preferences.putString(NSsi[0].c_str(), VSsi[0]);
    preferences.putString(NPwd[0].c_str(), VPwd[0]);
  }

  return rc;
}

/*
 * RETRIEVE_CONFIG
 *
 * Load and check parameters from non volatile storage
 */
void retrieve_config ( void )
{
  String s;
  int    i;
  
  // Retrieve data from remanent backup
  preferences.begin(HNAME, false);

  // Check if NVS has already been init (occurs only once, but avoid tons of log messages)
  s = preferences.getString(NM_INIT, "");
  if ( s == "" )
  {
    // Clear NVS and recreate default entries
    clean_nvs();
  }

  for (i=0 ; i<MX_LOC ; i++)
  {
    VLoc[i] = preferences.getString(NLoc[i].c_str(), (i ? "" : DEFCITY));
    VLat[i] = preferences.getString(NLat[i].c_str(), (i ? "" : DEFLAT));
    VLon[i] = preferences.getString(NLon[i].c_str(), (i ? "" : DEFLON));
  }
  DefLoc = preferences.getString(NDloc.c_str(), "0");
  defloc = DefLoc.toInt();

  for (i=0 ; i<MX_SSI ; i++)
  {
    VSsi[i] = preferences.getString(NSsi[i].c_str(), (i ? "" : WIFI_SSI1));
    VPwd[i] = preferences.getString(NPwd[i].c_str(), (i ? "" : WIFI_PWD1));
  }

  s = preferences.getString(NM_THR, String(PRECIP_THRESHOLD));
  PopTh = s.toFloat();
  s = preferences.getString(NM_BED, String(DEFBED));
  BedTime = s.toInt();
  s = preferences.getString(NM_WAK, String(DEFWAKE));
  WakeTime = s.toInt();
  s = preferences.getString(NM_SLP, String(DEFSLEEP));
  SleepDly = (long)s.toInt();
  s = preferences.getString(NM_HNB, String(DEFHOURNB));
  HourlyNb = s.toInt();
  s = preferences.getString(NM_MRT, String(DEF_MINREF_TIM));
  MinRefTim = s.toInt();
  s = preferences.getString(NM_MAT, String(DEF_MAXACT_TIM));
  MaxActTim = s.toInt();
  s = preferences.getString(NM_WAT, String(DEF_AP_TIMEOUT));
  WifiAPto = s.toInt();
  s = preferences.getString(NM_WGT, String(DEF_WIFI_TIMEOUT));
  WifiTimeout = s.toInt();

  check_config();
}

/*
 * RESET_WEATHER_CONFIG
 *
 * Reinit weather data to default values (as defined in config.h)
 */
void reset_weather_config ( void )
{
  int    i;
  
  for (i=0 ; i<MX_LOC ; i++)
  {
    VLoc[i] = i ? "" : DEFCITY;
    VLat[i] = i ? "" : DEFLAT;
    VLon[i] = i ? "" : DEFLON;
    preferences.putString(NLoc[i].c_str(), VLoc[i]);
    preferences.putString(NLat[i].c_str(), VLat[i]);
    preferences.putString(NLon[i].c_str(), VLon[i]);
  }

  defloc = 0;
  DefLoc = "0";
  preferences.putString(NDloc.c_str(), DefLoc);
}

/*
 * RESET_WIFI_CONFIG
 *
 * Reinit wifi data to default values (as define in config.h)
 */
void reset_wifi_config ( void )
{
  int    i;
  
  for (i=0 ; i<MX_SSI ; i++)
  {
    VSsi[i] = i ? "" : WIFI_SSI1;
    VPwd[i] = i ? "" : WIFI_PWD1;
    preferences.putString(NSsi[i].c_str(), VSsi[i]);
    preferences.putString(NPwd[i].c_str(), VPwd[i]);
  }

  check_config();
}

/*
 * RESET_PARM_CONFIG
 *
 * Reinit parm data to default values (as defined in config.h)
 */
void reset_parm_config ( void )
{
  PopTh = PRECIP_THRESHOLD;
  preferences.putString(NM_THR, String(PopTh));
  BedTime = DEFBED;
  preferences.putString(NM_BED, String(BedTime));
  WakeTime = DEFWAKE;
  preferences.putString(NM_WAK, String(WakeTime));
  SleepDly = DEFSLEEP;
  preferences.putString(NM_SLP, String(SleepDly));
  HourlyNb = DEFHOURNB;
  preferences.putString(NM_HNB, String(HourlyNb));
  MinRefTim = DEF_MINREF_TIM;
  preferences.putString(NM_MRT, String(MinRefTim));
  MaxActTim = DEF_MAXACT_TIM;
  preferences.putString(NM_MAT, String(MaxActTim));
  WifiAPto = DEF_AP_TIMEOUT;
  preferences.putString(NM_WAT, String(WifiAPto));
  WifiTimeout = DEF_WIFI_TIMEOUT;
  preferences.putString(NM_WGT, String(WifiTimeout));
}

/*
 * NOTFOUND
 *
 * Display "page not found" page
 */
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", W_PAGENOTFOUND);
}

// Check if remote user has been logged

#define MX_LOG 5

static int       remnb = 0;
static IPAddress remlog[MX_LOG];
static time_t    remtim[MX_LOG];

static int check_remoteLogged ( IPAddress *rem )
{
  int i;

  if ( !remnb )
    for (i=1 ; i<MX_LOG ; i++)
    {
      remlog[i] = (uint32_t)0;
      remtim[i] = (time_t)0;
    }

  for (i=0 ; i<remnb ; i++)
  {
    if ( remlog[i] == *rem )
    {
      Serial.print("IP address: ");
      Serial.print(*rem);
      Serial.print(" found at entry ");
      Serial.println(i);
      return 1;
    }
  }

  Serial.print("IP address: ");
  Serial.print(*rem);
  Serial.println(" not found");
  return 0;
}

static void log_remote ( IPAddress *rem )
{
  int i, f;

  if ( check_remoteLogged(rem) )
    return;

  if ( remnb < MX_LOG )
  {
    // Log new entry
    f = remnb++;
  }
  else
  {
    // Reuse the oldest entry
    time_t tm;
    tm = remtim[0];
    for (f=0, i=1 ; i<remnb ; i++)
      if ( remtim[i] < tm )
      {
        tm = remtim[i];
	f = i;
      }
  }

  remlog[f] = *rem;

  Serial.print("Backup IP address: ");
  Serial.print(*rem);
  Serial.print(" at entry ");
  Serial.println(f);
}

#ifdef WEBKEY
// Ask for a Web server access key (see WEBKEY in config.h)
#else
// No access key remap Invalid key menu to main page
#define RSP_INVAL_KEY MAIN_PAGE
#endif

const String KEY_PAGE = "<!DOCTYPE HTML><html><head>"
                  "<title>"+W_WEATHER+"</title>"
                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                  "<style>"
                  "label {display:inline-block;width:60px;text-align:center;}" 
                  "input {width:60px;text-align:center;}" 
                  "</style>"
                  "</head><body>"
                  "<h1>"+W_WEATHER+"</h1>"
                  "<form action=\"/get\" method=\"get\">"
                    "<label for=\"key\">"+W_KEY+"</label>"
                    "<input type=\"number\" id=\"key\" name=\"key\" value=\"""\" ><br><br>"
                    "<input type=\"submit\" value=\""+W_SUBMIT+"\">"
                  "</form>"
                  "</body></html>";
const String MAIN_PAGE = "<!DOCTYPE HTML><html><head>"
                  "<title>"+W_WEATHER+"</title>"
                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                  "</head><body>"
                  "<h1>"+W_WEATHER+"</h1>"
                  "<a href=\"/weather\">"+W_PARMM+"</a><br><br>"
                  "<a href=\"/wifi\">"+W_PARMW+"</a><br><br>"
                  "<a href=\"/parm\">"+W_PARMV+"</a><br><br>"
                  "</body></html>";
#ifdef WEBKEY
const String RSP_INVAL_KEY = "<!DOCTYPE HTML><html><head>"
                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
		  "<style>"
                  "label {display:inline-block;width:60px;text-align:center;}" 
                  "input {width:60px;text-align:center;}" 
		  "</style>"
                  "</head><body>"
                  "<h1>"+W_WEATHER+"</h1>"
                  "<form action=\"/get\" method=\"get\">"
                    "<p>"+W_INVALKEY+"</p>"
                    "<label for=\"key\">"+W_KEY+"</label>"
                    "<input type=\"number\" id=\"key\" name=\"key\" value=\"""\" ><br><br>"
                    "<input type=\"submit\" value=\""+W_SUBMIT+"\">"
                  "</form>"
                  "</body></html>";
#endif
const String RSP_INVAL_PARM = "<!DOCTYPE HTML><html><head>"
                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                  "</head><body>"
                  "<h1>"+W_WEATHER+"</h1>"
                  "<p>"+W_PARM_REINIT+"</p>"
                  "<a href=\"/weather\">"+W_RETURNM+"</a><br>"
                  "<a href=\"/wifi\">"+W_RETURNW+"</a><br><br>"
                  "<a href=\"/parm\">"+W_RETURNV+"</a><br><br>"
                  "<a href=\"/update\">"+W_UPDATE+"</a><br><br>"
                  "<a href=\"/exit\">"+W_EXIT+"</a><br>"
                  "</body></html>";
const String RSP_ACT_DONE = "<!DOCTYPE HTML><html><head>"
                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                  "</head><body>"
                  "<h1>"+W_WEATHER+"</h1>"
                  "<p>"+W_MOD_DONE+"</p>"
                  "<a href=\"/weather\">"+W_RETURNM+"</a><br><br>"
                  "<a href=\"/wifi\">"+W_RETURNW+"</a><br><br>"
                  "<a href=\"/parm\">"+W_RETURNV+"</a><br><br>"
                  "<a href=\"/update\">"+W_UPDATE+"</a><br><br>"
                  "<a href=\"/exit\">"+W_EXIT+"</a><br>"
                  "</body></html>";
const String RSP_TERMACT1a_DONE = "<!DOCTYPE HTML><html><head>"
                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                  "</head><body>"
                  "<h1>"+W_WEATHER+"</h1>"
                  "<p>"+W_ACT_DONE_MAJ;
const String RSP_TERMACT1b_DONE = " sec</p>"
                  "</body></html>";
const String RSP_TERMACT2_DONE = "<!DOCTYPE HTML><html><head>"
                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                  "</head><body>"
                  "<h1>"+W_WEATHER+"</h1>"
                  "<p>"+W_ACT_DONE_SLP+"</p>"
                  "</body></html>";

/*
 * PAGE_LOST
 *
 * Called when a page other than / is called from an unknow client
 */
void page_lost ( AsyncWebServerRequest *request )
{
#ifdef WEBKEY
  request->send(200, "text/html", RSP_INVAL_KEY);
#else
  IPAddress ip = request->client()->remoteIP();
  Serial.print("Validate IP address: ");
  Serial.println(ip);
  log_remote(&ip);

  request->send(200, "text/html", MAIN_PAGE);
#endif
}

/*
 * WEB_SVR_SETUP
 *
 * Entry point of WEB async server
 */
void web_svr_setup ( void )
{
  Lchecked[defloc] = "checked";

#ifdef WEBKEY
  // Init web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", KEY_PAGE);
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if ( request->hasParam("key")                    &&
        (request->getParam("key")->value() == WEBKEY) )
    {
      IPAddress ip = request->client()->remoteIP();
      Serial.print("Valid key from IP address: ");
      Serial.println(ip);
      log_remote(&ip);

      request->send(200, "text/html", MAIN_PAGE);
    }
    else
      request->send(200, "text/html", RSP_INVAL_KEY);
  });
#else
  server.on("/", HTTP_GET, [] (AsyncWebServerRequest *request) {
    IPAddress ip = request->client()->remoteIP();
    Serial.print("Validate IP address: ");
    Serial.println(ip);
    log_remote(&ip);

    request->send(200, "text/html", MAIN_PAGE);
  });
#endif

  // Send web page with input fields to client
  server.on("/weather", HTTP_GET, [](AsyncWebServerRequest *request){
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      request->send(200, "text/html",
      "<!DOCTYPE HTML><html><head>"
      "<style>"
      //".lloc {display:inline-block;width:60px;margin-left:10px;}" 
      ".lloc {display:inline-block;min-width:50px;width:auto;margin-left:10px;;margin-right:10px;}" 
      ".loc {width:130px;margin-top:8px;}" 
      ".llat {display:inline-block;width:80px;text-align:center;}" 
      ".llon {display:inline-block;width:60px;text-align:center;}" 
      ".geo {width:60px;margin-top:8px;text-align:center;}" 
      ".p1 {max-width:550px;}" 
      ".p2 {max-width:550px;}" 
      ".rst {float:right;}" 
      "@media only screen and (max-width: 550px) {.p1 {max-width:320px;}"
      ".llat {display:inline-block;width:83px;margin-left:11px;text-align:center;}}" 
      ".llon {display:inline-block;width:83px;text-align:center;}}" 
      "</style>"
      "<title>"+W_PARMM+"</title>"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
      "</head><body>"
      "<h1>"+W_PARMM+"</h1>"
      "<form action=\"/weather_get\" method=\"get\">"
        "<div class=\"p1\">"
        "<input type=\"radio\" id=\"def1\" name=\"DefLoc\" value=\"0\" " + Lchecked[0] + ">"
        "<label for=\"loc1\" class=\"lloc\">Loc 1</label>"
        "<input type=\"text\" class=\"loc\" id=\"loc1\" name=\"" + NLoc[0] + "\" value=\"" + VLoc[0] + "\" maxlength=30>"
        "<label for=\"lat1\" class=\"llat\">Lat 1</label>"
        "<input type=\"text\" class=\"geo\" id=\"lat1\" name=\"" + NLat[0] + "\" value=\"" + VLat[0] + "\" " + ">"
        "<label for=\"long1\" class=\"llon\">Lon 1</label>"
        "<input type=\"text\" class=\"geo\" id=\"long\" name=\"" + NLon[0] + "\" value=\"" + VLon[0] + "\" " + "><br>"
        "<input type=\"radio\" id=\"def2\" name=\"DefLoc\" value=\"1\" " + Lchecked[1] + ">"
        "<label for=\"loc2\" class=\"lloc\">Loc 2</label>"
        "<input type=\"text\" class=\"loc\" id=\"loc2\" name=\"" + NLoc[1] + "\" value=\"" + VLoc[1] + "\" maxlength=30>"
        "<label for=\"lat2\" class=\"llat\">Lat 2</label>"
        "<input type=\"text\" class=\"geo\" id=\"lat2\" name=\"" + NLat[1] + "\" value=\"" + VLat[1] + "\" " + ">"
        "<label for=\"long2\" class=\"llon\">Lon 2</label>"
        "<input type=\"text\" class=\"geo\" id=\"long2\" name=\"" + NLon[1] + "\" value=\"" + VLon[1] + "\" " + "><br>"
        "<input type=\"radio\" id=\"def3\" name=\"DefLoc\" value=\"2\" " + Lchecked[2] + ">"
        "<label for=\"loc3\" class=\"lloc\">Loc 3</label>"
        "<input type=\"text\" class=\"loc\" id=\"loc3\" name=\"" + NLoc[2] + "\" value=\"" + VLoc[2] + "\" maxlength=30>"
        "<label for=\"lat3\" class=\"llat\">Lat 3</label>"
        "<input type=\"text\" class=\"geo\" id=\"lat3\" name=\"" + NLat[2] + "\" value=\"" + VLat[2] + "\" " + ">"
        "<label for=\"long3\" class=\"llon\">Lon 3</label>"
        "<input type=\"text\" class=\"geo\" id=\"long3\" name=\"" + NLon[2] + "\" value=\"" + VLon[2] + "\" " + "><br>"
        "<input type=\"radio\" id=\"def4\" name=\"DefLoc\" value=\"3\" " + Lchecked[3] + ">"
        "<label for=\"loc4\" class=\"lloc\">Loc 4</label>"
        "<input type=\"text\" class=\"loc\" id=\"loc4\" name=\"" + NLoc[3] + "\" value=\"" + VLoc[3] + "\" maxlength=30>"
        "<label for=\"lat4\" class=\"llat\">Lat 4</label>"
        "<input type=\"text\" class=\"geo\" id=\"lat4\" name=\"" + NLat[3] + "\" value=\"" + VLat[3] + "\" " + ">"
        "<label for=\"long4\" class=\"llon\">Lon 4</label>"
        "<input type=\"text\" class=\"geo\" id=\"long4\" name=\"" + NLon[3] + "\" value=\"" + VLon[3] + "\" " + "><br>"
        "<input type=\"radio\" id=\"def5\" name=\"DefLoc\" value=\"4\" " + Lchecked[4] + ">"
        "<label for=\"loc5\" class=\"lloc\">Loc 5</label>"
        "<input type=\"text\" class=\"loc\" id=\"loc5\" name=\"" + NLoc[4] + "\" value=\"" + VLoc[4] + "\" maxlength=30>"
        "<label for=\"lat5\" class=\"llat\">Lat 5</label>"
        "<input type=\"text\" class=\"geo\" id=\"lat5\" name=\"" + NLat[4] + "\" value=\"" + VLat[4] + "\" " + ">"
        "<label for=\"long5\" class=\"llon\">Lon 5</label>"
        "<input type=\"text\" class=\"geo\" id=\"long5\" name=\"" + NLon[4] + "\" value=\"" + VLon[4] + "\" " + "><br>"
        "<input type=\"radio\" id=\"def6\" name=\"DefLoc\" value=\"5\" " + Lchecked[5] + ">"
        "<label for=\"loc6\" class=\"lloc\">Loc 6</label>"
        "<input type=\"text\" class=\"loc\" id=\"loc6\" name=\"" + NLoc[5] + "\" value=\"" + VLoc[5] + "\" maxlength=30>"
        "<label for=\"lat6\" class=\"llat\">Lat 6</label>"
        "<input type=\"text\" class=\"geo\" id=\"lat1\" name=\"" + NLat[5] + "\" value=\"" + VLat[5] + "\" " + ">"
        "<label for=\"long6\" class=\"llon\">Lon 6</label>"
        "<input type=\"text\" class=\"geo\" id=\"long\" name=\"" + NLon[5] + "\" value=\"" + VLon[5] + "\" " + "><br>"
        "<input type=\"radio\" id=\"def7\" name=\"DefLoc\" value=\"6\" " + Lchecked[6] + ">"
        "<label for=\"loc7\" class=\"lloc\">Loc 7</label>"
        "<input type=\"text\" class=\"loc\" id=\"loc7\" name=\"" + NLoc[6] + "\" value=\"" + VLoc[6] + "\" maxlength=30>"
        "<label for=\"lat7\" class=\"llat\">Lat 7</label>"
        "<input type=\"text\" class=\"geo\" id=\"lat7\" name=\"" + NLat[6] + "\" value=\"" + VLat[6] + "\" " + ">"
        "<label for=\"long7\" class=\"llon\">Lon 7</label>"
        "<input type=\"text\" class=\"geo\" id=\"long7\" name=\"" + NLon[6] + "\" value=\"" + VLon[6] + "\" " + "><br>"
        "<input type=\"radio\" id=\"def8\" name=\"DefLoc\" value=\"7\" " + Lchecked[7] + ">"
        "<label for=\"loc8\" class=\"lloc\">Loc 8</label>"
        "<input type=\"text\" class=\"loc\" id=\"loc8\" name=\"" + NLoc[7] + "\" value=\"" + VLoc[7] + "\" maxlength=30>"
        "<label for=\"lat8\" class=\"llat\">Lat 8</label>"
        "<input type=\"text\" class=\"geo\" id=\"lat8\" name=\"" + NLat[7] + "\" value=\"" + VLat[7] + "\" " + ">"
        "<label for=\"long8\" class=\"llon\">Lon 8</label>"
        "<input type=\"text\" class=\"geo\" id=\"long8\" name=\"" + NLon[7] + "\" value=\"" + VLon[7] + "\" " + "><br>"
        "<input type=\"radio\" id=\"def9\" name=\"DefLoc\" value=\"8\" " + Lchecked[8] + ">"
        "<label for=\"loc9\" class=\"lloc\">Loc 9</label>"
        "<input type=\"text\" class=\"loc\" id=\"loc9\" name=\"" + NLoc[8] + "\" value=\"" + VLoc[8] + "\" maxlength=30>"
        "<label for=\"lat9\" class=\"llat\">Lat 9</label>"
        "<input type=\"text\" class=\"geo\" id=\"lat9\" name=\"" + NLat[8] + "\" value=\"" + VLat[8] + "\" " + ">"
        "<label for=\"long9\" class=\"llon\">Lon 9</label>"
        "<input type=\"text\" class=\"geo\" id=\"long9\" name=\"" + NLon[8] + "\" value=\"" + VLon[8] + "\" " + "><br><br>"
	"</div>"
        "<div class=\"p2\">"
        "<a href=\"/wifi\">"+W_PARMW+"</a>"
        "<a href=\"/weather_reset\" class=\"rst\">"+W_REINITM+"</a><br><br>"
        "<a href=\"/parm\">"+W_PARMV+"</a><br><br>"
        "<input type=\"submit\" value=\""+W_SUBMIT+"\">"
	"</div>"
      "</form>"
      "</body></html>" );
    }
    else
      page_lost(request);
  });

  server.on("/weather_get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      int i;
      for (i=0 ; i<MX_SSI ; i++)
      {
        if (request->hasParam(NLoc[i])) {
          VLoc[i] = request->getParam(NLoc[i])->value();
          preferences.putString(NLoc[i].c_str(), VLoc[i]);
          Serial.printf("Loc[%d]: %s\n", i, VLoc[i].c_str());
        }
        if (request->hasParam(NLat[i])) {
          VLat[i] = request->getParam(NLat[i])->value();
          preferences.putString(NLat[i].c_str(), VLat[i]);
          Serial.printf("Lat[%d]: %s\n", i, VLat[i].c_str());
        }
        if (request->hasParam(NLon[i])) {
         VLon[i] = request->getParam(NLon[i])->value();
          preferences.putString(NLon[i].c_str(), VLon[i]);
          Serial.printf("Lon[%d]: %s\n", i, VLon[i].c_str());
        }
      }
      if (request->hasParam("DefLoc")) {
        DefLoc = request->getParam(NDloc)->value();
        Lchecked[defloc] = "";
        defloc = DefLoc.toInt();
        Lchecked[defloc] = "checked";
        preferences.putString(NDloc.c_str(), DefLoc);
        Serial.printf("Def Location: %d\n", defloc);
      }
      
      if ( check_config() )
        request->send(200, "text/html", RSP_INVAL_PARM);
      else
        request->send(200, "text/html", RSP_ACT_DONE);

      restart_wdg();
    }
    else
      page_lost(request);
  });

  server.on("/weather_reset", HTTP_GET, [] (AsyncWebServerRequest *request) {
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      reset_weather_config();
      request->send(200, "text/html", RSP_ACT_DONE);
    }
    else
      page_lost(request);

    restart_wdg();
  });

  // Send web page with input fields to client
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      request->send(200, "text/html",
      "<!DOCTYPE HTML><html><head>"
      "<style>"
      "label {display:inline-block;width:80px;text-align:center;}" 
      "input[type=text] {width:150px;;margin-top:10px;}"
      ".p1 {max-width:520px;}" 
      ".p2 {max-width:520px;}" 
      ".rst {float:right;}" 
      "@media only screen and (max-width: 460px) {.p1 {max-width:300px;}}"
      "</style>"
      "<title>"+W_PARMW+"</title>"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
      "</head><body>"
      "<h1>"+W_PARMW+"</h1>"
      "<form action=\"/wifi_get\" method=\"get\">"
        "<div class=\"p1\">"
	"<label for=\"ssi1\">SSID 1</label>"
        "<input type=\"text\" class=\"ssi\" id=\"ssi1\" name=\"" + NSsi[0] + "\" value=\"" + VSsi[0] + "\">"
        "<label for=\"pwd1\">PWD 1</label>"
        "<input type=\""+TAGPWD+"\" class=\"pwd\" id=\"pwd1\" name=\"" + NPwd[0] + "\" value=\"" + VPwd[0] + "\"><br>"
        "<label for=\"ssi2\">SSID 2</label>"
        "<input type=\"text\" class=\"ssi\" id=\"ssi2\" name=\"" + NSsi[1] + "\" value=\"" + VSsi[1] + "\">"
        "<label for=\"pwd2\">PWD 2</label>"
        "<input type=\""+TAGPWD+"\" class=\"pwd\" id=\"pwd2\" name=\"" + NPwd[1] + "\" value=\"" + VPwd[1] + "\"><br>"
        "<label for=\"ssi3\">SSID 3</label>"
        "<input type=\"text\" class=\"ssi\" id=\"ssi3\" name=\"" + NSsi[2] + "\" value=\"" + VSsi[2] + "\">"
        "<label for=\"pwd3\">PWD 3</label>"
        "<input type=\""+TAGPWD+"\" class=\"pwd\" id=\"pwd3\" name=\"" + NPwd[2] + "\" value=\"" + VPwd[2] + "\"><br>"
        "<label for=\"ssi4\">SSID 4</label>"
        "<input type=\"text\" class=\"ssi\" id=\"ssi4\" name=\"" + NSsi[3] + "\" value=\"" + VSsi[3] + "\">"
        "<label for=\"pwd4\">PWD 4</label>"
        "<input type=\""+TAGPWD+"\" class=\"pwd\" id=\"pwd4\" name=\"" + NPwd[3] + "\" value=\"" + VPwd[3] + "\"><br>"
        "<label for=\"ssi5\">SSID 5</label>"
        "<input type=\"text\" class=\"ssi\" id=\"ssi5\" name=\"" + NSsi[4] + "\" value=\"" + VSsi[4] + "\">"
        "<label for=\"pwd5\">PWD 5</label>"
        "<input type=\""+TAGPWD+"\" class=\"pwd\" id=\"pwd5\" name=\"" + NPwd[4] + "\" value=\"" + VPwd[4] + "\"><br>"
        "<label for=\"ssi6\">SSID 6</label>"
        "<input type=\"text\" class=\"ssi\" id=\"ssi6\" name=\"" + NSsi[5] + "\" value=\"" + VSsi[5] + "\">"
        "<label for=\"pwd6\">PWD 6</label>"
        "<input type=\""+TAGPWD+"\" class=\"pwd\" id=\"pwd6\" name=\"" + NPwd[5] + "\" value=\"" + VPwd[5] + "\"><br><br>"
	"</div>"
        "<div class=\"p2\">"
        "<a href=\"/wifi_reset\" class=\"rst\">"+W_REINITW+"</a>"
        "<a href=\"/weather\">"+W_PARMM+"</a><br><br>"
        "<a href=\"/parm\">"+W_PARMV+"</a><br><br>"
        "<input type=\"submit\" value=\""+W_SUBMIT+"\">"
	"</div>"
      "</form>"
      "</body></html>" );
    }
    else
      page_lost(request);
  });

  server.on("/wifi_get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      int i;
      for (i=0 ; i<MX_SSI ; i++)
      {
        if (request->hasParam(NSsi[i])) {
          VSsi[i] = request->getParam(NSsi[i])->value();
          preferences.putString(NSsi[i].c_str(), VSsi[i]);
          Serial.printf("Ssi[%d]: '%s'\n", i, VSsi[i].c_str());
        }
        if (request->hasParam(NPwd[i])) {
          VPwd[i] = request->getParam(NPwd[i])->value();
          preferences.putString(NPwd[i].c_str(), VPwd[i]);
          Serial.printf("Pwd[%d]: '%s'\n", i, VPwd[i].c_str());
        }
      }

      if ( check_config() )
        request->send(200, "text/html", RSP_INVAL_PARM);
      else
        request->send(200, "text/html", RSP_ACT_DONE);

      restart_wdg();
    }
    else
      page_lost(request);
  });

  server.on("/wifi_reset", HTTP_GET, [] (AsyncWebServerRequest *request) {
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      reset_wifi_config();
      request->send(200, "text/html", RSP_ACT_DONE);
    }
    else
      page_lost(request);

    restart_wdg();
  });

  // Send web page with input fields to client
  server.on("/parm", HTTP_GET, [](AsyncWebServerRequest *request){
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      request->send(200, "text/html",
      "<!DOCTYPE HTML><html><head>"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
      "<style>"
      ".p1 {max-width:520px;}" 
      ".l1 {display:inline-block;width:180px;margin-left:8px}" 
      ".l2 {display:inline-block;margin-left:8px}" 
      "input[type=number] {width:60px;margin-top:10px;text-align:center;}" 
      ".rst {float:right;}" 
      "</style>"
      "<title>"+W_PARMV+"</title>"
      "</head><body>"
      "<h1>"+W_PARMV+"</h1>"
      "<form action=\"/parm_get\" method=\"get\">"
        "<div class=\"p1\">"
        "<label for=\"popth\" class=\"l1\">"+W_POPTHR+"</label>"
        "<input type=\"number\" class=\"num\" id=\"popth\" name=\"" + NM_THR + "\" value=\"" + String(PopTh) + "\" "
          "min=0 max=1 step=0.01>"
        "<label for=\"popth\" class=\"l2\">%</label><br>"
        "<label for=\"bed\" class=\"l1\">"+W_BEDTIM+"</label>"
        "<input type=\"number\" class=\"num\" id=\"bed\" name=\"" + NM_BED + "\" value=\"" + String(BedTime) + "\" "
          "min=0 max=23 step=1>"
        "<label for=\"bed\" class=\"l2\">H</label><br>"
        "<label for=\"wake\" class=\"l1\">"+W_WAKTIM+"</label>"
        "<input type=\"number\" class=\"num\" id=\"wake\" name=\"" + NM_WAK + "\" value=\"" + String(WakeTime) + "\" "
          "min=0 max=23 step=1>"
        "<label for=\"wake\" class=\"l2\">H</label><br>"
        "<label for=\"sleep\" class=\"l1\">"+W_SLPDLY+"</label>"
        "<input type=\"number\" class=\"num\" id=\"sleep\" name=\"" + NM_SLP + "\" value=\"" + String(SleepDly) + "\" "
          "min=10 max=60 step=10>"
        "<label for=\"sleep\" class=\"l2\">min</label><br>"
        "<label for=\"hrnb\" class=\"l1\">"+W_HOURNB+"</label>"
        "<input type=\"number\" class=\"num\" id=\"hrnb\" name=\"" + NM_HNB + "\" value=\"" + String(HourlyNb) + "\" "
          "min=8 max=48 step=1>"
        "<label for=\"hrnb\" class=\"l2\">H</label><br>"
        "<label for=\"minr\"class=\"l1\">"+W_MAJMIN+"</label>"
        "<input type=\"number\" class=\"num\" id=\"minr\" name=\"" + NM_MRT + "\" value=\"" + String(MinRefTim) + "\" "
          "min=60 max=300 step=1>"
        "<label for=\"minr\" class=\"l2\">sec</label><br>"
        "<label for=\"maxa\" class=\"l1\">"+W_WEBDLY+"</label>"
        "<input type=\"number\" class=\"num\" id=\"maxa\" name=\"" + NM_MAT + "\" value=\"" + String(MaxActTim) + "\" "
          "min=60 max=600 step=1>"
        "<label for=\"maxa\" class=\"l2\">sec</label><br>"
        "<label for=\"hto\" class=\"l1\">"+W_HTO+"</label>"
        "<input type=\"number\" class=\"num\" id=\"hto\" name=\"" + NM_HTO + "\" value=\"" + String(HttpTimeout) + "\" >"
        "<label for=\"hto\" class=\"l2\">msec</label><br>"
        "<label for=\"wato\" class=\"l1\">"+W_WATO+"</label>"
        "<input type=\"number\" class=\"num\" id=\"wato\" name=\"" + NM_WAT + "\" value=\"" + String(WifiAPto) + "\" >"
        "<label for=\"wato\" class=\"l2\">msec</label><br>"
        "<label for=\"wgto\" class=\"l1\">"+W_WGTO+"</label>"
        "<input type=\"number\" class=\"num\" id=\"wgto\" name=\"" + NM_WGT + "\" value=\"" + String(WifiTimeout) + "\" >"
        "<label for=\"wgto\" class=\"l2\">msec</label><br><br>"
        "<label for=\"wgto\" class=\"l2\">" + W_WTOREC + "</label><br><br>"
        "<a href=\"/parm_reset\" class=\"rst\">"+W_REINITV+"</a>"
        "<a href=\"/weather\">"+W_PARMM+"</a><br><br>"
        "<a href=\"/wifi\">"+W_PARMW+"</a><br><br>"
        "<input type=\"submit\" value=\""+W_SUBMIT+"\">"
	"</div>"
      "</form>"
      "</body></html>" );
    }
    else
      page_lost(request);
  });

  server.on("/parm_get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      if (request->hasParam(NM_THR)) {
        String s = request->getParam(NM_THR)->value();
        PopTh = s.toFloat();
        preferences.putString(NM_THR, String(PopTh));
        Serial.printf("POP Threshold: %f\n", PopTh);
      }
      if (request->hasParam(NM_BED)) {
        String s = request->getParam(NM_BED)->value();
        BedTime = s.toInt();
        preferences.putString(NM_BED, s);
        Serial.printf("BED Time: %d\n", BedTime);
      }
      if (request->hasParam(NM_WAK)) {
        String s = request->getParam(NM_WAK)->value();
        WakeTime = s.toInt();
        preferences.putString(NM_WAK, s);
        Serial.printf("WAKE Time: %d\n", WakeTime);
      }
      if (request->hasParam(NM_SLP)) {
        String s = request->getParam(NM_SLP)->value();
        SleepDly = (long)s.toInt();
        preferences.putString(NM_SLP, s);
        Serial.printf("Sleep-dly: %ld\n", SleepDly);
      }
      if (request->hasParam(NM_HNB)) {
        String s = request->getParam(NM_HNB)->value();
        HourlyNb = s.toInt();
        preferences.putString(NM_HNB, s);
        Serial.printf("Hourly-Nb: %d\n", HourlyNb);
      }
      if (request->hasParam(NM_MRT)) {
        String s = request->getParam(NM_MRT)->value();
        MinRefTim = s.toInt();
        preferences.putString(NM_MRT, s);
        Serial.printf("Min-Refresh: %d\n", MinRefTim);
      }
      if (request->hasParam(NM_MAT)) {
        String s = request->getParam(NM_MAT)->value();
        MaxActTim = s.toInt();
        preferences.putString(NM_MAT, s);
        Serial.printf("Min-Refresh: %d\n", MaxActTim);
      }
      
      if ( check_config() )
        request->send(200, "text/html", RSP_INVAL_PARM);
      else
        request->send(200, "text/html", RSP_ACT_DONE);

      restart_wdg();
    }
    else
      page_lost(request);
  });

  server.on("/parm_reset", HTTP_GET, [] (AsyncWebServerRequest *request) {
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      reset_parm_config();
      request->send(200, "text/html", RSP_ACT_DONE);
    }
    else
      page_lost(request);

    restart_wdg();
  });

  // Initialize NVS in the actual state
  server.on("/clean", HTTP_GET, [] (AsyncWebServerRequest *request) {
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      Serial.println("Clearing the NVS");

      clean_nvs();
      request->send(200, "text/html", RSP_ACT_DONE);
    }
    else
      page_lost(request);

    restart_wdg();
  });

  // Initialize NVS in the initial state
  server.on("/clear", HTTP_GET, [] (AsyncWebServerRequest *request) {
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      Serial.println("Clearing the NVS");

      clear_nvs();
      request->send(200, "text/html", RSP_ACT_DONE);
    }
    else
      page_lost(request);

    restart_wdg();
  });

  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      // The objective is to wake up and refresh as soon as possible
      // but with a minimum interval of 'MinRefTim' since the last start
      unsigned long mintm = millis() - startTime;   // rollover handled by C substract
      mintm = (mintm > (MinRefTim*1000)) ? 500 : ((MinRefTim*1000) - mintm);

      request->send(200, "text/html",
                    RSP_TERMACT1a_DONE+String((mintm+999)/1000)+RSP_TERMACT1b_DONE);

      drawWebIcon(0);

      do_deep_sleep(mintm);
    }
    else
      page_lost(request);
  });

  server.on("/exit", HTTP_GET, [] (AsyncWebServerRequest *request) {
    IPAddress ip = request->client()->remoteIP();
    if ( check_remoteLogged(&ip) )
    {
      tm timeInfo = {};

      request->send(200, "text/html", RSP_TERMACT2_DONE);

      drawWebIcon(0);

      beginDeepSleep(startTime, &timeInfo);
    }
    else
      page_lost(request);
  });

  server.onNotFound(notFound);

  Serial.println("Starting WEB server");

  server.begin();
}
#endif // WEB_SVR

/*****************************************************/
/* WIFI management                                   */
/*****************************************************/

#include <WiFiMulti.h>

// Wifi instance
WiFiMulti wifiMulti;

/*
 * Current Wifi state
 */
static int cur_wst = -1;

#ifndef WEB_SVR
// WEB server not used, so need to define the credential locally
#define MX_SSI 6

String VSsi[MX_SSI] = { WIFI_SSI1, WIFI_SSI2, WIFI_SSI3, WIFI_SSI4, WIFI_SSI5, WIFI_SSI6 }; 
String VPwd[MX_SSI] = { WIFI_PWD1, WIFI_PWD2, WIFI_PWD3, WIFI_PWD4, WIFI_PWD5, WIFI_PWD6 }; 
#endif

/*
 * WIFI_CHECK
 *
 * Check and reconnect Wifi while serveur is active
 */
int wifi_check ( void )
{
  int wst = wifiMulti.run(WIFI_AP_TO);

  if ( wst != cur_wst )
  {
    cur_wst = wst;
    Serial.printf("WIFI state : %s (%d)\n",
                   (wst == WL_CONNECTED) ? "connected" : "off", wst);

    if ( wst == WL_CONNECTED )
    {
      Serial.print("SSID: ");
      Serial.println(WiFi.SSID());
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    }
  }

  return wst;
}

/*
 * WIFI_SCANSSID
 *
 * Print discovered Wifi
 */
void wifi_scanSsid ( void )
{
  // WiFi.scanNetworks will return the number of networks found
  int n;

  n = WiFi.scanNetworks();
  Serial.println("scan done");

  if (n == 0) {
    Serial.println("no networks found");
  } 
  else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      delay(10);
    }
  }
}

/*
 * WIFI_ADDAP
 *
 * Empty and set list of Wifi Access Points
 */
int wifi_addAP ( void )
{
  int i, n;
  
  //wifiMulti.APlistClean();

  for (i=n=0; i<MX_SSI ; i++)
  {
    if ( VSsi[i] != "" )
    {
      Serial.printf("Adding AP '%s'\n", VSsi[i].c_str());
      wifiMulti.addAP(VSsi[i].c_str(), VPwd[i].c_str());
      n++;
    }
  }
  
  return n;
}

/*
 * STARTWIFI
 *
 * Power-on and connect WiFi.
 * Takes int parameter to store WiFi RSSI (Received Signal Strength Indicator)
 *
 * Returns WiFi status.
 */
wl_status_t startWiFi(int &wifiRSSI, int web_mode)
{
  // timeout if WiFi does not connect in WIFI_TIMEOUT ms from now
  unsigned long timeout = millis() + WIFI_TIMEOUT;
  wl_status_t   connection_status = WL_CONNECTED;
  int           n;
  
  //define hostname
  WiFi.setHostname(HNAME);

#ifdef WEB_SVR
  if (web_mode)
  {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(SOFTAP_SSID, SOFTAP_PWD);

    Serial.print("WIFI AP ");
    Serial.print(SOFTAP_SSID);
    Serial.print(" created with IP gateway ");
    Serial.println(WiFi.softAPIP());
  }
  else
#endif
  {
    WiFi.mode(WIFI_STA);
    Serial.println("WIFI Station");
  }
  
  Serial.printf("%s '%s'\n", TXT_CONNECTING_TO, "WiFi");

  n = wifi_addAP();

  wifi_scanSsid();

#ifdef OLD
  do
  {
static int prv = -1;

    delay(50);
    connection_status = (wl_status_t) wifiMulti.run();
    if ( connection_status != prv )
    {
      Serial.printf("wifi state = %d\n", connection_status); 
      prv = connection_status;
    }
  }
  while ( (connection_status != WL_CONNECTED)  &&
          ((millis() - timeout) < WIFI_TIMEOUT) );
#else
  if ( n )
  {
    // At least one AP defined
    do
    {
      connection_status = (wl_status_t) wifi_check();
      //Serial.printf("ST=%d, (%d-%d-%d)\n", connection_status,
      //       WL_CONNECTED,
      //       WL_NO_SSID_AVAIL,
      //       WL_DISCONNECTED);
    }
    while ( (connection_status != WL_CONNECTED)     &&
            (connection_status != WL_NO_SSID_AVAIL) &&
            (connection_status != WL_DISCONNECTED)  &&
            ((millis() - timeout) < WIFI_TIMEOUT)    );
  }
  else
    connection_status = WL_NO_SSID_AVAIL;
#endif

  if (connection_status == WL_CONNECTED)
  {
    // Get WiFi signal strength now, because the WiFi
    // will be turned off to save power!
    wifiRSSI = WiFi.RSSI();

#ifdef OLD
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.println("IP: " + WiFi.localIP().toString());
    //wsta = 1;
#endif
  }
  else
    Serial.printf("%s '%s'\n", TXT_COULD_NOT_CONNECT_TO, "WiFi");

  return connection_status;
} // startWiFi

/*
 * KILLWIFI
 *
 * Disconnect and power-off WiFi.
 */
void killWiFi()
{
  Serial.println("Killing WIFI");
  
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
} // killWiFi

/*****************************************************/
/* OWM API ACCESS                                    */
/*****************************************************/

/*
 * getOWMonecall
 *
 * Perform an HTTP GET request to OpenWeatherMap's "One Call" API
 * If data is received, it will be parsed and stored in the global variable
 * owm_onecall.
 *
 * Returns the HTTP Status Code.
 */
#ifdef USE_HTTP
  int getOWMonecall(WiFiClient &client, owm_resp_onecall_t &r)
#else
  int getOWMonecall(WiFiClientSecure &client, owm_resp_onecall_t &r)
#endif
{
  int attempts = 0;
  bool rxSuccess = false;
  DeserializationError jsonErr = {};
  String uri = "/data/" + OWM_ONECALL_VERSION
               + "/onecall?lat=" + LAT + "&lon=" + LON + "&lang=" + OWM_LANG
               + "&units=standard&exclude=minutely";
#if !DISPLAY_ALERTS
  // exclude alerts
  uri += ",alerts";
#endif

  // This string is printed to terminal to help with debugging. The API key is
  // censored to reduce the risk of users exposing their key.
  String sanitizedUri = OWM_ENDPOINT + uri + "&appid={API key}";

  uri += "&appid=" + OWM_APIKEY;

  Serial.print(TXT_ATTEMPTING_HTTP_REQ);
  Serial.println(": " + sanitizedUri);
  int httpResponse = 0;
  while (!rxSuccess && attempts < 3)
  {
    wl_status_t connection_status = WiFi.status();
    if (connection_status != WL_CONNECTED)
    {
      // -512 offset distinguishes these errors from httpClient errors
      return -512 - static_cast<int>(connection_status);
    }

    HTTPClient http;
    http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT);
    http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);
    http.begin(client, OWM_ENDPOINT, OWM_PORT, uri);
    httpResponse = http.GET();
    if (httpResponse == HTTP_CODE_OK)
    {
      jsonErr = deserializeOneCall(http.getStream(), r);
      if (jsonErr)
      {
        // -256 offset distinguishes these errors from httpClient errors
        httpResponse = -256 - static_cast<int>(jsonErr.code());
      }
      rxSuccess = !jsonErr;
    }
    client.stop();
    http.end();
    Serial.println("  " + String(httpResponse, DEC) + " "
                   + getHttpResponsePhrase(httpResponse));
    ++attempts;
  }

  return httpResponse;
} // getOWMonecall

/*
 * getOWMairpollution
 *
 * Perform an HTTP GET request to OpenWeatherMap's "Air Pollution" API
 * If data is received, it will be parsed and stored in the global variable
 * owm_air_pollution.
 *
 * Returns the HTTP Status Code.
 */
#ifdef USE_HTTP
  int getOWMairpollution(WiFiClient &client, owm_resp_air_pollution_t &r)
#else
  int getOWMairpollution(WiFiClientSecure &client, owm_resp_air_pollution_t &r)
#endif
{
  int attempts = 0;
  bool rxSuccess = false;
  DeserializationError jsonErr = {};

  // set start and end to appropriate values so that the last 24 hours of air
  // pollution history is returned. Unix, UTC.
  time_t now;
  int64_t end = time(&now);
  // minus 1 is important here, otherwise we could get an extra hour of history
  int64_t start = end - ((3600 * OWM_NUM_AIR_POLLUTION) - 1);
  char endStr[22];
  char startStr[22];
  sprintf(endStr, "%lld", end);
  sprintf(startStr, "%lld", start);
  String uri = "/data/2.5/air_pollution/history?lat=" + LAT + "&lon=" + LON
               + "&start=" + startStr + "&end=" + endStr
               + "&appid=" + OWM_APIKEY;
  // This string is printed to terminal to help with debugging. The API key is
  // censored to reduce the risk of users exposing their key.
  String sanitizedUri = OWM_ENDPOINT +
               "/data/2.5/air_pollution/history?lat=" + LAT + "&lon=" + LON
               + "&start=" + startStr + "&end=" + endStr
               + "&appid={API key}";

  Serial.print(TXT_ATTEMPTING_HTTP_REQ);
  Serial.println(": " + sanitizedUri);
  int httpResponse = 0;
  while (!rxSuccess && attempts < 3)
  {
    wl_status_t connection_status = WiFi.status();
    if (connection_status != WL_CONNECTED)
    {
      // -512 offset distinguishes these errors from httpClient errors
      return -512 - static_cast<int>(connection_status);
    }

    HTTPClient http;
    http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT);
    http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);
    http.begin(client, OWM_ENDPOINT, OWM_PORT, uri);
    httpResponse = http.GET();
    if (httpResponse == HTTP_CODE_OK)
    {
      jsonErr = deserializeAirQuality(http.getStream(), r);
      if (jsonErr)
      {
        // -256 offset to distinguishes these errors from httpClient errors
        httpResponse = -256 - static_cast<int>(jsonErr.code());
      }
      rxSuccess = !jsonErr;
    }
    client.stop();
    http.end();
    Serial.println("  " + String(httpResponse, DEC) + " "
                   + getHttpResponsePhrase(httpResponse));
    ++attempts;
  }

  return httpResponse;
} // getOWMairpollution

/*****************************************************/
/* Other utilities                                   */
/*****************************************************/

/*
 * printHeapUsage
 *
 * Prints debug information about heap usage.
 */
void printHeapUsage() {
  Serial.println("[debug] Heap Size       : "
                 + String(ESP.getHeapSize()) + " B");
  Serial.println("[debug] Available Heap  : "
                 + String(ESP.getFreeHeap()) + " B");
  Serial.println("[debug] Min Free Heap   : "
                 + String(ESP.getMinFreeHeap()) + " B");
  Serial.println("[debug] Max Allocatable : "
                 + String(ESP.getMaxAllocHeap()) + " B");
  return;
}
