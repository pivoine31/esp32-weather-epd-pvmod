/* Main program for esp32-weather-epd.
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
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Preferences.h>
#include <time.h>
#include <WiFi.h>
#include <Wire.h>

#include "_locale.h"
#include "api_response.h"
#include "client_utils.h"
#include "config.h"
#include "display_utils.h"
#include "icons/icons_196x196.h"
#include "renderer.h"
#if defined(USE_HTTPS_WITH_CERT_VERIF) || defined(USE_HTTPS_NO_CERT_VERIF)
  #include <WiFiClientSecure.h>
#endif
#ifdef USE_HTTPS_WITH_CERT_VERIF
  #include "cert.h"
#endif

// too large to allocate locally on stack
static owm_resp_onecall_t       owm_onecall;
static owm_resp_air_pollution_t owm_air_pollution;

Preferences prefs;

unsigned long startTime;
unsigned long actionTime;

/*
 * restart_wdg
 *
 * Restart the wathdog timer while web user is acting
 */
void restart_wdg ( void )
{
  actionTime = millis();
}

/*
 * do_deep_sleep
 *
 * Actually enter deep sleep (on end of refresh, or on web server exit)
 */ 
void do_deep_sleep ( uint64_t sleepDuration )
{
#ifdef WEB_SVR
#ifdef BUTTON_PIN
  // Enable Wake Up with Button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  esp_sleep_enable_ext0_wakeup(BUTTON_PIN,0);
#else
  // Enable Wake Up with touch pin
  touchSleepWakeUpEnable(TOUCH_PIN,TOUCH_THR);
#endif
#endif // WEB_SVR

  esp_sleep_enable_timer_wakeup(sleepDuration * 1000ULL);
  Serial.print(TXT_AWAKE_FOR);
  Serial.println(" "  + String((millis() - startTime) / 1000.0, 3) + "s");
  Serial.print(TXT_ENTERING_DEEP_SLEEP_FOR);
  Serial.println(" " + String(sleepDuration/1000ULL) + "s");

  esp_deep_sleep_start();
}


/*
 * beginDeepSleep
 *
 * Put esp32 into ultra low-power deep sleep (<11uA).
 * Aligns wake time to the minute. Sleep times defined in config.cpp.
 */
void beginDeepSleep(unsigned long &startTime, tm *timeInfo)
{
#ifdef BEFORE_NEVER_CNX
#else
  int wifi_had_cnx = 1;
#endif

  delay(500);

  /* AUTO_TZ */
  // Assume time info have been set before (as far as possible)
  wifi_had_cnx = ((timeInfo->tm_year+1900) >= 2016) ? 1 : 0;

  uint64_t sleepDuration = 0;
  int extraHoursUntilWake = 0;
  int curHour = timeInfo->tm_hour;

  if (timeInfo->tm_min >= 58)
  { // if we are within 2 minutes of the next hour, then round up for the
    // purposes of bed time
    curHour = (curHour + 1) % 24;
    extraHoursUntilWake += 1;
  }

  // Wifi did not succeed -> don't check bed time since clock may not be reliable
  if ( !wifi_had_cnx )
    extraHoursUntilWake = 0;
  else
  if (BED_TIME < WAKE_TIME && curHour >= BED_TIME && curHour < WAKE_TIME)
  { // 0              B   v  W  24
    // |--------------zzzzZzz---|
    extraHoursUntilWake += WAKE_TIME - curHour;
  }
  else if (BED_TIME > WAKE_TIME && curHour < WAKE_TIME)
  { // 0 v W               B    24
    // |zZz----------------zzzzz|
    extraHoursUntilWake += WAKE_TIME - curHour;
  }
  else if (BED_TIME > WAKE_TIME && curHour >= BED_TIME)
  { // 0   W               B  v 24
    // |zzz----------------zzzZz|
    extraHoursUntilWake += WAKE_TIME - (curHour - 24);
  }
  else // This feature is disabled (BED_TIME == WAKE_TIME)
  {    // OR it is not past BED_TIME
    extraHoursUntilWake = 0;
  }

  if (extraHoursUntilWake == 0)
  { // align wake time to nearest multiple of SLEEP_DURATION
    sleepDuration = SLEEP_DURATION * 60ULL
                    - ((timeInfo->tm_min % SLEEP_DURATION) * 60ULL
                        + timeInfo->tm_sec);
  }
  else
  { // align wake time to the hour
    sleepDuration = extraHoursUntilWake * 3600ULL
                    - (timeInfo->tm_min * 60ULL + timeInfo->tm_sec);
  }

  // if we are within 2 minutes of the next alignment.
  if (sleepDuration <= 120ULL)
  {
    sleepDuration += SLEEP_DURATION * 60ULL;
  }

  // add extra delay to compensate for esp32's with fast RTCs.
  sleepDuration += 10ULL;

#if DEBUG_LEVEL >= 1
  printHeapUsage();
#endif

  do_deep_sleep(sleepDuration * 1000ULL);
} // end beginDeepSleep


#ifdef WEB_SVR
/*
 * print_wakeup_reason
 *
 * Print the origin of wake-up (power-up, timer or button)
 */
esp_sleep_wakeup_cause_t print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 :
      Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 :
      Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER :
      Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    //case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default :
      Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason);
      break;
  }

  return wakeup_reason;
}
#endif // WEB_SVR

/*
 * setup
 *
 * Program initialization
 */
void setup()
{
#ifdef WEB_SVR
  int manual_wakeup;
#endif

  actionTime = startTime = millis();
  Serial.begin(115200);

#ifdef WEB_SVR
  // Fetch Weather and Wifi data from non volatile memory
  retrieve_config();

  // Check if button pressed
#ifdef BUTTON_PIN
  manual_wakeup = (print_wakeup_reason() == ESP_SLEEP_WAKEUP_EXT0);
#else
  manual_wakeup = (print_wakeup_reason() == ESP_SLEEP_WAKEUP_TOUCHPAD);
#endif
  if ( manual_wakeup )
    Serial.println("Awaked, please press button again to go back to sleep");
#endif // WEB_SVR

#if DEBUG_LEVEL >= 1
  printHeapUsage();
#endif

  disableBuiltinLED();

  // Open namespace for read/write to non-volatile storage
  prefs.begin(NVS_NAMESPACE, false);

#if BATTERY_MONITORING
  uint32_t batteryVoltage = readBatteryVoltage();
  Serial.print(TXT_BATTERY_VOLTAGE);
  Serial.println(": " + String(batteryVoltage) + "mv");

  // When the battery is low, the display should be updated to reflect that, but
  // only the first time we detect low voltage. The next time the display will
  // refresh is when voltage is no longer low. To keep track of that we will
  // make use of non-volatile storage.
  bool lowBat = prefs.getBool("lowBat", false);

  // low battery, deep sleep now
  if (batteryVoltage <= LOW_BATTERY_VOLTAGE)
  {
    if (lowBat == false)
    { // battery is now low for the first time
      prefs.putBool("lowBat", true);
      prefs.end();
      initDisplay(0);
      do
      {
        drawError(battery_alert_0deg_196x196, TXT_LOW_BATTERY);
      } while (display.nextPage());
      powerOffDisplay();
    }

    if (batteryVoltage <= CRIT_LOW_BATTERY_VOLTAGE)
    { // critically low battery
      // don't set esp_sleep_enable_timer_wakeup();
      // We won't wake up again until someone manually presses the RST button.
      Serial.println(TXT_CRIT_LOW_BATTERY_VOLTAGE);
      Serial.println(TXT_HIBERNATING_INDEFINITELY_NOTICE);
    }
    else if (batteryVoltage <= VERY_LOW_BATTERY_VOLTAGE)
    { // very low battery
      esp_sleep_enable_timer_wakeup(VERY_LOW_BATTERY_SLEEP_INTERVAL
                                    * 60ULL * 1000000ULL);
      Serial.println(TXT_VERY_LOW_BATTERY_VOLTAGE);
      Serial.print(TXT_ENTERING_DEEP_SLEEP_FOR);
      Serial.println(" " + String(VERY_LOW_BATTERY_SLEEP_INTERVAL) + "min");
    }
    else
    { // low battery
      esp_sleep_enable_timer_wakeup(LOW_BATTERY_SLEEP_INTERVAL
                                    * 60ULL * 1000000ULL);
      Serial.println(TXT_LOW_BATTERY_VOLTAGE);
      Serial.print(TXT_ENTERING_DEEP_SLEEP_FOR);
      Serial.println(" " + String(LOW_BATTERY_SLEEP_INTERVAL) + "min");
    }
    esp_deep_sleep_start();
  }
  // battery is no longer low, reset variable in non-volatile storage
  if (lowBat == true)
  {
    prefs.putBool("lowBat", false);
  }
#else
  uint32_t batteryVoltage = UINT32_MAX;
#endif

  // All data should have been loaded from NVS. Close filesystem.
  prefs.end();

  String statusStr = {};
  String tmpStr = {};
  tm timeInfo = {};

  // START WIFI
  int wifiRSSI = 0; // Received Signal Strength Indicator
  wl_status_t wifiStatus = startWiFi(wifiRSSI);
  if (wifiStatus != WL_CONNECTED)
  { // WiFi Connection Failed
    killWiFi();
    initDisplay(0);
    if (wifiStatus == WL_NO_SSID_AVAIL)
    {
      Serial.println(TXT_NETWORK_NOT_AVAILABLE);
      do
      {
        drawError(wifi_x_196x196, TXT_NETWORK_NOT_AVAILABLE);
      } while (display.nextPage());
    }
    else
    {
      Serial.println(TXT_WIFI_CONNECTION_FAILED);
      do
      {
        drawError(wifi_x_196x196, TXT_WIFI_CONNECTION_FAILED);
      } while (display.nextPage());
    }
    powerOffDisplay();
    beginDeepSleep(startTime, &timeInfo);
  }

  /* AUTO_TZ no need for time synchronisation */

  // MAKE API REQUESTS
#ifdef USE_HTTP
  WiFiClient client;
#elif defined(USE_HTTPS_NO_CERT_VERIF)
  WiFiClientSecure client;
  client.setInsecure();
#elif defined(USE_HTTPS_WITH_CERT_VERIF)
  WiFiClientSecure client;
  client.setCACert(cert_Sectigo_RSA_Domain_Validation_Secure_Server_CA);
#endif
  int rxStatus = getOWMonecall(client, owm_onecall);
  if (rxStatus != HTTP_CODE_OK)
  {
    killWiFi();
    statusStr = "One Call " + OWM_ONECALL_VERSION + " API";
    tmpStr = String(rxStatus, DEC) + ": " + getHttpResponsePhrase(rxStatus);
    initDisplay(0); // WEB_SVR
    do
    {
      drawError(wi_cloud_down_196x196, statusStr, tmpStr);
    } while (display.nextPage());
    powerOffDisplay();
    beginDeepSleep(startTime, &timeInfo);
  }
  rxStatus = getOWMairpollution(client, owm_air_pollution);
  if (rxStatus != HTTP_CODE_OK)
  {
    killWiFi();
    statusStr = "Air Pollution API";
    tmpStr = String(rxStatus, DEC) + ": " + getHttpResponsePhrase(rxStatus);
    initDisplay(0); // WEB_SVR
    do
    {
      drawError(wi_cloud_down_196x196, statusStr, tmpStr);
    } while (display.nextPage());
    powerOffDisplay();
    beginDeepSleep(startTime, &timeInfo);
  }
  killWiFi(); // WiFi no longer needed

  // GET INDOOR TEMPERATURE AND HUMIDITY, start BME280...
  pinMode(PIN_BME_PWR, OUTPUT);
  digitalWrite(PIN_BME_PWR, HIGH);
  float inTemp     = NAN;
  float inHumidity = NAN;
  Serial.print(String(TXT_READING_FROM) + " BME280... ");
  TwoWire I2C_bme = TwoWire(0);
  Adafruit_BME280 bme;

  I2C_bme.begin(PIN_BME_SDA, PIN_BME_SCL, 100000); // 100kHz
  if(bme.begin(BME_ADDRESS, &I2C_bme))
  {
    inTemp     = bme.readTemperature(); // Celsius
    inHumidity = bme.readHumidity();    // %

    // check if BME readings are valid
    // note: readings are checked again before drawing to screen. If a reading
    //       is not a number (NAN) then an error occurred, a dash '-' will be
    //       displayed.
    if (std::isnan(inTemp) || std::isnan(inHumidity))
    {
      statusStr = "BME " + String(TXT_READ_FAILED);
      Serial.println(statusStr);
    }
    else
    {
      Serial.println(TXT_SUCCESS);
    }
  }
  else
  {
    statusStr = "BME " + String(TXT_NOT_FOUND); // check wiring
    Serial.println(statusStr);
  }
  digitalWrite(PIN_BME_PWR, LOW);

  String refreshTimeStr;

  /* AUTO_TZ */
  // Use date, time and timezone offset found in the OWM response
  time_t ts = owm_onecall.current.dt + owm_onecall.timezone_offset;
  tm   * ti = localtime(&ts);
  memcpy(&timeInfo,ti,sizeof(tm)); // needed because localtime returns its own struct
  getRefreshTimeStr(refreshTimeStr, true, &timeInfo);
  /* AUTO_TZ */

  String dateStr;
  getDateStr(dateStr, &timeInfo);

  // RENDER FULL REFRESH
  initDisplay(0); // WEB_SVR
  do
  {
    drawCurrentConditions(owm_onecall.current, owm_onecall.daily[0],
                          owm_air_pollution, inTemp, inHumidity,
			  owm_onecall.timezone_offset);  // AUTO_TZ
    drawForecast(owm_onecall.daily, timeInfo);
    drawLocationDate(CITY_STRING, dateStr);
    drawOutlookGraph(owm_onecall.hourly,owm_onecall.timezone_offset); // AUTO_TZ
#if DISPLAY_ALERTS
    drawAlerts(owm_onecall.alerts, CITY_STRING, dateStr);
#endif
    drawStatusBar(statusStr, refreshTimeStr, wifiRSSI, batteryVoltage);

#ifdef WEB_SVR
    if ( manual_wakeup )
      // Draw Web symbol in upper left corner
      drawWebIcon(1);
#endif
  }
  while (display.nextPage());

#ifdef WEB_SVR
  if ( manual_wakeup )
  {
    pinMode(GPIO_NUM_27, INPUT_PULLUP);

    // Start Web serveur and exit setup
    web_svr_setup();
    return;
  }
  else
#endif

  powerOffDisplay();

  // DEEP SLEEP
  beginDeepSleep(startTime, &timeInfo);
} // end setup


/*
 * loop
 *
 * Will run only when the Web server is active
 */
void loop()
{
#ifdef WEB_SVR

#ifdef BUTTON_PIN
  static int lastState = 1;
  
  int currentState;
#endif
  
  tm  timeInfo = {};

  wifi_check();  // WIFI_MULTI

#ifdef BUTTON_PIN
  // Avoid checking button too early after manual wakeup
  if ( (millis() - startTime) >= MIN_BUT_CHK )  // Rollover handled by C substract
  {
    currentState = digitalRead(BUTTON_PIN);

    if (lastState && !currentState)
    {
      Serial.println("The button is pressed");
    }
    else if (!lastState && currentState)
    {
      // Once button is pressed, go back to sleep
      Serial.println("The button is released");

      drawWebIcon(0);

      // Since timeinfo not initialised, bed/wake time is not checked this time
      // (but will be next time)
      beginDeepSleep(startTime, &timeInfo);
    }
    lastState = currentState;
  }
#else // BUTTON_PIN
  // Avoid checking touchpin too early after manual wakeup
  if ( (millis() - startTime) >= MIN_BUT_CHK )  // Rollover handled by C substract
  {
    if ( touchRead(TOUCH_PIN) <= TOUCH_THR )
    {
      // Once touchpin hit, go back to sleep
      Serial.println("The touchpin is hit");

      drawWebIcon(0);

      // Since timeinfo not initialised, bed/wake time is not checked this time
      // (but will be next time)
      beginDeepSleep(startTime, &timeInfo);
    }
  }
#endif // BUTTON_PIN

  if ( (millis() - actionTime) >= (MaxActTim*1000) )  // Rollover handled by C substract
  {
    // Deep sleep also on timeout
    Serial.println("Watch dog timer elapsed");

    drawWebIcon(0);
    
    beginDeepSleep(startTime, &timeInfo);
  }

  delay(1000);

#endif // WEB_SVR
} // end loop
