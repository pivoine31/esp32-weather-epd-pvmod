/* Configuration options for esp32-weather-epd.
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

#include <Arduino.h>
#include "config.h"

// See config.h for the main custom options

// PINS
// The configuration below is intended for use with the project's official 
// wiring diagrams using the FireBeetle 2 ESP32-E microcontroller board.
//
// Note: LED_BUILTIN pin will be disabled to reduce power draw.  Refer to your
//       board's pinout to ensure you avoid using a pin with this shared 
//       functionality.
//
// ADC pin used to measure battery voltage
const uint8_t PIN_BAT_ADC  = A2; // A0 for micro-usb firebeetle
// Pins for E-Paper Driver Board
const uint8_t PIN_EPD_BUSY = 14; // 5 for micro-usb firebeetle
const uint8_t PIN_EPD_CS   = 13;
const uint8_t PIN_EPD_RST  = 21;
const uint8_t PIN_EPD_DC   = 22;
const uint8_t PIN_EPD_SCK  = 18;
const uint8_t PIN_EPD_MISO = 19; // 19 Master-In Slave-Out not used, as no data from display
const uint8_t PIN_EPD_MOSI = 23;
const uint8_t PIN_EPD_PWR  = 26; // Irrelevant if directly connected to 3.3V
// I2C Pins used for BME280
const uint8_t PIN_BME_SDA = 17;
const uint8_t PIN_BME_SCL = 16;
const uint8_t PIN_BME_PWR =  4;   // Irrelevant if directly connected to 3.3V
const uint8_t BME_ADDRESS = 0x76; // If sensor does not work, try 0x77

// Values in DEF* are defined in config.h

// WIFI
// Default SSID / Password
// WIFI_MULTI credentials now defined in config.h

// WiFi connection timeout (msec)
// WIFI_MULTI Increase value when multiples AP in use
const unsigned long WIFI_TIMEOUT = DEF_WIFI_TIMEOUT;

// HTTP request timeout (msec)
// The following errors are likely the result of insufficient http client tcp 
// timeout:
//   -1   Connection Refused
//   -11  Read Timeout
//   -258 Deserialization Incomplete Input
const unsigned HTTP_CLIENT_TCP_TIMEOUT = 10000; // ms

// OPENWEATHERMAP API
// OpenWeatherMap API key, https://openweathermap.org/
const String OWM_APIKEY   = DEFAPIKEY;
const String OWM_ENDPOINT = "api.openweathermap.org";
// OpenWeatherMap One Call 2.5 API is deprecated for all new free users
// (accounts created after Summer 2022).
//
// Please note, that One Call API 3.0 is included in the "One Call by Call"
// subscription only. This separate subscription includes 1,000 calls/day for
// free and allows you to pay only for the number of API calls made to this
// product.
//
// Here is how to subscribe and avoid any credit card changes:
// - Go to https://home.openweathermap.org/subscriptions/billing_info/onecall_30/base?key=base&service=onecall_30
// - Follow the instructions to complete the subscription.
// - /Go to https://home.openweathermap.org/subscriptions and set the "Calls per
//   day (no more than)" to 1,000. This ensures you will never overrun the free
//   calls.
const String OWM_ONECALL_VERSION = "3.0";

// LOCATION
// Set your latitude and longitude.
// (used to get weather data as part of API requests to OpenWeatherMap)
#ifdef WEB_SVR
// LAT, LON and CITY_STRING coming from the WEB server data
#else
const String LAT = DEFLAT;
const String LON = DEFLON;
// City name that will be shown in the top-right corner of the display.
const String CITY_STRING = DEFCITY;
#endif // WEB_SVR


// TIME FORMAT moved to config.h
// AUTO_TZ suppress TIMEZONE - no longer necessary
// Time format used when displaying sunrise/set times. (Max 11 characters)
const char *TIME_FORMAT = DEF_TIME_FORMAT;
// Time format used when displaying axis labels. (Max 11 characters)
const char *HOUR_FORMAT = DEF_HOUR_FORMAT;
// Date format used when displaying date in top-right corner.
const char *DATE_FORMAT = DEF_DATE_FORMAT;
// Date/Time format used when displaying the last refresh time along the bottom
// of the screen.
const char *REFRESH_TIME_FORMAT = DEF_REFRESH_TIME_FORMAT;

/* AUTO_TZ suppress NTP management - no longer necessary */

#ifdef WEB_SVR
// SLEEP_DURATION, BED_TIME and CITY_STRING coming from the WEB server data
#else
const long SLEEP_DURATION = DEFSLEEP;
const int BED_TIME  = DEFBED;  // Last update at 00:00 (midnight) until WAKE_TIME.
const int WAKE_TIME = DEFWAKE; // Hour of first update after BED_TIME, 06:00.

// HOURLY OUTLOOK GRAPH
// Number of hours to display on the outlook graph. (range: [8-48])
const int HOURLY_GRAPH_MAX = DEFHOURNB;
#endif // WEB_SVR

// BATTERY
// To protect the battery upon LOW_BATTERY_VOLTAGE, the display will cease to
// update until battery is charged again. The ESP32 will deep-sleep (consuming
// < 11uA), waking briefly check the voltage at the corresponding interval (in
// minutes). Once the battery voltage has fallen to CRIT_LOW_BATTERY_VOLTAGE,
// the esp32 will hibernate and a manual press of the reset (RST) button to
// begin operating again.
const uint32_t MAX_BATTERY_VOLTAGE      = 4200; // (millivolts)
const uint32_t WARN_BATTERY_VOLTAGE     = 3400; // (millivolts)
const uint32_t LOW_BATTERY_VOLTAGE      = 3200; // (millivolts)
const uint32_t VERY_LOW_BATTERY_VOLTAGE = 3100; // (millivolts)
const uint32_t CRIT_LOW_BATTERY_VOLTAGE = 3000; // (millivolts)
const unsigned long LOW_BATTERY_SLEEP_INTERVAL      = 30;  // (minutes)
const unsigned long VERY_LOW_BATTERY_SLEEP_INTERVAL = 120; // (minutes)

