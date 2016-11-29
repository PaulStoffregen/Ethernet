#Ethernet Library for Teensy#

Wiznet chips W5100, W5200 and W5500 are automatically detected.

Efficient block mode of W5200 & W5500 are used for data and multi-byte register access.

Wiznet socket registers are cached, which greatly reduces SPI communication overhead, at the cost of a small increase in RAM usage.  TCP connections and UDP packets accessed in small chunks typically see a substantial performance boost.

Adafruit's Ethernet.init(CSPIN) extension is supported, to allow use of different pins for the chip select signal.  Optimized direct register I/O is used for most Arduino compatible boards.

http://www.pjrc.com/teensy/td_libs_Ethernet.html

![](http://www.pjrc.com/store/wiz820_assem5.jpg)

