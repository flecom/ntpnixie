# ntpnixie
Simple NTP clock using ESP32 for GRA &amp; AFCH Nixie Arduino HAT NCS314-6 v3.4

Wanted a simple application to use an ESP32 UNO style board on an GRA and AFCH Nixie clock Arduino hat (NCS314-6 v3.4) available on ebay and NTP discipline it…

eBay listing said clock had a DS3231SN RTC but mine came with a RV3028

additionally said LEDs were PWM controlled but clock came with addressable LEDs

so obviously YMMV on hardware

Libraries required:

NTPClient

https://github.com/taranais/NTPClient

Adafruit-NeoPixel

https://github.com/adafruit/Adafruit_NeoPixel/

Please note I made this for my clock and it does what I wanted it to do, I have no idea what I am doing, this code is entirely as-is... it's enitrely possible I am actually a cat pretending to be a human, you will never know
