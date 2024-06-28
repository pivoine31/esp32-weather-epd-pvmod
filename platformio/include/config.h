/* Configuration option declarations for esp32-weather-epd.
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <cstdint>
#include <Arduino.h>

// AUTO_TZ feature
// Fetch current time (at location) and timezone in OWM response
// (avoid SNTP setup delay, and automate time management for various locations)

// AUTO_POP_CONTRAST feature
// Permit automatic modification of POP graph density
// depending on the amount of surface grayed

// WIFI_MULTI feature
// Use Multi credential WIFI
// Anyway the default wifi defined in Config.cpp is always available

// Use WEB serveur
#define WEB_SVR

#ifdef WEB_SVR

// Strategy/pin for entering Web server from deep sleep
// BUTTON and TOUCH are mutually exclusive
// Either define BUTTON_PIN to the adhoc pin for using a button pin
// Or Undefine (comment) BUTTON_PIN for using a touch pin
// Default is to use the internal button (GPIO 27)

// This is the default:
// - You may use an other button (pulled up internally or externally)
// - or comment (//) or remove or #undef this line for using a touch pin 
#define BUTTON_PIN GPIO_NUM_27

// When BUTTON_PIN is not defined, you may customize TOUCH_PIN
// The possible touch pins are : T1, T2, T3 and T5
// (T0, T4 and T6 already used for other purpose)
#ifndef BUTTON_PIN
#define TOUCH_PIN T1 // GPIO_NUM_11
#define TOUCH_THR 40
#endif

// Key for acceeding the Web Server
// comment out or undef to avoid using it
// otherwise cusyomize to more specific value
#define WEBKEY         "0000"

// Hide password in Wifi Web page when defined
//#define WEB_HIDE_PWD

// Max Active time (sec) allowed after Web server started (msec)
// Reset every time Wifi or Weather Web pages are submitted
#define DEF_MAXACT_TIM 180

// Min interval (sec) before refreshing the display, on Web server
// relative to the time The Web server was started
// (3 min seems to be the manufacturer requirement)
#define DEF_MINREF_TIM 180

// Min interval (ms) for checking button/touchpin after manual wakeup
// (in order to avoid bouncing)
#define MIN_BUT_CHK    5000

/*
 * WEB server fct and vars called from outside
 */
void retrieve_config ( void );
void web_svr_setup   ( void );

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
 */
void restart_wdg     ( void );
void do_deep_sleep   ( uint64_t );
void beginDeepSleep  ( unsigned long &startTime, tm *timeInfo );
void drawWebIcon     ( int active );

extern unsigned long startTime;

#endif // WEB_SVR

// Default location and coordinates
#define DEFLAT        "40.7128" 
#define DEFLON        "-74.0060" 
#define DEFCITY       "New York" 

// Define hours of last update (DEFBEDà until wake time (DEFWAKE)
// If bed time == wake time, then this battery saving feature is disabled
// (range: [0-23])
#define DEFBED         0
#define DEFWAKE        7

// Sleep duration in minutes. (aka how often esp32 will wake for an update)
// Aligned to the nearest minute boundary and must evenly divide 60.
// For example, if set to 30 (minutes) the display will update at 00 or 30
// minutes past the hour. (range: [2-60])
#define DEFSLEEP       10

// Number of hours to display on the outlook graph. (range: [8-48])
#define DEFHOURNB      24

// OpenWeatherMap API key, https://openweathermap.org/
#define DEFAPIKEY      "11223344556677889900112233445566"

// AUTO_POP_CONTRAST feature
// Threshold for generating precipitation graph
// Indicates the percentage of graph filling from which the 
// draw pattern density is decreased (default to 65 %)
// Value 1.0 disable the feature
#define PRECIP_THRESHOLD 0.65f

/* WIFI_MULTI */
// Nom d'hote (used also for NVS storage)
#define HNAME "ESP32-Meteo"

// Define Wifi credential here
// When Web server in use, only one is defined
// It is made read-only and used as default (emergency wifi credential)
// Other credential are entered through the Web interface
#define WIFI_SSI1 ""
#define WIFI_PWD1 ""

#ifndef WEB_SVR
// When Web server not used, up to 5 five additional credential
// may be defined here
// If empty, credential entry not used
#define WIFI_SSI2 ""
#define WIFI_PWD2 ""
#define WIFI_SSI3 ""
#define WIFI_PWD3 ""
#define WIFI_SSI4 ""
#define WIFI_PWD4 ""
#define WIFI_SSI5 ""
#define WIFI_PWD5 ""
#define WIFI_SSI6 ""
#define WIFI_PWD6 ""
#endif // WEB_SVR

// Check and re-establish Wifi (web server mode)
int wifi_check ( void );
/* WIFI_MULTI */

// WIFI
// Default SSID / Password

// WiFi connection timeout (msec)
// WIFI_MULTI Increase value when multiples AP in use
#define DEF_WIFI_TIMEOUT 30000

// HTTP request timeout (msec)
// The following errors are likely the result of insuffient http client tcp 
// timeout:
//   -1   Connection Refused
//   -11  Read Timeout
//   -258 Deserialization Incomplete Input
#define DEF_HTTP_CLIENT_TCP_TIMEOUT 10000

// E-PAPER PANEL
// This project supports the following E-Paper panels:
//   DISP_BW_V2 - 7.5in e-Paper (v2)      800x480px  Black/White
//   DISP_3C_B  - 7.5in e-Paper (B)       800x480px  Red/Black/White
//   DISP_7C_F  - 7.3in ACeP e-Paper (F)  800x480px  7-Color
//   DISP_BW_V1 - 7.5in e-Paper (v1)      640x384px  Black/White
// Uncomment the macro that identifies your physical panel.
#define DISP_BW_V2
// #define DISP_3C_B
// #define DISP_7C_F
// #define DISP_BW_V1

// E-PAPER DRIVER BOARD
// The DESPI-C02 is the only officially supported driver board.
// Support for the Waveshare rev2.2 and rev2.3 is deprecated.
// The Waveshare rev2.2 is no longer in production.
// Users of the Waveshare rev2.3 have reported experiencing low contrast issues.
// Uncomment the macro that identifies your driver board hardware.
#define DRIVER_DESPI_C02
// #define DRIVER_WAVESHARE

// 3 COLOR E-INK ACCENT COLOR
// Defines the 3rd color to be used when a 3+ color display is selected.
#if defined(DISP_3C_B) || defined(DISP_7C_F)
  // #define ACCENT_COLOR GxEPD_BLACK
  #define ACCENT_COLOR GxEPD_RED
  // #define ACCENT_COLOR GxEPD_GREEN
  // #define ACCENT_COLOR GxEPD_BLUE
  // #define ACCENT_COLOR GxEPD_YELLOW
  // #define ACCENT_COLOR GxEPD_ORANGE
#endif

// LOCALE
// If your locale is not here, you can add it by copying and modifying one of
// the files in src/locales. Please feel free to create a pull request to add
// official support for your locale.
//   Language (Territory)            code
//   German (Germany)                de_DE
//   English (United Kingdom)        en_GB
//   English (United States)         en_US
//   Estonian (Estonia)              et_EE
//   Finnish (Finland)               fi_FI
//   French (France)                 fr_FR
//   Dutch (Belgium)                 nl_BE
//   Portuguese (Brazil)             pt_BR
#define LOCALE en_US

// TIME
// AUTO_TZ suppress TIMEZONE - no longer necessary
// Time format used when displaying sunrise/set times. (Max 11 characters)
// For more information about formatting see
// https://man7.org/linux/man-pages/man3/strftime.3.html
// #define DEF_TIME_FORMAT "%l:%M%P" // 12-hour ex: 1:23am  11:00pm
#define DEF_TIME_FORMAT "%H:%M"   // 24-hour ex: 01:23   23:00
// Time format used when displaying axis labels. (Max 11 characters)
// For more information about formatting see
// https://man7.org/linux/man-pages/man3/strftime.3.html
// #define DEF_HOUR_FORMAT "%l%P" // 12-hour ex: 1am  11pm
#define DEF_HOUR_FORMAT "%H"      // 24-hour ex: 01   23
// Date format used when displaying date in top-right corner.
// For more information about formatting see
// https://man7.org/linux/man-pages/man3/strftime.3.html
#define DEF_DATE_FORMAT "%a, %B %e" // ex: Sat, January 1
// #define DEF_DATE_FORMAT "%a %e %B" // ex: Sat 1 January
// Date/Time format used when displaying the last refresh time along the bottom
// of the screen.
// For more information about formatting see
// https://man7.org/linux/man-pages/man3/strftime.3.html
#define DEF_REFRESH_TIME_FORMAT "%x %H:%M"

// UNITS
// Define exactly one macro for each measurement type below.

// UNITS - TEMPERATURE
//   Metric   : Celsius
//   Imperial : Fahrenheit
// #define UNITS_TEMP_KELVIN
// #define UNITS_TEMP_CELSIUS
#define UNITS_TEMP_FAHRENHEIT

// UNITS - WIND SPEED
//   Metric   : Kilometers per Hour
//   Imperial : Miles per Hour
// #define UNITS_SPEED_METERSPERSECOND
// #define UNITS_SPEED_FEETPERSECOND
// #define UNITS_SPEED_KILOMETERSPERHOUR
#define UNITS_SPEED_MILESPERHOUR
// #define UNITS_SPEED_KNOTS
// #define UNITS_SPEED_BEAUFORT

// UNITS - PRESSURE
//   Metric   : Millibars
//   Imperial : Inches of Mercury
// #define UNITS_PRES_HECTOPASCALS
// #define UNITS_PRES_PASCALS
// #define UNITS_PRES_MILLIMETERSOFMERCURY
#define UNITS_PRES_INCHESOFMERCURY
// #define UNITS_PRES_MILLIBARS
// #define UNITS_PRES_ATMOSPHERES
// #define UNITS_PRES_GRAMSPERSQUARECENTIMETER
// #define UNITS_PRES_POUNDSPERSQUAREINCH

// UNITS - VISIBILITY DISTANCE
//   Metric   : Kilometers
//   Imperial : Miles
// #define UNITS_DIST_KILOMETERS
#define UNITS_DIST_MILES

// UNITS - PRECIPITATION (HOURLY)
// Measure of precipitation.
// This can either be Probability of Precipitation (PoP) or hourly volume.
//   Metric   : Millimeters
//   Imperial : Inches
#define UNITS_HOURLY_PRECIP_POP

#ifdef UNITS_HOURLY_PRECIP_POP 

// When using POP display
// hourly volume val may also be displayed as a thin line
#define POP_AND_VOL

#ifdef POP_AND_VOL
// In this case the volume unit shall also be indicated below
// #define POP_VOL_MM
// #define POP_VOL_CM
#define POP_VOL_INCHES

// Draw precipitation volume graph using thick line
// uncomment for single line (risk of shimmering)
#define POP_THICK_LINE

#endif // POP_AND_VOL

#else // UNITS_HOURLY_PRECIP_POP 

// Just indicate the volume unit
// #define UNITS_HOURLY_PRECIP_MILLIMETERS
// #define UNITS_HOURLY_PRECIP_CENTIMETERS
#define UNITS_HOURLY_PRECIP_INCHES

#endif // UNITS_HOURLY_PRECIP_POP 

// UNITS - PRECIPITATION (DAILY)
// Measure of precipitation.
// This can either be Probability of Precipitation (PoP) or daily volume.
//   Metric   : Millimeters
//   Imperial : Inches
// #define UNITS_DAILY_PRECIP_POP
// #define UNITS_DAILY_PRECIP_MILLIMETERS
// #define UNITS_DAILY_PRECIP_CENTIMETERS
#define UNITS_DAILY_PRECIP_INCHES

// Hypertext Transfer Protocol (HTTP)
// HTTP
//   HTTP does not provide encryption or any security measures, making it highly
//   vulnerable to eavesdropping and data tampering. Has the advantage of using
//   less power.
// HTTPS_NO_CERT_VERIF
//   HTTPS without X.509 certificate verification provides encryption but lacks
//   authentication and is susceptible to man-in-the-middle attacks.
// HTTPS_WITH_CERT_VERIF
//   HTTPS with X.509 certificate verification offers the highest level of
//   security by providing encryption and verifying the identity of the server.
//
//   HTTPS with X.509 certificate verification comes with the draw back that
//   eventually the certificates on the esp32 will expire, requiring you to
//   update the certificates in cert.h and reflash this software.
//   Running cert.py will generate an updated cert.h file.
//   The current certificate for api.openweathermap.org is valid until
//   2030-12-31 23:59:59.
// (uncomment exactly one)
// #define USE_HTTP
// #define USE_HTTPS_NO_CERT_VERIF
#define USE_HTTPS_WITH_CERT_VERIF

// WIND DIRECTION INDICATOR
// Choose whether the wind direction indicator should be an arrow, number, or
// expressed in Compass Point Notation (CPN).
// The arrow indicator can be combined with NUMBER or CPN.
//
//   PRECISION                  #     ERROR   EXAMPLE
//   Cardinal                   4  ±45.000°   E
//   Intercardinal (Ordinal)    8  ±22.500°   NE
//   Secondary Intercardinal   16  ±11.250°   NNE
//   Tertiary Intercardinal    32   ±5.625°   NbE
#define WIND_INDICATOR_ARROW
// #define WIND_INDICATOR_NUMBER
// #define WIND_INDICATOR_CPN_CARDINAL
// #define WIND_INDICATOR_CPN_INTERCARDINAL
// #define WIND_INDICATOR_CPN_SECONDARY_INTERCARDINAL
// #define WIND_INDICATOR_CPN_TERTIARY_INTERCARDINAL
// #define WIND_INDICATOR_NONE

// WIND DIRECTION ICON PRECISION
// The wind direction icon shown to the left of the wind speed can indicate wind
// direction with a minimum error of ±0.5°. This uses more flash storage because
// 360 24x24 wind direction icons must be stored, totaling ~25kB. For either
// preference or in case flash space becomes a concern there are a handful of
// selectable options listed below. 360 points seems excessive, but the option
// is there.
//
//   PRECISION                  #     ERROR  STORAGE
//   Cardinal                   4  ±45.000°     288B  E
//   Intercardinal (Ordinal)    8  ±22.500°     576B  NE
//   Secondary Intercardinal   16  ±11.250°   1,152B  NNE
//   Tertiary Intercardinal    32   ±5.625°   2,304B  NbE
//   (360)                    360   ±0.500°  25,920B  1°
// Uncomment your preferred wind level direction precision.
// #define WIND_ICONS_CARDINAL
// #define WIND_ICONS_INTERCARDINAL
#define WIND_ICONS_SECONDARY_INTERCARDINAL
// #define WIND_ICONS_TERTIARY_INTERCARDINAL
// #define WIND_ICONS_360

// FONTS
// A handful of popular Open Source typefaces have been included with this
// project for your convenience. Change the font by selecting its corresponding
// header file.
//
//   FONT           HEADER FILE              FAMILY          LICENSE
//   FreeMono       FreeMono.h               GNU FreeFont    GNU GPL v3.0
//   FreeSans       FreeSans.h               GNU FreeFont    GNU GPL v3.0
//   FreeSerif      FreeSerif.h              GNU FreeFont    GNU GPL v3.0
//   Lato           Lato_Regular.h           Lato            SIL OFL v1.1
//   Montserrat     Montserrat_Regular.h     Montserrat      SIL OFL v1.1
//   Open Sans      OpenSans_Regular.h       Open Sans       SIL OFL v1.1
//   Poppins        Poppins_Regular.h        Poppins         SIL OFL v1.1
//   Quicksand      Quicksand_Regular.h      Quicksand       SIL OFL v1.1
//   Raleway        Raleway_Regular.h        Raleway         SIL OFL v1.1
//   Roboto         Roboto_Regular.h         Roboto          Apache v2.0
//   Roboto Mono    RobotoMono_Regular.h     Roboto Mono     Apache v2.0
//   Roboto Slab    RobotoSlab_Regular.h     Roboto Slab     Apache v2.0
//   Ubuntu         Ubuntu_R.h               Ubuntu font     UFL v1.0
//   Ubuntu Mono    UbuntuMono_R.h           Ubuntu font     UFL v1.0
//
// Adding new fonts is relatively straightforward, see fonts/README.
//
// Note:
//   The layout of the display was designed around spacing and size of the GNU
//   FreeSans font, but this project supports the ability to modularly swap
//   fonts. Using a font other than FreeSans may result in undesired spacing or
//   other artifacts.
#define FONT_HEADER "fonts/FreeSans.h"

// DAILY PRECIPITATION
// Daily precipitation indicated under Hi|Lo can optionally be configured using
// the following options.
//   0 : Disable (hide always)
//   1 : Enable (show always)
//   2 : Smart (show only when precipitation is forecasted)
#define DISPLAY_DAILY_PRECIP 2

// ALERTS
//   The handling of alerts is complex. Each country has a unique national alert
//   system that receives alerts from many different government agencies. This
//   results is huge variance in the formatting of alerts. OpenWeatherMap
//   provides alerts in English only. Any combination of these factors may make
//   it undesirable to display alerts in some regions.
//   Disable alerts by changing the DISPLAY_ALERTS macro to 0.
#define DISPLAY_ALERTS 1

// STATUS BAR EXTRAS
//   Extra information that can be displayed on the status bar. Set to 1 to
//   enable.
#define STATUS_BAR_EXTRAS_BAT_VOLTAGE 1
#define STATUS_BAR_EXTRAS_WIFI_RSSI   1

// BATTERY MONITORING
//   You may choose to power your weather display with or without a battery.
//   Low power behavior can be controlled in config.cpp.
//   If you wish to disable battery monitoring set this macro to 0.
#define BATTERY_MONITORING 1

// NON-VOLATILE STORAGE (NVS) NAMESPACE
#define NVS_NAMESPACE "weather_epd"

// DEBUG
//   If defined, enables increase verbosity over the serial port.
//   level 0: basic status information, assists troubleshooting (default)
//   level 1: increased verbosity for debugging
//   level 2: print api responses to serial monitor
#define DEBUG_LEVEL 0

// Set the below constants in "config.cpp"
extern const uint8_t PIN_BAT_ADC;
extern const uint8_t PIN_EPD_BUSY;
extern const uint8_t PIN_EPD_CS;
extern const uint8_t PIN_EPD_RST;
extern const uint8_t PIN_EPD_DC;
extern const uint8_t PIN_EPD_SCK;
extern const uint8_t PIN_EPD_MISO;
extern const uint8_t PIN_EPD_MOSI;
extern const uint8_t PIN_EPD_PWR;
extern const uint8_t PIN_BME_SDA;
extern const uint8_t PIN_BME_SCL;
extern const uint8_t PIN_BME_PWR;
extern const uint8_t BME_ADDRESS;
extern const char *WIFI_SSID;
extern const char *WIFI_PASSWORD;
extern const unsigned long WIFI_TIMEOUT;
extern const unsigned HTTP_CLIENT_TCP_TIMEOUT;
extern const String OWM_APIKEY;
extern const String OWM_ENDPOINT;
extern const String OWM_ONECALL_VERSION;
#ifdef WEB_SVR
// Remap references to these data
#define LAT              VLat[defloc]
#define LON              VLon[defloc]
#define CITY_STRING      VLoc[defloc]
#define BED_TIME         BedTime
#define WAKE_TIME        WakeTime
#define SLEEP_DURATION   SleepDly
#define HOURLY_GRAPH_MAX HourlyNb

#else // WEB_SVR
extern const String LAT;
extern const String LON;
extern const String CITY_STRING;
extern const int BED_TIME;
extern const int WAKE_TIME;
extern const long SLEEP_DURATION;
extern const int HOURLY_GRAPH_MAX;
#endif // WEB_SVR
extern const char *TIME_FORMAT;
extern const char *HOUR_FORMAT;
extern const char *DATE_FORMAT;
extern const char *REFRESH_TIME_FORMAT;
extern const uint32_t MAX_BATTERY_VOLTAGE;
extern const uint32_t WARN_BATTERY_VOLTAGE;
extern const uint32_t LOW_BATTERY_VOLTAGE;
extern const uint32_t VERY_LOW_BATTERY_VOLTAGE;
extern const uint32_t CRIT_LOW_BATTERY_VOLTAGE;
extern const unsigned long LOW_BATTERY_SLEEP_INTERVAL;
extern const unsigned long VERY_LOW_BATTERY_SLEEP_INTERVAL;

// CONFIG VALIDATION - DO NOT MODIFY
#if !(  defined(DISP_BW_V2)  \
      ^ defined(DISP_3C_B)   \
      ^ defined(DISP_7C_F)   \
      ^ defined(DISP_BW_V1))
  #error Invalid configuration. Exactly one display panel must be selected.
#endif
#if !(  defined(DRIVER_WAVESHARE) \
      ^ defined(DRIVER_DESPI_C02))
  #error Invalid configuration. Exactly one driver board must be selected.
#endif
#if !(defined(LOCALE))
  #error Invalid configuration. Locale not selected.
#endif
#if !(  defined(UNITS_TEMP_KELVIN)      \
      ^ defined(UNITS_TEMP_CELSIUS)     \
      ^ defined(UNITS_TEMP_FAHRENHEIT))
  #error Invalid configuration. Exactly one temperature unit must be selected.
#endif
#if !(  defined(UNITS_SPEED_METERSPERSECOND)   \
      ^ defined(UNITS_SPEED_FEETPERSECOND)     \
      ^ defined(UNITS_SPEED_KILOMETERSPERHOUR) \
      ^ defined(UNITS_SPEED_MILESPERHOUR)      \
      ^ defined(UNITS_SPEED_KNOTS)             \
      ^ defined(UNITS_SPEED_BEAUFORT))
  #error Invalid configuration. Exactly one wind speed unit must be selected.
#endif
#if !(  defined(UNITS_PRES_HECTOPASCALS)             \
      ^ defined(UNITS_PRES_PASCALS)                  \
      ^ defined(UNITS_PRES_MILLIMETERSOFMERCURY)     \
      ^ defined(UNITS_PRES_INCHESOFMERCURY)          \
      ^ defined(UNITS_PRES_MILLIBARS)                \
      ^ defined(UNITS_PRES_ATMOSPHERES)              \
      ^ defined(UNITS_PRES_GRAMSPERSQUARECENTIMETER) \
      ^ defined(UNITS_PRES_POUNDSPERSQUAREINCH))
  #error Invalid configuration. Exactly one pressure unit must be selected.
#endif
#if !(  defined(UNITS_DIST_KILOMETERS) \
      ^ defined(UNITS_DIST_MILES))
  #error Invalid configuration. Exactly one distance unit must be selected.
#endif
#if !(  defined(UNITS_HOURLY_PRECIP_POP)         \
      ^ defined(UNITS_HOURLY_PRECIP_MILLIMETERS) \
      ^ defined(UNITS_HOURLY_PRECIP_CENTIMETERS) \
      ^ defined(UNITS_HOURLY_PRECIP_INCHES))
  #error Invalid configuration. Exactly one hourly precipitation measurement must be selected.
#endif
#if !(  defined(UNITS_DAILY_PRECIP_POP)         \
      ^ defined(UNITS_DAILY_PRECIP_MILLIMETERS) \
      ^ defined(UNITS_DAILY_PRECIP_CENTIMETERS) \
      ^ defined(UNITS_DAILY_PRECIP_INCHES))
  #error Invalid configuration. Exactly one daily precipitation measurement must be selected.
#endif
#if !(  defined(USE_HTTP)                   \
      ^ defined(USE_HTTPS_NO_CERT_VERIF)    \
      ^ defined(USE_HTTPS_WITH_CERT_VERIF))
  #error Invalid configuration. Exactly one HTTP mode must be selected.
#endif
#if !(  defined(WIND_INDICATOR_ARROW)                         \
      || (                                                    \
          defined(WIND_INDICATOR_NUMBER)                      \
        ^ defined(WIND_INDICATOR_CPN_CARDINAL)                \
        ^ defined(WIND_INDICATOR_CPN_INTERCARDINAL)           \
        ^ defined(WIND_INDICATOR_CPN_SECONDARY_INTERCARDINAL) \
        ^ defined(WIND_INDICATOR_CPN_TERTIARY_INTERCARDINAL)  \
      )                                                       \
      ^ defined(WIND_INDICATOR_NONE))
  #error Invalid configuration. Illegal selction of wind indicator(s).
#endif
#if defined(WIND_INDICATOR_ARROW)                   \
 && !(  defined(WIND_ICONS_CARDINAL)                \
      ^ defined(WIND_ICONS_INTERCARDINAL)           \
      ^ defined(WIND_ICONS_SECONDARY_INTERCARDINAL) \
      ^ defined(WIND_ICONS_TERTIARY_INTERCARDINAL)  \
      ^ defined(WIND_ICONS_360))
  #error Invalid configuration. Exactly one wind direction icon precision level must be selected.
#endif
#if !(defined(FONT_HEADER))
  #error Invalid configuration. Font not selected.
#endif
#if !(defined(DISPLAY_DAILY_PRECIP))
  #error Invalid configuration. DISPLAY_DAILY_PRECIP not defined.
#endif
#if !(defined(DISPLAY_ALERTS))
  #error Invalid configuration. DISPLAY_ALERTS not defined.
#endif
#if !(defined(BATTERY_MONITORING))
  #error Invalid configuration. BATTERY_MONITORING not defined.
#endif
#if !(defined(DEBUG_LEVEL))
  #error Invalid configuration. DEBUG_LEVEL not defined.
#endif

#endif
