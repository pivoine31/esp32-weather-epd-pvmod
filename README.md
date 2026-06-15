L.Marzen ESP32 Weather EPD station software with additional features 

Based on https://github.com/lmarzen/esp32-weather-epd (source code retrieved on 24/03/2026) 

This repository contains an update of the Luke Marzen ESP32 Weather station software, adding some interesting features:
- Wi-Fi handling several networks / credentials
- Embedded Web server (for managing Wi-Fi credentials, Geographic locations for displaying weather, weather station general parameters)
- A feature named POP_AND_VOL that allows displaying simultaneously the probability of precipitations and the volume of precipitations (hourly and daily)
- [dwuhls contribution] Automatic switching of the precipitation display pattern to avoid the contrast problem (the threshold may be modified trough the Web parameters page). The contrast modification in itself was submitted by dwuhls on issue #62 "eink display loses contrast on days with high PoP"
- [asdf1qaz contribution] reported AQI indication malfunction; now fixed
- [asdf1qaz contribution] battery threshold (config.cpp) modified to increase lipo battery life
  (see https://oscarliang.com/wp-content/uploads/2017/02/Lipo-battery-guide-Voltage-vs-capacity-used-percentage.jpg for guidelines)
- [asdf1qaz contribution] battery voltage multiplier made customizable in config.h to permit adjustement after multimeter check  
- [domp27 contribution] display weather icons on the graph on a per hour basis; the feature is controlled through the Web parameters page - two possible vertical positions for the icons)
- [Stelian Hurghis (RedShuriken)] contribution for adding Romanian language
- Ability to upload software using one of the OTA modes (through Wifi)

In the modified code, most of the customization options are moved to config.h
The indications below give some tips on how to configure the new features
Anyway, config.h may be customized as in the L Marzen software, relying on the indications found in comments

MORE ON THE WEB SERVER SUPPORT

Using the Web server requires to identify a means for waking-up the station in web server mode: by default, this is triggered by hitting the internal button
(the URL https://www.printables.com/fr/model/929910-esp32-weather-base-remix-and-lmarzen-modified-soft provides a box remix compatible with the use of the internal FireBeetle button) 

Others alternatives are:
- using a touch pin connected to a metallic thing accessible from the outside (probably the best alternative when the internal button is not accessible)
- using a custom button other than the internal one (connected with a pullup resistor to the esp32)

Once the station is Wake-Up (displaying a "world" icon at the upper left), the Web server it acceeded :
+ using the mDns name : http://Weather32.local ("Weather32" may be changed through HNAME in config.h)
+ or if it fails, using the IP address : http://<station-ip>
however, in this case, it is necessary to detect the station address using a network scanner (or sometimes on the admin pages of the Internel box)

The first page displayed asks for a key. It defaults to 0000 (may be changed through WEBKEY in config.h)

Examples of provided Web pages are given in show.mod directory

If not interested by the Web server, there is a custom option in config.h to disable it (comment or #undef WEB_SVR).
Doing this, the "automatic pop switch", the "pop and vol" features and the "weather icons hourly" are still available

Concerning the Wifi credentials and the location :
- if Web server is not used, the location (DEFLAT / DEFLON / DEFCITY) and one credential (WIFI_SSI1 / WIFI_PWD1) needs to be defined (in the section "WEB SERVER NOT USED")
- if Web server is used, default values for the location (DEFLAT / DEFLON / DEFCITY) and the credential (WIFI_SSI1 / WIFI_PWD1) may be either left empty, or may be defined (in the section "WEB SERVER USED")

If any (wifi/location) value is defined, it is used as a default value: it is setup initially in first (wifi/location) entry and reinstated every time the parameters are reset

Once the Web server is started, a specific icon (World) is displayed in the upper left corner and the Web pages may be acceeded using the IP of the station as URL (HTTP, because HTTPS not supported for this purpose)

The Web server terminates itself after 3 min without activity (by default, see DEF_MAXACT_TIM value), when the button is pressed again, or through "exit" on Web pages

When no Wifi network is available, and the Web button is pressed, the weather station acts as an Access Point (SSID "ESP32-Meteo", password "Weather.32"); the Web server is acceeded in this case at address 192.168.4.1 in HTTP mode (or using the mDns name). The  "No Wifi" page is displayed, but with an icon in the upper left indicating Web access availability.
This permits in particular to enter the Wifi credentials initially

AUTOMATIC TIMEZONE

This software remix does no longer need tuning the TIMEZONE and using NTP for time synchronization since the OpenWeatherMap service responses provides both the time information and the time offset relative to GMT based on the selected geographic location (lat/lon)

OTA SUPPORT

The software may be uploaded using OTA (On The Air) in one of the following modes (requiring the Web server function) :
  - arduino mode : platformio upload the compiled software through Wifi
    to enable this mode, the station IP address (or mDns name) shall be declared appropriately in platform.ini (see comments in the [upload] section)
  - web page mode : a new page is added to select then upload the compiled software (usually firmware.bin under platformio\.pio\build\dfrobot_firebeetle2_esp32e)

NEW IN THIS VERSION (V6 15/06/2026)
(for a complete history, see history.xlsx)

+ Merge with the 24/03/26 L Marzen version
+ (from the L Marzen version) add ability to manage position and function of Left panel icons (see "WIDGET POSITIONS" section in config.h)
+ (from the L Marzen version) support BME680 temperature sensor (see SENSOR_BME680 in config.h)
+ In case of HTTP error, wake-up interval is shortened to 60 sec to permit a quick recovery (however, this is limited to 10 consecutives intervals)
 
HINTS
+ Entering USB download mode :
 When uploading using USB, to enter boot mode, press and release the reset button when "connecting ..." appears (need to be quick because the sequence does not last long)
+ Setting-up the download mode (USB or OTA arduino mode) is done through the [upload] section in platform.ini (see comments)
