# ntpnixie
Simple NTP clock using ESP32 for GRA &amp; AFCH Nixie Arduino HAT NCS314-6 v3.4

Wanted a simple application to use an ESP32 UNO style board on an GRA and AFCH Nixie clock Arduino hat (NCS314-6 v3.4) available on ebay and NTP discipline it…

eBay listing said clock had a DS3231SN RTC but mine came with a RV3028... I could not get any of the existing RV3028 arduino libraries to function for whatever reason so I am talking to it directly with the wire library

additionally said LEDs were PWM controlled but clock came with addressable LEDs so using the Adafruit NeoPixel library to just turn them off (I have no interest in using them, but the library is there if you want to do something with it)

obviously YMMV on hardware

Libraries required:

NTPClient

https://github.com/taranais/NTPClient

Adafruit-NeoPixel

https://github.com/adafruit/Adafruit_NeoPixel/

Please note I made this for my clock and it does what I wanted it to do, I have no idea what I am doing, this code is entirely as-is... it's enitrely possible I am actually a cat pretending to be a human, you will never know
