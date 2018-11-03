# Improved Performance Ethernet Library

Wiznet chips W5100, W5200 and W5500 are automatically detected.

Efficient block mode of W5200 & W5500 are used for data and multi-byte register access.

Wiznet socket registers are cached, which greatly reduces SPI communication overhead, at the cost of a small increase in RAM usage.  TCP connections and UDP packets accessed in small chunks typically see a substantial performance boost.

Adafruit's Ethernet.init(CSPIN) extension is supported, to allow use of different pins for the chip select signal.  Optimized direct register I/O is used for most Arduino compatible boards.

http://www.pjrc.com/teensy/td_libs_Ethernet.html

![](http://www.pjrc.com/store/wiz820_assem5.jpg)

### Tested Hardware


| Board			| Chip	| Shield / Module	| Internet	| Local (LAN)	|
| ---------------------	| :---: | :-------------------: | ------------: | ------------: |
| Teensy 3.6		| W5500	| WIZ850io		| 212.59	| 1143.58	|
| Teensy 3.6		| W5200	| WIZ820io		| 202.44	| 1102.71	|
| Teensy 3.6		| W5100	| WIZ812MJ		| 180.76	| 274.14	|
| ESP32 Feather		| W5500 | Featherwing Ethernet	| 211.06	| 965.76	|
| Teensy 3.2		| W5500	| WIZ850io		| 205.37	| 958.06	|
| Teensy 3.2		| W5200	| WIZ820io		| 215.44	| 914.78	|
| Teensy 3.2		| W5100	| WIZ812MJ		| 170.07	| 234.55	|
| ChipKit Uno32		| W5500 | Seeed Ethernet W5500	| 177.19	| 858.81	|
| ChipKit Uno32		| W5200 | WIZ820io		| 188.31	| 837.56	|
| ChipKit Uno32		| W5100 | Ethernet R2 (clone)	| 159.72	| 272.18	|
| Arduino Due		| W5500 | Seeed Ethernet	| 214.44	| 689.69	|
| Arduino Due		| W5200 | WIZ820io		| 206.51	| 670.88	|
| Arduino Due		| W5100 | Arduino Ethernet R3	| 105.98	| 109.73	|
| ESP8266 Feather	| W5500 | Featherwing Ethernet	| fail (dns)	| 583.31	|
| Teensy LC		| W5500	| WIZ850io		| 200.51	| 479.73	|
| Teensy LC		| W5200	| WIZ820io		| 199.62	| 471.95	|
| Teensy LC		| W5100	| WIZ812MJ		| 126.40	| 137.77	|
| Arduino 101 (Intel)	| W5500 | Seeed Ethernet W5500	| 168.96	| 359.32	|
| Arduino 101 (Intel)	| W5200 | WIZ820io		| 169.37	| 349.35	|
| Arduino 101 (Intel)	| W5100 | Arduino Ethernet R3	| 42.39		| 43.60		|
| Teensy 2.0		| W5100	| WIZ812MJ		| 81.07		| 84.85		|
| Arduino Uno R3	| W5500 | Seeed Ethernet W5500	| 185.44	| 329.00	|
| Arduino Uno R3	| W5500 | Arduino.org Ethernet2	| 195.32	| 329.60	|
| Arduino Uno R3	| W5200 | WIZ820io		| 191.85	| 331.27	|
| Arduino Uno R3	| W5100 | Arduino Ethernet R3	| 79.11		| 82.66		|
| Arduino Uno R3	| W5100 | Ethernet R2 (clone)	| 79.27		| 82.66		|
| Arduino Leonardo	| W5500 | Seeed Ethernet W5500	| 179.98	| 328.14	|
| Arduino Leonardo	| W5200 | WIZ820io		| 183.69	| 330.30	|
| Arduino Leonardo	| W5100 | Ethernet R2 (clone)	| 78.75		| 82.28		|
| Mega 2560 (clone)	| W5500 | Seeed Ethernet W5500	| 179.58	| 323.36	|
| Mega 2560 (clone)	| W5200 | WIZ820io		| 172.73	| 325.44	|
| Mega 2560 (clone)	| W5100 | Ethernet R2 (clone)	| 74.31	 	| 77.44		|
| Arduino Zero		| W5500 | Seeed Ethernet W5500	| 183.13	| 305.26	|
| Arduino Zero		| W5500 | Arduino.org Ethernet2	| 181.60	| 305.28	|
| Arduino Zero		| W5200 | WIZ820io		| 177.53	| 298.53	|
| Arduino Zero		| W5100 | Arduino Ethernet R3	| 91.33		| 96.64		|
| Arduino Zero		| W5100 | Ethernet R2 (clone)	| 91.42		| 96.64		|
| Arduino MKR1000	| W5500	| MKR ETH		| 181.27	| 298.93	|
| Arduino MKR1000	| W5200	| WIZ820io		| 125.20	| 291.98	|
| Arduino Uno Wifi Rev2	| W5500	| Seeed Ethernet W5500	| 163.86	| 213.36	|
| Arduino Uno Wifi Rev2	| W5500	| Arduino.org Ethernet2	| 169.72	| 212.88	|
| Arduino Uno Wifi Rev2	| W5200	| WIZ820io		| 161.94	| 212.19	|
| Arduino Uno Wifi Rev2	| W5100	| Arduino.org Ethernet2	| 69.50		| 72.23		|
| Arduino Uno Wifi Rev2	| W5100	| Ethernet R2 (clone)	| 69.55		| 72.30		|

For more information about these benchmarks:
https://www.pjrc.com/arduino-ethernet-library-2-0-0/

ESP32 may require edit to SPI.h to add SPI.transfer(data, size)
https://github.com/espressif/arduino-esp32/issues/1623
