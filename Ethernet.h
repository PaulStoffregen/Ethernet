#ifndef ethernet_h
#define ethernet_h

#include <inttypes.h>
#include "IPAddress.h"
#include "EthernetClient.h"
#include "EthernetServer.h"
#include "Dhcp.h"

#define MAX_SOCK_NUM 4

class EthernetClass {
private:
	static IPAddress _dnsServerAddress;
	static DhcpClass* _dhcp;
public:
	// Initialise the Ethernet shield to use the provided MAC address and
	// gain the rest of the configuration through DHCP.
	// Returns 0 if the DHCP configuration failed, and 1 if it succeeded
	static int begin(uint8_t *mac, unsigned long timeout = 60000, unsigned long responseTimeout = 4000);
	static int maintain();

	// Manaul configuration
	static void begin(uint8_t *mac, IPAddress ip);
	static void begin(uint8_t *mac, IPAddress ip, IPAddress dns);
	static void begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway);
	static void begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet);

	static IPAddress localIP();
	static IPAddress subnetMask();
	static IPAddress gatewayIP();
	static IPAddress dnsServerIP() { return _dnsServerAddress; }

	friend class EthernetClient;
	friend class EthernetServer;
	friend class EthernetUdp;




};

extern EthernetClass Ethernet;

#endif
