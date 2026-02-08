/* Simple iperf receive speed benchmark test

   Requires Wiznet-based Ethernet shield connected to SPI (pins 10,11,12,13)

   Open serial monitor.  When ready, run the iperf command on your PC
   to transmit test data.  Speed will be shown on your PC and in serial
   monitor.

   By default, SPI 14 MHz is used for compatibility with Wiznet W5100.  Edit
   {libraries}/Ethernet/src/utility/w5100.h for faster with W5500 and W5200.

   This example is in the public domain
*/

#include <Ethernet.h>

EthernetServer server(5001);
uint8_t rxbuf[4096];
static void getMACAddress(uint8_t* const mac);

void setup() {
  Serial.begin(115200);
  while (!Serial && (millis() < 4000)) ; // wait for serial monitor

  Serial.println("Simple iperf receive");
  uint8_t mac[6];
  getMACAddress(mac);
  if (!Ethernet.begin(mac)) {
    printf("Failed to start Ethernet\r\n");
    while (1) ; // stop here
  }
  Serial.println("Ready to receive, run this commmand on your computer:");
  Serial.print("  iperf -i 1 -c ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  EthernetClient client = server.available();
  if (client.connected()) {
    Serial.print("iperf test begin, from ");
    Serial.println(client.remoteIP());
    int sum = 0;
    elapsedMillis msec = 0;
    while (1) {
      int avail = client.available();
      if (avail < 0) break;
      if (avail == 0) {
        if (client.connected()) continue;
        Serial.println("host disconnected");
        break;
      }
      if (avail > (int)sizeof(rxbuf)) avail = sizeof(rxbuf);
      int num = client.read(rxbuf, avail);
      if (num > 0) {
        sum = sum + num;
        uint32_t ms = msec;
        if (ms > 1000) {
          Serial.print((float)sum * (float)(8.0 / 1000000.0));
          Serial.println(" Mbits/sec");
          msec -= 1000;
          sum = 0;
        }
      }
    }
    client.stop();
    Serial.println();
  }
}

// Gets the Teensy 4's MAC address and fills in the given array.
static void getMACAddress(uint8_t* const mac) {
#ifdef __IMXRT1062__
  const uint32_t m1 = HW_OCOTP_MAC1;
  const uint32_t m2 = HW_OCOTP_MAC0;
  mac[0] = static_cast<uint8_t>(m1 >>  8);
  mac[1] = static_cast<uint8_t>(m1 >>  0);
  mac[2] = static_cast<uint8_t>(m2 >> 24);
  mac[3] = static_cast<uint8_t>(m2 >> 16);
  mac[4] = static_cast<uint8_t>(m2 >>  8);
  mac[5] = static_cast<uint8_t>(m2 >>  0);
#else
  static uint8_t dummymac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  memcpy(mac, dummymac, 6);
#endif
}

