# cat-clock

This is the C++ source code for the firmware in a simple Arduino-based clock.

An I2C LCD displays the time and date using custom characters to build double-sized digits, while a DS3231 does the actual clock-related stuff. One alarm out of the two on the RTC is used. There is also a pin configured as open-drain that pulses every couple of seconds when the clock is in the alarm state, which I've connected to the _next_ button of an MP3 player module (DFPlayer Mini).
