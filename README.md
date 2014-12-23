GalileoWeatherReport
====================

Windows on Galileo project that displays current and forecasted weather on LED Matrix and posts readings to Xively.

API support included:
- BMP085
- Seeed Studio's Grove Temperature and Humidity Sensor (TH02-based)
- Posting data to Xively (aka Cosm)
- Retrieving data from Weather Underground

Requirements:
- Intel Galileo (Gen 1 or Gen 2)
- Arduino to handle writing to the LED matrix (optional but recommended - support for writing to a MAX7219-based LED matrix is included in the code, but not recently tested.
- Microsoft's Windows Image and Galileo SDK (Dec 2/14 version or later) - http://ms-iot.github.io/content/
- Weather Underground API key
- Xively (or Cosm) API key
- temperature sensor: support included for BMP05, SeeedStudio's TH02, or Arduino-connected sensor

Requires that an WeatherReport.ini file be placed in the executable's folder on the Galilo, like the following:

COSM_API_KEY = xxxxxxxxxxxxxxx
COSM_FEED_ID = nnnnn
COSM_TEMP_DATASTREAM = xxxxxxxxxxxxx
COSM_HUMIDITY_DATASTREAM = xxxxxxxxxxxxxx
WUNDERGROUND_API = xxxxxxxxxxxxxxxxx
WUNDERGROUND_CITY = YourTown
WUNDERGROUND_COUNTRY = YourCountry
