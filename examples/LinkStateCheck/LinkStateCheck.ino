/*

Link State Check - dubpixel - 3.28.2018 - rev1

 This sketch tests the w5200 or w5500 for physical link state (cable connected). bit 5 of a given register shouls blah blah. The function getLINK 
 should return a uint8, and away we go. 

 requires 'dubpixel" fork of arduino ethernet lib

 

 based on " Telnet Client" by Tom Igoe 
===============================================================================//
 Telnet client

 This sketch connects to a a telnet server (http://www.google.com)
 using an Arduino Wiznet Ethernet shield.  You'll need a telnet server
 to test this with.
 Processing's ChatServer example (part of the network library) works well,
 running on port 10002. It can be found as part of the examples
 in the Processing application, available at
 http://processing.org/
=============================================================//


 */

#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:


byte ip[] = {10,10,10,102};
//IPaddress ip(10,10,10,102);
byte mac[] ={0x00, 0x66, 0x06, 0x6B, 0x84, 0x58};

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 23 is default for telnet;
// if you're using Processing's ChatServer, use port 10002):
EthernetClient client;

void setup() {
    //setup led PIN 
   pinMode(13, OUTPUT); 
  // start the Ethernet connection:
  Ethernet.begin(mac, ip);
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  
  while (!Serial) {
     // wait for serial port to connect. Needed for native USB port only
    
  }


  // give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.println("connecting...");
  bootSeqLED();

}
  

void loop() {

  Serial.print("IP = ");
  Serial.println(Ethernet.localIP());
  Serial.print("PHYS = ");
  Serial.println(Ethernet.getLINK(),BIN);
  
  uint8_t getLINK = Ethernet.getLINK();
  uint8_t masked = ( 0b00100000 & getLINK);

  Serial.print("MASK = ");
  Serial.println(masked,BIN);

  uint8_t shifted = masked >> 5;
  Serial.print("SHIFTED = ");
  Serial.println(shifted);

  
  delay(2000);
  
 
}




