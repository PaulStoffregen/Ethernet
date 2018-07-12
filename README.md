# Improved Performance Ethernet Library

Wiznet chips W5100, W5200 and W5500 are automatically detected.

Efficient block mode of W5200 & W5500 are used for data and multi-byte register access.

Wiznet socket registers are cached, which greatly reduces SPI communication overhead, at the cost of a small increase in RAM usage.  TCP connections and UDP packets accessed in small chunks typically see a substantial performance boost.

Adafruit's Ethernet.init(CSPIN) extension is supported, to allow use of different pins for the chip select signal.  Optimized direct register I/O is used for most Arduino compatible boards.

http://www.pjrc.com/teensy/td_libs_Ethernet.html

![](http://www.pjrc.com/store/wiz820_assem5.jpg)

### Tested Hardware


| Board			| Chip	| Shield / Module	| Internet	| Local		|
| ---------------------	| :---: | :-------------------: | ------------: | ------------: |
| Teensy 3.6		| W5500	| WIZ850io		| 		| 1142.40	|
| Teensy 3.6		| W5200	| WIZ820io		| 		| 1100.50	|
| Teensy 3.6		| W5100	| WIZ811MJ		| 		| 		|
| Teensy 3.2		| W5500	| WIZ850io		| 		| 955.60	|
| Teensy LC		| W5500	| WIZ850io		| 		| 479.97	|
| Teensy 2.0		| W5100	| WIZ811MJ		| 		| 		|
| Arduino MKR1000	| W5500	| MKR ETH		|		| 324.14	|
| Arduino Uno R3	| W5500 | Seeed Ethernet	| 		| 342.35	|
| Arduino Uno R3	| W5100 | Arduino Ethernet R3	| 		| 		|
| Arduino Uno R3	| W5100 | Ethernet R2 (clone)	| 		| 		|
| Arduino Due		| W5100 | Arduino Ethernet R3	| 		| 		|
| Arduino Due		| W5500 | Seeed Ethernet	| 		| 696.62	|
| Arduino Zero		| W5500 | Seeed Ethernet	| 		| 344.42	|
| Arduino 101		| W5500 | Seeed Ethernet	| 		| 359.62	|
| ChipKit Uno32		| W5500 | Seeed Ethernet	| 		| 860.66	|
| Mega 2560		| W5500 | Seeed Ethernet	| 		| 337.02	|
| Mega 2560		| W5100 | Ethernet R3 		| 		| 		|
| ESP8266 Feather	| W5500 | Featherwing Ethernet	|		| 608.68	|
| ESP32 Feather		| W5500 | Featherwing Ethernet	|		| 964.03	|

ESP8266 & ESP32 require edit to SPI.h to add SPI.transfer(data, size)
https://github.com/esp8266/Arduino/issues/2677
https://github.com/espressif/arduino-esp32/issues/1623
