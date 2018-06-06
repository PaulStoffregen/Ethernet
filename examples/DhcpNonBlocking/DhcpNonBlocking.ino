/*
  This sketch uses the DHCP extensions to the Ethernet library
  to get an IP address via DHCP and print the address obtained.
  using a Wiznet Ethernet shield.

  The non-blocking begin() and maintain() functions are used. This is demonstrated by the LED blinking on pin 13
  During initial DHCP transaction : a short 50ms pulse every 500ms.
  During normal operation : regular blinking (500ms ON - 500ms OFF)
  During DHCP renewal : fast blinking (50ms ON - 50ms OFF)
  DHCP fail : a short 50ms pulse every 2s.

  created 26 may 2017 by Tom Magnier
 */

#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};

typedef enum {
	INIT,
  NORMAL,
  RENEWAL,
  FAIL
} state_t;

state_t state = INIT;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  // this check is only needed on the Leonardo:
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(LED_BUILTIN, OUTPUT);

  SPI.setSCK(14); //TODO TEMP

  // start the Ethernet connection:
  Serial.println("Initializing Ethernet controller and starting DHCP request.");
  Ethernet.initialize(mac, 10000); //10s timeout
}

void loop() {
  /*Serial.print("state : ");
  Serial.println(state); */
  int result;

  switch(state) {
    case INIT:
      blinkLed(50, 500);
      result = Ethernet.initialized();
      if (result == 1)
      {
        Serial.println("DHCP successful.");
        printIPAddress();
        state = NORMAL;
      }
      else if (result == 2)
      {
        Serial.println("Failed to configure Ethernet using DHCP");
        state = FAIL;
      }
      break;

    case NORMAL:
      blinkLed(500, 1000);
      if (Ethernet.maintainNeeded() > 0)
      {
        Serial.println("DHCP lease expired, requesting a new one.");
        state = RENEWAL;
      }
      break;

    case RENEWAL:
      blinkLed(50, 100);
      result = Ethernet.maintainFinished();
      if (result == 1)
      {
        Serial.println("DHCP renewal successful.");
        printIPAddress();
        state = NORMAL;
      }
      else if (result == 2)
      {
        Serial.println("DHCP renewal failed.");
        state = FAIL;
      }
      break;

    case FAIL:
      blinkLed(50, 2000);
      break;
  }
}

void blinkLed(unsigned int onTime, unsigned int period)
{
  static unsigned long lastPeriodStart = 0;
  if (millis() - lastPeriodStart > period)
  {
    lastPeriodStart = millis();
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else if (millis() - lastPeriodStart > onTime)
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void printIPAddress() {
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
}
