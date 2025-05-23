/* Renderer for esp32-weather-epd.
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

#include "_locale.h"
#include "_strftime.h"
#include "renderer.h"
#include "api_response.h"
#include "config.h"
#include "conversions.h"
#include "client_utils.h"
#include "display_utils.h"

// fonts
#include FONT_HEADER

// icon header files
#include "icons/icons_16x16.h"
#include "icons/icons_24x24.h"
#include "icons/icons_32x32.h"
#include "icons/icons_40x40.h"  //WEB_SVR
#include "icons/icons_48x48.h"
#include "icons/icons_64x64.h"
#include "icons/icons_96x96.h"
#include "icons/icons_128x128.h"
#include "icons/icons_160x160.h"
#include "icons/icons_196x196.h"

#ifdef DISP_BW_V2
  GxEPD2_BW<GxEPD2_750_T7,
            GxEPD2_750_T7::HEIGHT> display(
    GxEPD2_750_T7(PIN_EPD_CS,
                  PIN_EPD_DC,
                  PIN_EPD_RST,
                  PIN_EPD_BUSY));
#endif
#ifdef DISP_3C_B
  GxEPD2_3C<GxEPD2_750c_Z08,
            GxEPD2_750c_Z08::HEIGHT / 2> display(
    GxEPD2_750c_Z08(PIN_EPD_CS,
                    PIN_EPD_DC,
                    PIN_EPD_RST,
                    PIN_EPD_BUSY));
#endif
#ifdef DISP_7C_F
  GxEPD2_7C<GxEPD2_730c_GDEY073D46,
            GxEPD2_730c_GDEY073D46::HEIGHT / 4> display(
    GxEPD2_730c_GDEY073D46(PIN_EPD_CS,
                           PIN_EPD_DC,
                           PIN_EPD_RST,
                           PIN_EPD_BUSY));
#endif
#ifdef DISP_BW_V1
  GxEPD2_BW<GxEPD2_750,
            GxEPD2_750::HEIGHT> display(
    GxEPD2_750(PIN_EPD_CS,
               PIN_EPD_DC,
               PIN_EPD_RST,
               PIN_EPD_BUSY));
#endif

#ifndef ACCENT_COLOR
  #define ACCENT_COLOR GxEPD_BLACK
#endif

/*
 * GetStringWidth
 *
 * Returns the string width in pixels
 */
uint16_t getStringWidth(const String &text)
{
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return w;
}

/*
 * GetStringHeigh
 *
 * Returns the string height in pixels
 */
uint16_t getStringHeight(const String &text)
{
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return h;
}

/*
 * DrawString
 *
 * Draws a string with alignment
 */
void drawString(int16_t x, int16_t y, const String &text, alignment_t alignment,
                uint16_t color)
{
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextColor(color);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (alignment == RIGHT)
  {
    x = x - w;
  }
  if (alignment == CENTER)
  {
    x = x - w / 2;
  }
  display.setCursor(x, y);
  display.print(text);
  return;
} // end drawString

/*
 * DrawMultiLnString
 *
 * Draws a string that will flow into the next line when max_width is reached.
 * If a string exceeds max_lines an ellipsis (...) will terminate the last word.
 * Lines will break at spaces(' ') and dashes('-').
 *
 * Note: max_width should be big enough to accommodate the largest word that
 *       will be displayed. If an unbroken string of characters longer than
 *       max_width exist in text, then the string will be printed beyond
 *       max_width.
 */
void drawMultiLnString(int16_t x, int16_t y, const String &text,
                       alignment_t alignment, uint16_t max_width,
                       uint16_t max_lines, int16_t line_spacing,
                       uint16_t color)
{
  uint16_t current_line = 0;
  String textRemaining = text;
  // print until we reach max_lines or no more text remains
  while (current_line < max_lines && !textRemaining.isEmpty())
  {
    int16_t  x1, y1;
    uint16_t w, h;

    display.getTextBounds(textRemaining, 0, 0, &x1, &y1, &w, &h);

    int endIndex = textRemaining.length();
    // check if remaining text is to wide, if it is then print what we can
    String subStr = textRemaining;
    int splitAt = 0;
    int keepLastChar = 0;
    while (w > max_width && splitAt != -1)
    {
      if (keepLastChar)
      {
        // if we kept the last character during the last iteration of this while
        // loop, remove it now so we don't get stuck in an infinite loop.
        subStr.remove(subStr.length() - 1);
      }

      // find the last place in the string that we can break it.
      if (current_line < max_lines - 1)
      {
        splitAt = std::max(subStr.lastIndexOf(" "),
                           subStr.lastIndexOf("-"));
      }
      else
      {
        // this is the last line, only break at spaces so we can add ellipsis
        splitAt = subStr.lastIndexOf(" ");
      }

      // if splitAt == -1 then there is an unbroken set of characters that is
      // longer than max_width. Otherwise if splitAt != -1 then we can continue
      // the loop until the string is <= max_width
      if (splitAt != -1)
      {
        endIndex = splitAt;
        subStr = subStr.substring(0, endIndex + 1);

        char lastChar = subStr.charAt(endIndex);
        if (lastChar == ' ')
        {
          // remove this char now so it is not counted towards line width
          keepLastChar = 0;
          subStr.remove(endIndex);
          --endIndex;
        }
        else if (lastChar == '-')
        {
          // this char will be printed on this line and removed next iteration
          keepLastChar = 1;
        }

        if (current_line < max_lines - 1)
        {
          // this is not the last line
          display.getTextBounds(subStr, 0, 0, &x1, &y1, &w, &h);
        }
        else
        {
          // this is the last line, we need to make sure there is space for
          // ellipsis
          display.getTextBounds(subStr + "...", 0, 0, &x1, &y1, &w, &h);
          if (w <= max_width)
          {
            // ellipsis fit, add them to subStr
            subStr = subStr + "...";
          }
        }

      } // end if (splitAt != -1)
    } // end inner while

    drawString(x, y + (current_line * line_spacing), subStr, alignment, color);

    // update textRemaining to no longer include what was printed
    // +1 for exclusive bounds, +1 to get passed space/dash
    textRemaining = textRemaining.substring(endIndex + 2 - keepLastChar);

    ++current_line;
  } // end outer while

  return;
} // end drawMultiLnString

/*
 * InitDisplay
 *
 * Initialize e-paper display
 */
void initDisplay ( int partial )
{
  pinMode(PIN_EPD_PWR, OUTPUT);
  digitalWrite(PIN_EPD_PWR, HIGH);

  // Just to avoid runtime warning in digitalWrite
  pinMode(PIN_EPD_CS, OUTPUT);
  pinMode(PIN_EPD_RST, OUTPUT);
  pinMode(PIN_EPD_DC, OUTPUT);
  
#ifdef DRIVER_WAVESHARE
  display.init(115200, partial ? false : true, 2, false);
  // remap spi for waveshare
  SPI.end();
  SPI.begin(PIN_EPD_SCK,
            PIN_EPD_MISO,
            PIN_EPD_MOSI,
            PIN_EPD_CS);
#endif
#ifdef DRIVER_DESPI_C02
  display.init(115200, partial ? false : true, 10, false);
#endif

  display.setRotation(0);
  display.setTextSize(1);
  display.setTextColor(GxEPD_BLACK);
  display.setTextWrap(false);
  // display.fillScreen(GxEPD_WHITE);
  if ( partial )
  {
    Serial.println("Partial Refresh");
    display.setPartialWindow(0, 0, 40, 40);
  }
  else
  {
    display.setFullWindow();
  }
    
  display.firstPage(); // use paged drawing mode, sets fillScreen(GxEPD_WHITE)
  return;
} // end initDisplay

/*
 * powerOffDisplay
 *
 * Power-off e-paper display
 */
void powerOffDisplay()
{
  display.hibernate(); // turns powerOff() and sets controller to deep sleep for
                       // minimum power use
  digitalWrite(PIN_EPD_PWR, LOW);
  return;
} // end powerOffDisplay

/*
 * drawWebIcon
 *
 * This function is responsible for drawing / erasing the web access icon
 * (erasing is performed in partial mode)
 */
#ifdef WEB_SVR
void drawWebIcon ( int drawIcon )
{
  if ( drawIcon == 1 )
    display.drawInvertedBitmap(0, 0, web_icon_40x40, 40, 40, GxEPD_BLACK);
  else if ( drawIcon == 2 )
    display.drawInvertedBitmap(0, 0, ap_icon_40x40, 40, 40, GxEPD_BLACK);
  else
  {
    // Just clear the Web icon using partial refresh
    initDisplay(1);
    do
    {
      display.fillRect(0, 0, 40, 40, GxEPD_WHITE);
    }
    while(display.nextPage());

    // Poweroff before entering in deep sleep mode soon
    powerOffDisplay();
  }
}
#endif // WEB_SVR

/*
 * drawCurrentCondition
 *
 * draw two raws of icons and indications on lower left side
 */
void drawCurrentConditions(const owm_current_t &current,
                           const owm_daily_t &today,
                           const owm_resp_air_pollution_t &owm_air_pollution,
                           float inTemp, float inHumidity,
                           int tz_off   // AUTO_TZ
                          )
{
  String dataStr, unitStr;

  // current weather icon
  display.drawInvertedBitmap(0, 0,
                             getCurrentConditionsBitmap196(current, today),
                             196, 196, GxEPD_BLACK);

  // current temp
#ifdef UNITS_TEMP_KELVIN
  dataStr = String(static_cast<int>(std::round(current.temp)));
  unitStr = TXT_UNITS_TEMP_KELVIN;
#endif
#ifdef UNITS_TEMP_CELSIUS
  dataStr = String(static_cast<int>(
            std::round(kelvin_to_celsius(current.temp))));
  unitStr = TXT_UNITS_TEMP_CELSIUS;
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
  dataStr = String(static_cast<int>(
            std::round(kelvin_to_fahrenheit(current.temp))));
  unitStr = TXT_UNITS_TEMP_FAHRENHEIT;
#endif
  // FONT_**_temperature fonts only have the character set used for displaying
  // temperature (0123456789.-\260)
  display.setFont(&FONT_48pt8b_temperature);
#ifndef DISP_BW_V1
    //drawString(196 + 164 / 2 - 20, 196 / 2 + 69 / 2, dataStr, CENTER);
    drawString(196 + 164 / 2 - 20, 196 / 2 + 69 / 2 + 10, dataStr, CENTER);
#elif defined(DISP_BW_V1)
    drawString(156 + 164 / 2 - 20, 196 / 2 + 69 / 2, dataStr, CENTER);
#endif
  display.setFont(&FONT_14pt8b);
  //drawString(display.getCursorX(), 196 / 2 - 69 / 2 + 20, unitStr, LEFT);
  drawString(display.getCursorX(), 196 / 2 - 69 / 2 + 20 + 10, unitStr, LEFT);

  // current feels like
#ifdef UNITS_TEMP_KELVIN
  dataStr = String(TXT_FEELS_LIKE) + ' '
            + String(static_cast<int>(std::round(current.feels_like)));
#endif
#ifdef UNITS_TEMP_CELSIUS
  dataStr = String(TXT_FEELS_LIKE) + ' '
            + String(static_cast<int>(std::round(
                     kelvin_to_celsius(current.feels_like))))
            + '\260';
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
  dataStr = String(TXT_FEELS_LIKE) + ' '
            + String(static_cast<int>(std::round(
                     kelvin_to_fahrenheit(current.feels_like))))
            + '\260';
#endif
  display.setFont(&FONT_12pt8b);
#ifndef DISP_BW_V1
  //drawString(196 + 164 / 2, 98 + 69 / 2 + 12 + 17, dataStr, CENTER);
  drawString(196 + 164 / 2, 98 + 69 / 2 + 12 + 17+20, dataStr, CENTER);
#elif defined(DISP_BW_V1)
  drawString(156 + 164 / 2, 98 + 69 / 2 + 12 + 17, dataStr, CENTER);
#endif
  // line dividing top and bottom display areas
  // display.drawLine(0, 196, DISP_WIDTH - 1, 196, GxEPD_BLACK);

  // current weather data icons
  display.drawInvertedBitmap(0, 204 + (48 + 8) * 0,
                             wi_sunrise_48x48, 48, 48, GxEPD_BLACK);
  display.drawInvertedBitmap(0, 204 + (48 + 8) * 1,
                             wi_strong_wind_48x48, 48, 48, GxEPD_BLACK);
  display.drawInvertedBitmap(0, 204 + (48 + 8) * 2,
                             wi_day_sunny_48x48, 48, 48, GxEPD_BLACK);
#ifndef DISP_BW_V1
  display.drawInvertedBitmap(0, 204 + (48 + 8) * 3,
                             air_filter_48x48, 48, 48, GxEPD_BLACK);
  display.drawInvertedBitmap(0, 204 + (48 + 8) * 4,
                             house_thermometer_48x48, 48, 48, GxEPD_BLACK);
#endif
  // 170->155
  display.drawInvertedBitmap(155, 204 + (48 + 8) * 0,
                             wi_sunset_48x48, 48, 48, GxEPD_BLACK);
  display.drawInvertedBitmap(155, 204 + (48 + 8) * 1,
                             wi_humidity_48x48, 48, 48, GxEPD_BLACK);
  display.drawInvertedBitmap(155, 204 + (48 + 8) * 2,
                             wi_barometer_48x48, 48, 48, GxEPD_BLACK);
#ifndef DISP_BW_V1
  display.drawInvertedBitmap(155, 204 + (48 + 8) * 3,
                             visibility_icon_48x48, 48, 48, GxEPD_BLACK);
  display.drawInvertedBitmap(155, 204 + (48 + 8) * 4,
                             house_humidity_48x48, 48, 48, GxEPD_BLACK);
#endif

  // current weather data labels
  /* INCR_POL */
  display.setFont(&FONT_8pt8b);
  drawString(48, 204 + 8 + (48 + 8) * 0, TXT_SUNRISE, LEFT);
  drawString(48, 204 + 8 + (48 + 8) * 1, TXT_WIND, LEFT);
  drawString(48, 204 + 8 + (48 + 8) * 2, TXT_UV_INDEX, LEFT);
#ifndef DISP_BW_V1
  const char *air_quality_index_label;
  if (aqi_desc_type(AQI_SCALE) == AIR_QUALITY_DESC)
  {
    air_quality_index_label = TXT_AIR_QUALITY;
  }
  else // (aqi_desc_type(AQI_SCALE) == AIR_POLLUTION_DESC)
  {
    air_quality_index_label = TXT_AIR_POLLUTION;
  }
  drawString(48, 204 + 8 + (48 + 8) * 3, air_quality_index_label, LEFT);
  drawString(48, 204 + 8 + (48 + 8) * 4, TXT_INDOOR_TEMPERATURE, LEFT);
#endif
  drawString(155 + 48, 204 + 8 + (48 + 8) * 0, TXT_SUNSET, LEFT);
  drawString(155 + 48, 204 + 8 + (48 + 8) * 1, TXT_HUMIDITY, LEFT);
  drawString(155 + 48, 204 + 8 + (48 + 8) * 2, TXT_PRESSURE, LEFT);
#ifndef DISP_BW_V1
  drawString(155 + 48, 204 + 8 + (48 + 8) * 3, TXT_VISIBILITY, LEFT);
  drawString(155 + 48, 204 + 8 + (48 + 8) * 4, TXT_INDOOR_HUMIDITY, LEFT);
#endif
  /* INCR_POL */

  // sunrise
  display.setFont(&FONT_12pt8b);
  char timeBuffer[12] = {}; // big enough to accommodate "hh:mm:ss am"
  time_t ts = current.sunrise + tz_off;  // AUTO_TZ
  tm *timeInfo = localtime(&ts);
  _strftime(timeBuffer, sizeof(timeBuffer), TIME_FORMAT, timeInfo);
  drawString(48, 204 + 17 / 2 + (48 + 8) * 0 + 48 / 2, timeBuffer, LEFT);

  // wind
#ifdef WIND_INDICATOR_ARROW
  display.drawInvertedBitmap(48, 204 + 24 / 2 + (48 + 8) * 1,
                             getWindBitmap24(current.wind_deg),
                             24, 24, GxEPD_BLACK);
#endif
#ifdef UNITS_SPEED_METERSPERSECOND
  dataStr = String(static_cast<int>(std::round(current.wind_speed)));
  unitStr = TXT_UNITS_SPEED_METERSPERSECOND;
#endif
#ifdef UNITS_SPEED_FEETPERSECOND
  dataStr = String(static_cast<int>(std::round(
                   meterspersecond_to_feetpersecond(current.wind_speed) )));
  unitStr = TXT_UNITS_SPEED_FEETPERSECOND;
#endif
#ifdef UNITS_SPEED_KILOMETERSPERHOUR
  dataStr = String(static_cast<int>(std::round(
                   meterspersecond_to_kilometersperhour(current.wind_speed) )));
  unitStr = TXT_UNITS_SPEED_KILOMETERSPERHOUR;
#endif
#ifdef UNITS_SPEED_MILESPERHOUR
  dataStr = String(static_cast<int>(std::round(
                   meterspersecond_to_milesperhour(current.wind_speed) )));
  unitStr = TXT_UNITS_SPEED_MILESPERHOUR;
#endif
#ifdef UNITS_SPEED_KNOTS
  dataStr = String(static_cast<int>(std::round(
                   meterspersecond_to_knots(current.wind_speed) )));
  unitStr = TXT_UNITS_SPEED_KNOTS;
#endif
#ifdef UNITS_SPEED_BEAUFORT
  dataStr = String(meterspersecond_to_beaufort(current.wind_speed));
  unitStr = TXT_UNITS_SPEED_BEAUFORT;
#endif

#ifdef WIND_INDICATOR_ARROW
  drawString(48 + 24, 204 + 17 / 2 + (48 + 8) * 1 + 48 / 2, dataStr, LEFT);
#else
  drawString(48     , 204 + 17 / 2 + (48 + 8) * 1 + 48 / 2, dataStr, LEFT);
#endif
  display.setFont(&FONT_8pt8b);
  drawString(display.getCursorX(), 204 + 17 / 2 + (48 + 8) * 1 + 48 / 2,
             unitStr, LEFT);

#if defined(WIND_INDICATOR_NUMBER)
  dataStr = String(current.wind_deg) + "\260";
  display.setFont(&FONT_12pt8b);
  drawString(display.getCursorX() + 6, 204 + 17 / 2 + (48 + 8) * 1 + 48 / 2,
             dataStr, LEFT);
#endif
#if defined(WIND_INDICATOR_CPN_CARDINAL)                \
 || defined(WIND_INDICATOR_CPN_INTERCARDINAL)           \
 || defined(WIND_INDICATOR_CPN_SECONDARY_INTERCARDINAL) \
 || defined(WIND_INDICATOR_CPN_TERTIARY_INTERCARDINAL)
  dataStr = getCompassPointNotation(current.wind_deg);
  display.setFont(&FONT_12pt8b);
  drawString(display.getCursorX() + 6, 204 + 17 / 2 + (48 + 8) * 1 + 48 / 2,
             dataStr, LEFT);
#endif

  // uv and air quality indices
  // spacing between end of index value and start of descriptor text
  const int sp = 8;

  // uv index
  display.setFont(&FONT_12pt8b);
  uint uvi = static_cast<uint>(std::max(std::round(current.uvi), 0.0f));
  dataStr = String(uvi);
  drawString(48, 204 + 17 / 2 + (48 + 8) * 2 + 48 / 2, dataStr, LEFT);
  display.setFont(&FONT_7pt8b);
  dataStr = String(getUVIdesc(uvi));
  // 170 -> 155
  int max_w = 155 - (display.getCursorX() + sp);
  if (getStringWidth(dataStr) <= max_w)
  { // Fits on a single line, draw along bottom
    drawString(display.getCursorX() + sp, 204 + 17 / 2 + (48 + 8) * 2 + 48 / 2,
               dataStr, LEFT);
  }
  else
  { // use smaller font
    display.setFont(&FONT_5pt8b);
    if (getStringWidth(dataStr) <= max_w)
    { // Fits on a single line with smaller font, draw along bottom
      drawString(display.getCursorX() + sp,
                 204 + 17 / 2 + (48 + 8) * 2 + 48 / 2,
                 dataStr, LEFT);
    }
    else
    { // Does not fit on a single line, draw higher to allow room for 2nd line
      drawMultiLnString(display.getCursorX() + sp,
                        204 + 17 / 2 + (48 + 8) * 2 + 48 / 2 - 10,
                        dataStr, LEFT, max_w, 2, 10);
    }
  }

#ifndef DISP_BW_V1
  // air quality index
  display.setFont(&FONT_12pt8b);
  const owm_components_t &c = owm_air_pollution.components;
  // OpenWeatherMap does not provide pb (lead) conentrations, so we pass NULL.
  int aqi = calc_aqi(AQI_SCALE, c.co, c.nh3, c.no, c.no2, c.o3, NULL, c.so2,
                                c.pm10, c.pm2_5);
  int aqi_max = aqi_scale_max(AQI_SCALE);
  if (aqi > aqi_max)
  {
    dataStr = "> " + String(aqi_max);
  }
  else
  {
    dataStr = String(aqi);
  }
  drawString(48, 204 + 17 / 2 + (48 + 8) * 3 + 48 / 2, dataStr, LEFT);
  display.setFont(&FONT_7pt8b);
  dataStr = String(aqi_desc(AQI_SCALE, aqi));
  // 170 -> 155
  max_w = 155 - (display.getCursorX() + sp);
  if (getStringWidth(dataStr) <= max_w)
  { // Fits on a single line, draw along bottom
    drawString(display.getCursorX() + sp, 204 + 17 / 2 + (48 + 8) * 3 + 48 / 2,
               dataStr, LEFT);
  }
  else
  { // use smaller font
    display.setFont(&FONT_5pt8b);
    if (getStringWidth(dataStr) <= max_w)
    { // Fits on a single line with smaller font, draw along bottom
      drawString(display.getCursorX() + sp,
                 204 + 17 / 2 + (48 + 8) * 3 + 48 / 2,
                 dataStr, LEFT);
    }
    else
    { // Does not fit on a single line, draw higher to allow room for 2nd line
      drawMultiLnString(display.getCursorX() + sp,
                        204 + 17 / 2 + (48 + 8) * 3 + 48 / 2 - 10,
                        dataStr, LEFT, max_w, 2, 10);
    }
  }

  // indoor temperature
  display.setFont(&FONT_12pt8b);
  if (!std::isnan(inTemp))
  {
#ifdef BEFORE_TEMP_DECIMAL
#ifdef UNITS_TEMP_KELVIN
    dataStr = String(static_cast<int>(std::round(celsius_to_kelvin(inTemp))));
#endif
#ifdef UNITS_TEMP_CELSIUS
    dataStr = String(static_cast<int>(std::round(inTemp)));
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
    dataStr = String(static_cast<int>(
              std::round(celsius_to_fahrenheit(inTemp))));
#endif
#else // BEFORE_TEMP_DECIMAL
    float curTemp;
#ifdef UNITS_TEMP_KELVIN
    curTemp = celsius_to_kelvin(inTemp);
#endif
#ifdef UNITS_TEMP_CELSIUS
    curTemp = inTemp;
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
    curTemp = celsius_to_fahrenheit(inTemp);
#endif
    dataStr = String(static_cast<int>(std::round(curTemp))) + "." +
              String(static_cast<int>(std::round((curTemp)*10))%10);
#endif // BEFORE_TEMP_DECIMAL
  }
  else
  {
    dataStr = "--";
  }
#if defined(UNITS_TEMP_CELSIUS) || defined(UNITS_TEMP_FAHRENHEIT)
  dataStr += "\260";
#endif
  drawString(48, 204 + 17 / 2 + (48 + 8) * 4 + 48 / 2, dataStr, LEFT);
#endif // defined(DISP_BW_V2) || defined(DISP_3C_B) || defined(DISP_7C_F)

  // sunset
  memset(timeBuffer, '\0', sizeof(timeBuffer));
  ts = current.sunset + tz_off; // AUTO_TZ
  timeInfo = localtime(&ts);
  _strftime(timeBuffer, sizeof(timeBuffer), TIME_FORMAT, timeInfo);
  // 170 -> 155
  drawString(155 + 48, 204 + 17 / 2 + (48 + 8) * 0 + 48 / 2, timeBuffer, LEFT);

  // humidity
  dataStr = String(current.humidity);
  drawString(155 + 48, 204 + 17 / 2 + (48 + 8) * 1 + 48 / 2, dataStr, LEFT);
  display.setFont(&FONT_8pt8b);
  drawString(display.getCursorX(), 204 + 17 / 2 + (48 + 8) * 1 + 48 / 2,
             "%", LEFT);

  // pressure
#ifdef UNITS_PRES_HECTOPASCALS
  dataStr = String(current.pressure);
  unitStr = TXT_UNITS_PRES_HECTOPASCALS;
#endif
#ifdef UNITS_PRES_PASCALS
  dataStr = String(static_cast<int>(std::round(
                   hectopascals_to_pascals(current.pressure) )));
  unitStr = TXT_UNITS_PRES_PASCALS;
#endif
#ifdef UNITS_PRES_MILLIMETERSOFMERCURY
  dataStr = String(static_cast<int>(std::round(
                   hectopascals_to_millimetersofmercury(current.pressure) )));
  unitStr = TXT_UNITS_PRES_MILLIMETERSOFMERCURY;
#endif
#ifdef UNITS_PRES_INCHESOFMERCURY
  dataStr = String(std::round(1e1f *
                   hectopascals_to_inchesofmercury(current.pressure)
                   ) / 1e1f, 1);
  unitStr = TXT_UNITS_PRES_INCHESOFMERCURY;
#endif
#ifdef UNITS_PRES_MILLIBARS
  dataStr = String(static_cast<int>(std::round(
                   hectopascals_to_millibars(current.pressure) )));
  unitStr = TXT_UNITS_PRES_MILLIBARS;
#endif
#ifdef UNITS_PRES_ATMOSPHERES
  dataStr = String(std::round(1e3f *
                   hectopascals_to_atmospheres(current.pressure) )
                   / 1e3f, 3);
  unitStr = TXT_UNITS_PRES_ATMOSPHERES;
#endif
#ifdef UNITS_PRES_GRAMSPERSQUARECENTIMETER
  dataStr = String(static_cast<int>(std::round(
                   hectopascals_to_gramspersquarecentimeter(current.pressure)
                   )));
  unitStr = TXT_UNITS_PRES_GRAMSPERSQUARECENTIMETER;
#endif
#ifdef UNITS_PRES_POUNDSPERSQUAREINCH
  dataStr = String(std::round(1e2f *
                   hectopascals_to_poundspersquareinch(current.pressure)
                   ) / 1e2f, 2);
  unitStr = TXT_UNITS_PRES_POUNDSPERSQUAREINCH;
#endif
  display.setFont(&FONT_12pt8b);
  drawString(155 + 48, 204 + 17 / 2 + (48 + 8) * 2 + 48 / 2, dataStr, LEFT);
  display.setFont(&FONT_8pt8b);
  drawString(display.getCursorX(), 204 + 17 / 2 + (48 + 8) * 2 + 48 / 2,
             unitStr, LEFT);

#ifndef DISP_BW_V1
  // visibility
  display.setFont(&FONT_12pt8b);
#ifdef UNITS_DIST_KILOMETERS
  float vis = meters_to_kilometers(current.visibility);
  unitStr = TXT_UNITS_DIST_KILOMETERS;
#endif
#ifdef UNITS_DIST_MILES
  float vis = meters_to_miles(current.visibility);
  unitStr = TXT_UNITS_DIST_MILES;
#endif
  // if visibility is less than 1.95, round to 1 decimal place
  // else round to int
  if (vis < 1.95)
  {
    dataStr = String(std::round(10 * vis) / 10.0, 1);
  }
  else
  {
    dataStr = String(static_cast<int>(std::round(vis)));
  }
#ifdef UNITS_DIST_KILOMETERS
  if (vis >= 10)
#endif
#ifdef UNITS_DIST_MILES
  if (vis >= 6)
#endif
  {
    dataStr = "> " + dataStr;
  }
  drawString(155 + 48, 204 + 17 / 2 + (48 + 8) * 3 + 48 / 2, dataStr, LEFT);
  display.setFont(&FONT_8pt8b);
  drawString(display.getCursorX(), 204 + 17 / 2 + (48 + 8) * 3 + 48 / 2,
             unitStr, LEFT);

  // indoor humidity
  display.setFont(&FONT_12pt8b);
  if (!std::isnan(inHumidity))
  {
    dataStr = String(static_cast<int>(std::round(inHumidity)));
  }
  else
  {
    dataStr = "--";
  }
  drawString(155 + 48, 204 + 17 / 2 + (48 + 8) * 4 + 48 / 2, dataStr, LEFT);
  display.setFont(&FONT_8pt8b);
  drawString(display.getCursorX(), 204 + 17 / 2 + (48 + 8) * 4 + 48 / 2,
             "%", LEFT);
#endif // defined(DISP_BW_V2) || defined(DISP_3C_B) || defined(DISP_7C_F)
  return;
} // end drawCurrentConditions

/*
 * drawForecast
 *
 * This function is responsible for drawing the five day forecast.
 */
void drawForecast(owm_daily_t *const daily, tm timeInfo)
{
  // 5 day, forecast
  String hiStr, loStr;
  String dataStr, unitStr;
#ifdef POP_AND_VOL
  String dataStr2, unitStr2;
#endif
  for (int i = 0; i < 5; ++i)
  {
#ifndef DISP_BW_V1
    int x = 398 + (i * 82);
#elif defined(DISP_BW_V1)
    int x = 318 + (i * 64);
#endif
    // icons
    display.drawInvertedBitmap(x, 98 + 69 / 2 - 32 - 6,
                               getForecastBitmap64(daily[i]),
                               64, 64, GxEPD_BLACK);
    // day of week label
    display.setFont(&FONT_11pt8b);
    char dayBuffer[8] = {};
    _strftime(dayBuffer, sizeof(dayBuffer), "%a", &timeInfo); // abbrv'd day
    drawString(x + 31 - 2, 98 + 69 / 2 - 32 - 26 - 6 + 16, dayBuffer, CENTER);
    timeInfo.tm_wday = (timeInfo.tm_wday + 1) % 7; // increment to next day

    // high | low
#ifdef TEMP_FONT_ENH
    display.setFont(&FONT_10pt8b);
#else
    display.setFont(&FONT_8pt8b);
#endif
    drawString(x + 31, 98 + 69 / 2 + 38 - 6 + 12, "|", CENTER);
#ifdef UNITS_TEMP_KELVIN
    hiStr = String(static_cast<int>(std::round(daily[i].temp.max)));
    loStr = String(static_cast<int>(std::round(daily[i].temp.min)));
#endif
#ifdef UNITS_TEMP_CELSIUS
    hiStr = String(static_cast<int>(
                std::round(kelvin_to_celsius(daily[i].temp.max)))) +
            "\260";
    loStr = String(static_cast<int>(
                std::round(kelvin_to_celsius(daily[i].temp.min)))) +
            "\260";
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
    hiStr = String(static_cast<int>(
                std::round(kelvin_to_fahrenheit(daily[i].temp.max)))) +
            "\260";
    loStr = String(static_cast<int>(
                std::round(kelvin_to_fahrenheit(daily[i].temp.min)))) +
            "\260";
#endif
    drawString(x + 31 - 4, 98 + 69 / 2 + 38 - 6 + 12, hiStr, RIGHT);
    drawString(x + 31 + 5, 98 + 69 / 2 + 38 - 6 + 12, loStr, LEFT);

// daily forecast precipitation
#if DISPLAY_DAILY_PRECIP
    float dailyPrecip;
#if defined(UNITS_DAILY_PRECIP_POP)
    dailyPrecip = daily[i].pop * 100;
    dataStr = String(static_cast<int>(dailyPrecip));
    unitStr = "%";
#ifdef POP_AND_VOL
    dailyPrecip = daily[i].snow + daily[i].rain;
#if defined(POP_VOL_MM)
    // Round up to nearest mm
    dailyPrecip = std::round(dailyPrecip);
    dataStr2 = String(static_cast<int>(dailyPrecip));
    unitStr2 = "mm";
#elif defined(POP_VOL_CM)
    // Round up to nearest 0.1 cm
    dailyPrecip = millimeters_to_centimeters(dailyPrecip);
    dailyPrecip = std::round(dailyPrecip * 10) / 10.0f;
    dataStr2 = String(dailyPrecip, 1);
    unitStr2 = "cm";
#elif defined(POP_VOL_INCHES)
    // Round up to nearest 0.1 inch
    dailyPrecip = millimeters_to_inches(dailyPrecip);
    dailyPrecip = std::round(dailyPrecip * 10) / 10.0f;
    dataStr2 = String(dailyPrecip, 1);
    unitStr2 = "in";
#endif
#endif // POP_AND_VOL
#else
    dailyPrecip = daily[i].snow + daily[i].rain;
#if defined(UNITS_DAILY_PRECIP_MILLIMETERS)
    // Round up to nearest mm
    dailyPrecip = std::round(dailyPrecip);
    dataStr = String(static_cast<int>(dailyPrecip));
    unitStr = "mm";
#elif defined(UNITS_DAILY_PRECIP_CENTIMETERS)
    // Round up to nearest 0.1 cm
    dailyPrecip = millimeters_to_centimeters(dailyPrecip);
    dailyPrecip = std::round(dailyPrecip * 10) / 10.0f;
    dataStr = String(dailyPrecip, 1);
    unitStr = "cm";
#elif defined(UNITS_DAILY_PRECIP_INCHES)
    // Round up to nearest 0.1 inch
    dailyPrecip = millimeters_to_inches(dailyPrecip);
    dailyPrecip = std::round(dailyPrecip * 10) / 10.0f;
    dataStr = String(dailyPrecip, 1);
    unitStr = "in";
#endif
#endif
#if (DISPLAY_DAILY_PRECIP == 2) // smart
      if (dailyPrecip > 0.0f)
#endif
      {
#ifdef TEMP_FONT_ENH
        display.setFont(&FONT_8pt8b);
#define YADJ 28
#else
        display.setFont(&FONT_6pt8b);
#define YADJ 26
#endif
#ifdef POP_AND_VOL
        drawString(x + 31, 98 + 69 / 2 + 38 - 6 + YADJ, dataStr + unitStr , CENTER);

        display.setFont(&FONT_6pt8b);
        drawString(x + 31, 98 + 69 / 2 + 38 - 6 + YADJ + 12, "(" + dataStr2 + unitStr2 + ")", CENTER);
#else
        drawString(x + 31, 98 + 69 / 2 + 38 - 6 + YADJ, dataStr + unitStr, CENTER);
#endif
      }
#endif // DISPLAY_DAILY_PRECIP
    }

    return;
} // end drawForecast

/*
 * drawAlerts
 *
 * This function is responsible for drawing the current alerts if any.
 * Up to 2 alerts can be drawn.
 */
void drawAlerts(std::vector<owm_alerts_t> &alerts,
                const String &city, const String &date)
{
#if DEBUG_LEVEL >= 1
  Serial.println("[debug] alerts.size()    : " + String(alerts.size()));
#endif
  if (alerts.size() == 0)
  { // no alerts to draw
    return;
  }

  int *ignore_list = (int *) calloc(alerts.size(), sizeof(*ignore_list));
  int *alert_indices = (int *) calloc(alerts.size(), sizeof(*alert_indices));
  if (!ignore_list || !alert_indices)
  {
    Serial.println("Error: Failed to allocate memory while handling alerts.");
    free(ignore_list);
    free(alert_indices);
    return;
  }

  // Converts all event text and tags to lowercase, removes extra information,
  // and filters out redundant alerts of lesser urgency.
  filterAlerts(alerts, ignore_list);

  // limit alert text width so that is does not run into the location or date
  // strings
  display.setFont(&FONT_16pt8b);
  int city_w = getStringWidth(city);
  display.setFont(&FONT_12pt8b);
  int date_w = getStringWidth(date);
  int max_w = DISP_WIDTH - 2 - std::max(city_w, date_w) - (196 + 4) - 8;

  // find indices of valid alerts
  int num_valid_alerts = 0;
#if DEBUG_LEVEL >= 1
  Serial.print("[debug] ignore_list      : [ ");
#endif
  for (int i = 0; i < alerts.size(); ++i)
  {
#if DEBUG_LEVEL >= 1
    Serial.print(String(ignore_list[i]) + " ");
#endif
    if (!ignore_list[i])
    {
      alert_indices[num_valid_alerts] = i;
      ++num_valid_alerts;
    }
  }
#if DEBUG_LEVEL >= 1
  Serial.println("]\n[debug] num_valid_alerts : " + String(num_valid_alerts));
#endif

  if (num_valid_alerts == 1)
  { // 1 alert
    // adjust max width to for 48x48 icons
    max_w -= 48;

    owm_alerts_t &cur_alert = alerts[alert_indices[0]];
    display.drawInvertedBitmap(196, 8, getAlertBitmap48(cur_alert), 48, 48,
                               ACCENT_COLOR);
    // must be called after getAlertBitmap
    toTitleCase(cur_alert.event);

    //display.setFont(&FONT_14pt8b);
    display.setFont(&FONT_12pt8b);
    if (getStringWidth(cur_alert.event) <= max_w)
    { // Fits on a single line, draw along bottom
      drawString(196 + 48 + 4, 24 + 8 - 12 + 20 + 1, cur_alert.event, LEFT);
    }
    else
    { // use smaller font
      //display.setFont(&FONT_12pt8b);
      display.setFont(&FONT_10pt8b);
      if (getStringWidth(cur_alert.event) <= max_w)
      { // Fits on a single line with smaller font, draw along bottom
        drawString(196 + 48 + 4, 24 + 8 - 12 + 17 + 1, cur_alert.event, LEFT);
      }
      else
      { // Does not fit on a single line, draw higher to allow room for 2nd line
        drawMultiLnString(196 + 48 + 4, 24 + 8 - 12 + 17 - 11,
                          cur_alert.event, LEFT, max_w, 2, 23);
      }
    }
  } // end 1 alert
  else
  { // 2 alerts
    // adjust max width to for 32x32 icons
    max_w -= 32;

    //display.setFont(&FONT_12pt8b);
    display.setFont(&FONT_10pt8b);
    for (int i = 0; i < 2; ++i)
    {
      owm_alerts_t &cur_alert = alerts[alert_indices[i]];

      display.drawInvertedBitmap(196, (i * 32), getAlertBitmap32(cur_alert),
                                 32, 32, ACCENT_COLOR);
      // must be called after getAlertBitmap
      toTitleCase(cur_alert.event);

      drawMultiLnString(196 + 32 + 3, 5 + 17 + (i * 32),
                        cur_alert.event, LEFT, max_w, 1, 0);
    } // end for-loop
  } // end 2 alerts

  free(ignore_list);
  free(alert_indices);

  return;
} // end drawAlerts

/*
 * drawLocationDate
 *
 * This function is responsible for drawing the city string and date
 * information in the top right corner.
 */
void drawLocationDate(const String &city, const String &date)
{
  // location, date
  display.setFont(&FONT_16pt8b);
  drawString(DISP_WIDTH - 2, 23, city, RIGHT, ACCENT_COLOR);
  display.setFont(&FONT_12pt8b);
  drawString(DISP_WIDTH - 2, 30 + 4 + 17, date, RIGHT);
  return;
} // end drawLocationDate

/*
 * modulo
 *
 * The % operator in C++ is not a true modulo operator but it instead a
 * remainder operator. The remainder operator and modulo operator are equivalent
 * for positive numbers, but not for negatives. The follow implementation of the
 * modulo operator works for +/-a and +b.
 */
inline int modulo(int a, int b)
{
  const int result = a % b;
  return result >= 0 ? result : result + b;
}

/*
 * drawOutlookGraph
 *
 * This function is responsible for drawing the outlook graph for the specified
 * number of hours(up to 47).
 */
void drawOutlookGraph(owm_hourly_t *const hourly, int tz_off) // AUTO_TZ
{
 /* AUTO_POP_CONTRAST */
  int pop_step = 2;

#ifdef UNITS_HOURLY_PRECIP_POP
  float precipValTot = hourly[0].pop * 100;
#else
  float precipValTot = hourly[0].rain_1h + hourly[0].snow_1h;
#endif
  float precipValTotMax;
 /* AUTO_POP_CONTRAST */

  // 350 -> 340
  const int xPos0 = 340;
#ifdef XXX_STD
  int xPos1 = DISP_WIDTH - 23; // may be moved to make room for decimal places
#endif
#ifdef UNITS_HOURLY_PRECIP_CENTIMETERS
  //const int xPos1 = DISP_WIDTH - 62; // A little extra room for more decimals
  int xPos1 = DISP_WIDTH - 62; // A little extra room for more decimals
#else
  //const int xPos1 = DISP_WIDTH - 56;
  int xPos1 = DISP_WIDTH - 56;
#endif
  const int yPos0 = 216;
  const int yPos1 = DISP_HEIGHT - 46;

  // calculate y max/min and intervals
  int yMajorTicks = 5;
#ifdef UNITS_TEMP_KELVIN
  float tempMin = hourly[0].temp;
#endif
#ifdef UNITS_TEMP_CELSIUS
  float tempMin = kelvin_to_celsius(hourly[0].temp);
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
  float tempMin = kelvin_to_fahrenheit(hourly[0].temp);
#endif
  float tempMax = tempMin;
#ifdef UNITS_HOURLY_PRECIP_POP
  float precipMax = hourly[0].pop;
#else
  float precipMax = hourly[0].rain_1h + hourly[0].snow_1h;
#endif
#ifdef POP_AND_VOL
  float precipMax2 = hourly[0].rain_1h + hourly[0].snow_1h;
#endif
  int yTempMajorTicks = 5;
  float newTemp = 0;

  /*
   * This loop computes the extreme values for temperature and precipitaions
   */
  for (int i = 1; i < HOURLY_GRAPH_MAX; ++i)
  {
#ifdef UNITS_TEMP_KELVIN
    newTemp = hourly[i].temp;
#endif
#ifdef UNITS_TEMP_CELSIUS
    newTemp = kelvin_to_celsius(hourly[i].temp);
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
    newTemp = kelvin_to_fahrenheit(hourly[i].temp);
#endif
    tempMin = std::min(tempMin, newTemp);
    tempMax = std::max(tempMax, newTemp);
#ifdef UNITS_HOURLY_PRECIP_POP
    precipMax = std::max<float>(precipMax, hourly[i].pop);
#else
    precipMax = std::max<float>(
                precipMax, hourly[i].rain_1h + hourly[i].snow_1h);
#endif
#ifdef POP_AND_VOL
    precipMax2 = std::max<float>(precipMax2, hourly[i].rain_1h + hourly[i].snow_1h);
#endif
    /* AUTO_POP_CONTRAST */
#ifdef UNITS_HOURLY_PRECIP_POP
    precipValTot += hourly[i].pop * 100;
#else
    precipMax = std::max<float>(
                precipMax, hourly[i].rain_1h + hourly[i].snow_1h);
#endif
    /* AUTO_POP_CONTRAST */
  }

  int tempBoundMin = static_cast<int>(tempMin - 1)
                      - modulo(static_cast<int>(tempMin - 1), yTempMajorTicks);
  int tempBoundMax = static_cast<int>(tempMax + 1)
   + (yTempMajorTicks - modulo(static_cast<int>(tempMax + 1), yTempMajorTicks));

  // while we have to many major ticks then increase the step
  while ((tempBoundMax - tempBoundMin) / yTempMajorTicks > yMajorTicks)
  {
    yTempMajorTicks += 5;
    tempBoundMin = static_cast<int>(tempMin - 1)
                      - modulo(static_cast<int>(tempMin - 1), yTempMajorTicks);
    tempBoundMax = static_cast<int>(tempMax + 1) + (yTempMajorTicks
                      - modulo(static_cast<int>(tempMax + 1), yTempMajorTicks));
  }
  // while we have not enough major ticks add to either bound
  while ((tempBoundMax - tempBoundMin) / yTempMajorTicks < yMajorTicks)
  {
    // add to whatever bound is closer to the actual min/max
    if (tempMin - tempBoundMin <= tempBoundMax - tempMax)
    {
      tempBoundMin -= yTempMajorTicks;
    }
    else
    {
      tempBoundMax += yTempMajorTicks;
    }
  }

#ifdef UNITS_HOURLY_PRECIP_POP
  float precipBoundMax;
  if (precipMax > 0)
  {
    precipBoundMax = 100.0f;
  }
  else
  {
    precipBoundMax = 0.0f;
  }

#ifdef POP_AND_VOL
#ifdef POP_VOL_MM
  float precipBoundMax2 = std::ceil(precipMax2); // Round up to nearest mm
  int yPrecipMajorTickDecimals2 = (precipBoundMax2 < 10) ? 1 : 0;
#endif
#ifdef POP_VOL_CM
  precipMax2 = millimeters_to_centimeters(precipMax2);
  // Round up to nearest 0.1 cm
  float precipBoundMax2 = std::ceil(precipMax2 * 10) / 10.0f;
  int yPrecipMajorTickDecimals2 = ((precipBoundMax2 < 1) ? 2 :
                                  ((precipBoundMax2 < 10) ? 1 : 0));
#endif
#ifdef POP_VOL_INCHES
  precipMax2 = millimeters_to_inches(precipMax2);
  // Round up to nearest 0.1 inch
  float precipBoundMax2 = std::ceil(precipMax2 * 10) / 10.0f;
  int yPrecipMajorTickDecimals2 = ((precipBoundMax2 < 1) ? 2 :
                                  ((precipBoundMax2 < 10) ? 1 : 0));
#endif
  float yPrecipMajorTickValue2 = precipBoundMax2 / yMajorTicks;
  float precipRoundingMultiplier2 = std::pow(10.f, yPrecipMajorTickDecimals2);
#endif // POP_AND_VOL

#else
#ifdef UNITS_HOURLY_PRECIP_MILLIMETERS
  float precipBoundMax = std::ceil(precipMax); // Round up to nearest mm
  int yPrecipMajorTickDecimals = precipBoundMax < 10 ? 1 : 0;
#endif
#ifdef UNITS_HOURLY_PRECIP_CENTIMETERS
  precipMax = millimeters_to_centimeters(precipMax);
  // Round up to nearest 0.1 cm
  float precipBoundMax = std::ceil(precipMax * 10) / 10.0f;
  int yPrecipMajorTickDecimals;
  if (precipBoundMax < 1)
  {
    yPrecipMajorTickDecimals = 2;
  }
  else if (precipBoundMax < 10)
  {
    yPrecipMajorTickDecimals = 1;
  }
  else
  {
    yPrecipMajorTickDecimals = 0;
  }
#endif

#ifdef UNITS_HOURLY_PRECIP_INCHES
  precipMax = millimeters_to_inches(precipMax);
  // Round up to nearest 0.1 inch
  float precipBoundMax = std::ceil(precipMax * 10) / 10.0f;
  int yPrecipMajorTickDecimals;
  if (precipBoundMax < 1)
  {
    yPrecipMajorTickDecimals = 2;
  }
  else if (precipBoundMax < 10)
  {
    yPrecipMajorTickDecimals = 1;
  }
  else
  {
    yPrecipMajorTickDecimals = 0;
  }
#endif
  float yPrecipMajorTickValue = precipBoundMax / yMajorTicks;
  float precipRoundingMultiplier = std::pow(10.f, yPrecipMajorTickDecimals);
#endif

#ifdef BEFORE_EXT_GRAPH
#else
  if ( !precipBoundMax )
    // extend graph on right margin when no precipitations
    xPos1 += 30;
#endif

  /* AUTO_POP_CONTRAST */
  precipValTotMax = precipBoundMax * HOURLY_GRAPH_MAX;
  //Serial.printf("POP: ValToT=%f, ValMax=%f, BoundMax=%f\n", precipValTot, precipValTotMax, precipBoundMax);
  if ( precipValTotMax )
    Serial.print("Precipitation graph fill ratio = " + String(precipValTot/precipValTotMax));
  else
    Serial.print("Precipitation graph fill ratio = 0 ");

#ifdef WEB_SVR
  Serial.println(" Threshold = " + String(PopTh));
  if ( precipValTot > (precipValTotMax * PopTh) )
    pop_step = 3;
#else
  Serial.println(" Threshold = " + String(PRECIP_THRESHOLD));
  if ( precipValTot > (precipValTotMax * PRECIP_THRESHOLD) )
    pop_step = 3;
#endif // WEB_SVR

  Serial.println("Precipitation graph step = " + String(pop_step));
  /* AUTO_POP_CONTRAST */

  /*
   * Draw x axis
   */
  display.drawLine(xPos0, yPos1    , xPos1, yPos1    , GxEPD_BLACK);
  display.drawLine(xPos0, yPos1 - 1, xPos1, yPos1 - 1, GxEPD_BLACK);

  /*
   * Draw y axis
   */
  float yInterval = (yPos1 - yPos0) / static_cast<float>(yMajorTicks);
  for (int i = 0; i <= yMajorTicks; ++i)
  {
    /*
     * Display temperature scale on left
     */
    String dataStr;
    int yTick = static_cast<int>(yPos0 + (i * yInterval));
    display.setFont(&FONT_8pt8b);
    // Temperature
    dataStr = String(tempBoundMax - (i * yTempMajorTicks));
#if defined(UNITS_TEMP_CELSIUS) || defined(UNITS_TEMP_FAHRENHEIT)
    dataStr += "\260";
#endif
#ifdef BEFORE_EXT_GRAPH
    drawString(xPos0 - 8, yTick + 4, dataStr, RIGHT, ACCENT_COLOR);
#else
    drawString(xPos0 - 12, yTick + 4, dataStr, RIGHT, ACCENT_COLOR);
#endif

#ifdef POP_AND_VOL
    String precipUnit2, dataStr2;
#endif

    /*
     * Display precipitation scales on right
     * (only when some precipitations)
     */
    if (precipBoundMax > 0)
    { // don't labels if precip is 0
#ifdef UNITS_HOURLY_PRECIP_POP
      // PoP
      dataStr = String(100 - (i * 20));
      String precipUnit = "%";
#ifdef POP_AND_VOL
      if ( precipBoundMax2 )
      {
        float precipTick2 = precipBoundMax2 - (i * yPrecipMajorTickValue2);
        precipTick2 = std::round(precipTick2 * precipRoundingMultiplier2)
                                / precipRoundingMultiplier2;
        dataStr2 = "("+String(precipTick2, yPrecipMajorTickDecimals2);

#ifdef POP_VOL_MM
        precipUnit2 = "mm";
#endif
#ifdef POP_VOL_CM
        precipUnit2 = "cm";
#endif
#ifdef POP_VOL_INCHES
        precipUnit2 = "in";
#endif
      }
#endif // POP_AND_VOL
#else // UNITS_HOURLY_PRECIP_POP
      // Precipitation volume
      float precipTick = precipBoundMax - (i * yPrecipMajorTickValue);
      precipTick = std::round(precipTick * precipRoundingMultiplier)
                              / precipRoundingMultiplier;
      dataStr = String(precipTick, yPrecipMajorTickDecimals);
#ifdef UNITS_HOURLY_PRECIP_MILLIMETERS
      String precipUnit = "mm";
#endif
#ifdef UNITS_HOURLY_PRECIP_CENTIMETERS
      String precipUnit = "cm";
#endif
#ifdef UNITS_HOURLY_PRECIP_INCHES
      String precipUnit = "in";
#endif
#endif // UNITS_HOURLY_PRECIP_POP

      drawString(xPos1 + 8, yTick + 4, dataStr, LEFT);
      display.setFont(&FONT_5pt8b);
      drawString(display.getCursorX(), yTick + 4, precipUnit, LEFT);
    } // end draw labels if precip is >0

    /*
     * Draw horizontal dotted line
     */
    if (i < yMajorTicks)
    {
      for (int x = xPos0; x <= xPos1 + 1; x += 3)
      {
        display.drawPixel(x, yTick + (yTick % 2), GxEPD_BLACK);
      }
    }
#ifdef POP_AND_VOL
    if ( (i < yMajorTicks) && precipBoundMax && precipBoundMax2 )
    {
      display.setFont(&FONT_7pt8b);
      drawString(xPos1 + 4, yTick + 15 + 4, dataStr2, LEFT);
      display.setFont(&FONT_5pt8b);
      drawString(display.getCursorX(), yTick + 15 + 4, precipUnit2, LEFT);
      display.setFont(&FONT_7pt8b);
      drawString(display.getCursorX(), yTick + 15 + 4, ")", LEFT);
    }
#endif // POP_AND_VOL
  }

#ifdef POP_AND_VOL
  int last_y0_t = yPos1;
#endif
  int xMaxTicks = 8;
  int hourInterval = static_cast<int>(ceil(HOURLY_GRAPH_MAX
                                           / static_cast<float>(xMaxTicks)));
  float xInterval = (xPos1 - xPos0 - 1) / static_cast<float>(HOURLY_GRAPH_MAX);
  display.setFont(&FONT_8pt8b);
#ifdef BEFORE_RECENTER_GRAPH
  for (int i = 0; i < HOURLY_GRAPH_MAX; ++i)
#else
  for (int i = 0; i <= HOURLY_GRAPH_MAX; ++i)
#endif
  {
    int xTick = static_cast<int>(xPos0 + (i * xInterval));
    int x0_t, x1_t, y0_t, y1_t;
    float yPxPerUnit;

    /*
     * Compute temperature bounds
     */
    yPxPerUnit = (yPos1 - yPos0) / static_cast<float>(tempBoundMax - tempBoundMin);
#ifdef BEFORE_RECENTER_GRAPH
    x0_t = static_cast<int>(std::round(xPos0 + ((i - 1) * xInterval) + (0.5 * xInterval) ));
    x1_t = static_cast<int>(std::round(xPos0 + (i * xInterval) + (0.5 * xInterval) ));
#else
    x0_t = static_cast<int>(std::round(xPos0 + ((i - 1) * xInterval) ));
    x1_t = static_cast<int>(std::round(xPos0 + (i * xInterval) ));
#endif

#ifdef UNITS_TEMP_KELVIN
    if (i > 0)
      y0_t = static_cast<int>(std::round(yPos1 - (yPxPerUnit * ((hourly[i - 1].temp) - tempBoundMin)) ));
    y1_t = static_cast<int>(std::round(yPos1 - (yPxPerUnit * ((hourly[i].temp) - tempBoundMin)) ));
#endif
#ifdef UNITS_TEMP_CELSIUS
    if (i > 0)
      y0_t = static_cast<int>(std::round(yPos1 - (yPxPerUnit * (kelvin_to_celsius(hourly[i - 1].temp) - tempBoundMin)) ));
    y1_t = static_cast<int>(std::round(yPos1 - (yPxPerUnit * (kelvin_to_celsius(hourly[i].temp) - tempBoundMin)) ));
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
    if (i > 0)
      y0_t = static_cast<int>(std::round(yPos1 - (yPxPerUnit * (kelvin_to_fahrenheit(hourly[i - 1].temp) - tempBoundMin)) ));
    y1_t = static_cast<int>(std::round( yPos1 - (yPxPerUnit * (kelvin_to_fahrenheit(hourly[i].temp) - tempBoundMin)) ));
#endif

    if (i > 0)
    {
      /*
       * Graph temperature line
       */
      display.drawLine(x0_t    , y0_t    , x1_t    , y1_t    , ACCENT_COLOR);
      display.drawLine(x0_t    , y0_t + 1, x1_t    , y1_t + 1, ACCENT_COLOR);
      display.drawLine(x0_t - 1, y0_t    , x1_t - 1, y1_t    , ACCENT_COLOR);
    }

    /*
     * If fonction enabled, draw hourly bitmap (DISPLAY_HOURLY_ICONS)
     */
#ifdef BEFORE_RECENTER_GRAPH
    // at every graph mark, skipping last tick
    if ( WicFlg && (i != HOURLY_GRAPH_MAX) && !(i % hourInterval) )
#else
    // at every graph mark, skipping last tick only when some prepicitations 
    if ( WicFlg &&
         ((i != HOURLY_GRAPH_MAX) || (!precipBoundMax && !precipBoundMax2)) &&
         !(i % hourInterval) )
#endif
    {
      const uint8_t *bitmap = getForecastBitmap32(hourly[i]);
#ifdef BEFORE_RECENTER_GRAPH
#else
      x0_t += (0.5 * xInterval);
      x1_t += (0.5 * xInterval);
#endif
      if ( WicTemp )
        // Display Icon along the temperature line
        display.drawInvertedBitmap(x0_t, y1_t - 42, bitmap, 32, 32, GxEPD_BLACK);
      else
        // Display Icon near upper limit of the graph
        display.drawInvertedBitmap(x0_t, yPos0, bitmap, 32, 32, GxEPD_BLACK);
    }

#ifdef UNITS_HOURLY_PRECIP_POP
    float precipVal = hourly[i].pop * 100;
#ifdef POP_AND_VOL
    float precipVal2 = hourly[i].rain_1h + hourly[i].snow_1h;
#ifdef POP_VOL_CM
    precipVal2 = millimeters_to_centimeters(precipVal2);
#endif
#ifdef POP_VOL_INCHES
    precipVal2 = millimeters_to_inches(precipVal2);
#endif
#endif // POP_AND_VOL
#else
    float precipVal = hourly[i].rain_1h + hourly[i].snow_1h;
#ifdef UNITS_HOURLY_PRECIP_CENTIMETERS
    precipVal = millimeters_to_centimeters(precipVal);
#endif
#ifdef UNITS_HOURLY_PRECIP_INCHES
    precipVal = millimeters_to_inches(precipVal);
#endif
#endif

    x0_t = static_cast<int>(std::round( xPos0 + 1 + (i * xInterval)));
    x1_t = static_cast<int>(std::round( xPos0 + 1 + ((i + 1) * xInterval) ));
    yPxPerUnit = (yPos1 - yPos0) / precipBoundMax;
    y0_t = static_cast<int>(std::round( yPos1 - (yPxPerUnit * (precipVal)) ));
    y1_t = yPos1;

    /*
     * Graph Precipitation as a greyed area
     * Contribution of Dwuhls (solution to issue #62 on L Marzen Github)
     */
    /* AUTO_POP_CONTRAST */
#ifdef BEFORE_RECENTER_GRAPH
#else
    if ( i != HOURLY_GRAPH_MAX )
#endif
    {
      for (int y = y1_t - 1; y > y0_t; y -= pop_step)
      {
        for (int x = x0_t; x < x1_t; x += 1)
        {
          if ((x % pop_step) == 0)
          {
            display.drawPixel(x, y, GxEPD_BLACK);
          }
        }
      }
      /* AUTO_POP_CONTRAST */

#ifdef POP_AND_VOL
      /*
       * Display precipitation volume as a line
       */
      if ( precipBoundMax2 )
      {
        if ( precipVal )
        {
          //Serial.printf("VOL: Val[%d]=%f, BoundMax=%f\n", i, precipVal2, precipBoundMax2);
          yPxPerUnit = (yPos1 - yPos0) / precipBoundMax2;
          y0_t = static_cast<int>(std::round( yPos1 - (yPxPerUnit * (precipVal2)) ));
          display.drawLine(x0_t    , last_y0_t, x0_t    , y0_t    , ACCENT_COLOR);   // Vertical left
          display.drawLine(x0_t    , y0_t     , x1_t    , y0_t    , ACCENT_COLOR);   // Horizontal
#ifdef POP_THICK_LINE
          display.drawLine(x0_t-1  , last_y0_t, x0_t-1  , y0_t    , ACCENT_COLOR);   // Vertical left (2nd)
          if ( y0_t != yPos1 )
            display.drawLine(x0_t    , y0_t+1   , x1_t    , y0_t+1  , ACCENT_COLOR); // Horizontal (2nd)
#endif
          if ( (i+1) == HOURLY_GRAPH_MAX )
          {
            display.drawLine(x1_t  , y0_t     , x1_t    , yPos1   , ACCENT_COLOR);   // Vertical right
#ifdef POP_THICK_LINE
            display.drawLine(x1_t-1, y0_t     , x1_t-1  , yPos1   , ACCENT_COLOR);   // Vertical right (2nd)
#endif
          }
          last_y0_t = y0_t;
        }
        else if (last_y0_t != yPos1 )
        {
          // draw right line of previous bar
          display.drawLine(x0_t    , last_y0_t, x0_t    , yPos1   , ACCENT_COLOR);   // Vertical right
#ifdef POP_THICK_LINE
          display.drawLine(x0_t-1  , last_y0_t, x0_t-1  , yPos1   , ACCENT_COLOR);   // Vertical right (2nd)
#endif
          last_y0_t = yPos1;
        }
      }
#endif // POP_AND_VOL
    }

    if ((i % hourInterval) == 0)
    {
      // draw x tick marks
      display.drawLine(xTick    , yPos1 + 1, xTick    , yPos1 + 4, GxEPD_BLACK);
      display.drawLine(xTick + 1, yPos1 + 1, xTick + 1, yPos1 + 4, GxEPD_BLACK);
      // draw x axis labels
      char timeBuffer[12] = {}; // big enough to accommodate "hh:mm:ss am"
      time_t ts = hourly[i].dt + tz_off; // AUTO_TZ
      tm *timeInfo = localtime(&ts);
      _strftime(timeBuffer, sizeof(timeBuffer), HOUR_FORMAT, timeInfo);
      drawString(xTick, yPos1 + 1 + 12 + 4 + 3, timeBuffer, CENTER);
    }
  }

  // draw the last tick mark
  if ((HOURLY_GRAPH_MAX % hourInterval) == 0)
  {
    int xTick = static_cast<int>(
                std::round(xPos0 + (HOURLY_GRAPH_MAX * xInterval)));
    // draw x tick marks
    display.drawLine(xTick    , yPos1 + 1, xTick    , yPos1 + 4, GxEPD_BLACK);
    display.drawLine(xTick + 1, yPos1 + 1, xTick + 1, yPos1 + 4, GxEPD_BLACK);
    // draw x axis labels
    char timeBuffer[12] = {}; // big enough to accommodate "hh:mm:ss am"
    time_t ts = hourly[HOURLY_GRAPH_MAX - 1].dt + 3600 + tz_off; // AUTO_TZ
    tm *timeInfo = localtime(&ts);
    _strftime(timeBuffer, sizeof(timeBuffer), HOUR_FORMAT, timeInfo);
    drawString(xTick, yPos1 + 1 + 12 + 4 + 3, timeBuffer, CENTER);
  }

  return;
} // end drawOutlookGraph

/* This function is responsible for drawing the status bar along the bottom of
 * the display.
 */
void drawStatusBar(const String &statusStr, const String &refreshTimeStr,
                   int rssi, uint32_t batVoltage)
{
  String dataStr;
  uint16_t dataColor = GxEPD_BLACK;
  display.setFont(&FONT_6pt8b);
  int pos = DISP_WIDTH - 2;
  const int sp = 2;

#if BATTERY_MONITORING
  // battery
  uint32_t batPercent = calcBatPercent(batVoltage,
                                       CRIT_LOW_BATTERY_VOLTAGE,
                                       MAX_BATTERY_VOLTAGE);
#if defined(DISP_3C_B) || defined(DISP_7C_F)
  if (batVoltage < WARN_BATTERY_VOLTAGE)
  {
    dataColor = ACCENT_COLOR;
  }
#endif
  dataStr = String(batPercent) + "%";
#if STATUS_BAR_EXTRAS_BAT_VOLTAGE
  dataStr += " (" + String( std::round(batVoltage / 10.f) / 100.f, 2 ) + "v)";
#endif
  drawString(pos, DISP_HEIGHT - 1 - 2, dataStr, RIGHT, dataColor);
  pos -= getStringWidth(dataStr) + 25;
  display.drawInvertedBitmap(pos, DISP_HEIGHT - 1 - 17,
                             getBatBitmap24(batPercent), 24, 24, dataColor);
  pos -= sp + 9;
#endif

  // WiFi
  dataStr = String(getWiFidesc(rssi));
  dataColor = rssi >= -70 ? GxEPD_BLACK : ACCENT_COLOR;
#if STATUS_BAR_EXTRAS_WIFI_RSSI
  if (rssi != 0)
  {
    dataStr += " (" + String(rssi) + "dBm)";
  }
#endif
  drawString(pos, DISP_HEIGHT - 1 - 2, dataStr, RIGHT, dataColor);
  pos -= getStringWidth(dataStr) + 19;
  display.drawInvertedBitmap(pos, DISP_HEIGHT - 1 - 13, getWiFiBitmap16(rssi),
                             16, 16, dataColor);
  pos -= sp + 8;

  // last refresh
  dataColor = GxEPD_BLACK;
  drawString(pos, DISP_HEIGHT - 1 - 2, refreshTimeStr, RIGHT, dataColor);
  pos -= getStringWidth(refreshTimeStr) + 25;
  display.drawInvertedBitmap(pos, DISP_HEIGHT - 1 - 21, wi_refresh_32x32,
                             32, 32, dataColor);
  pos -= sp;

  // status
  dataColor = ACCENT_COLOR;
  if (!statusStr.isEmpty())
  {
    drawString(pos, DISP_HEIGHT - 1 - 2, statusStr, RIGHT, dataColor);
    pos -= getStringWidth(statusStr) + 24;
    display.drawInvertedBitmap(pos, DISP_HEIGHT - 1 - 18, error_icon_24x24,
                               24, 24, dataColor);
  }

  return;
} // end drawStatusBar

/* This function is responsible for drawing prominent error messages to the
 * screen.
 *
 * If error message line 2 (errMsgLn2) is empty, line 1 will be automatically
 * wrapped.
 */
void drawError(const uint8_t *bitmap_196x196,
               const String &errMsgLn1, const String &errMsgLn2)
{
  display.setFont(&FONT_26pt8b);
  if (!errMsgLn2.isEmpty())
  {
    drawString(DISP_WIDTH / 2,
               DISP_HEIGHT / 2 + 196 / 2 + 21,
               errMsgLn1, CENTER);
    drawString(DISP_WIDTH / 2,
               DISP_HEIGHT / 2 + 196 / 2 + 21 + 55,
               errMsgLn2, CENTER);
  }
  else
  {
    drawMultiLnString(DISP_WIDTH / 2,
                      DISP_HEIGHT / 2 + 196 / 2 + 21,
                      errMsgLn1, CENTER, DISP_WIDTH - 200, 2, 55);
  }
  display.drawInvertedBitmap(DISP_WIDTH / 2 - 196 / 2,
                             DISP_HEIGHT / 2 - 196 / 2 - 21,
                             bitmap_196x196, 196, 196, ACCENT_COLOR);
  return;
} // end drawError

