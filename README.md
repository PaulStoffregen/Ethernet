#Ethernet Library for Teensy#

Wiznet chips W5100, W5200 and W5500 are automatically detected.  Efficient block transfer modes of W5200 and W5500 are utilized.

Chip select must be connected to pin 10.  Reset for W5200 must be connected to pin 9.

Wiznet socket status registers are cached, which greatly reduces SPI communication overhead, at the cost of a small increase in RAM usage.  TCP connections and UDP packets accessed in small chunks typically see a substantial performance boost.

http://www.pjrc.com/teensy/td_libs_Ethernet.html

![](http://www.pjrc.com/store/wiz820_assem5.jpg)

