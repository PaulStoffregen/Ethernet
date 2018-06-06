#include "Ethernet.h"
#include "w5100.h"
#include "Dhcp.h"

IPAddress EthernetClass::_dnsServerAddress;
DhcpClass* EthernetClass::_dhcp = NULL;
uint8_t EthernetClass::_initialized;
uint8_t EthernetClass::_releasing;

int EthernetClass::begin(uint8_t *mac, unsigned long timeout, unsigned long responseTimeout)
{
	initialize(mac, timeout, responseTimeout);
	int result = 0;
	while (result == 0) {
		result = initialized();
	}
	return result == 1;
}

void EthernetClass::initialize(uint8_t *mac, unsigned long timeout, unsigned long responseTimeout)
{
	static DhcpClass s_dhcp;
	_dhcp = &s_dhcp;

	// Initialise the basic info
	W5100.init();
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.setMACAddress(mac);
	W5100.setIPAddress(IPAddress(0,0,0,0).raw_address());
	SPI.endTransaction();

	// Now try to get our config info from a DHCP server
	_dhcp->initialize(mac, timeout, responseTimeout);
	_initialized = 0;
}

int EthernetClass::initialized() {
	if (_dhcp == NULL || _initialized) {
		return 1;
	}
	int result = _dhcp->successful();
	if (result == 1) {
		// We've successfully found a DHCP server and got our configuration
		// info, so set things accordingly
		SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
		W5100.setIPAddress(_dhcp->getLocalIp().raw_address());
		W5100.setGatewayIp(_dhcp->getGatewayIp().raw_address());
		W5100.setSubnetMask(_dhcp->getSubnetMask().raw_address());
		SPI.endTransaction();
		_dnsServerAddress = _dhcp->getDnsServerIp();
		socketPortRand(micros());
		_initialized = 1;
		_releasing = 0;
	}
	else if (result == 2) {
		_initialized = 1;
		_releasing = 0;
	}
	return result;
}

void EthernetClass::begin(uint8_t *mac, IPAddress ip)
{
	// Assume the DNS server will be the machine on the same network as the local IP
	// but with last octet being '1'
	IPAddress dns = ip;
	dns[3] = 1;
	begin(mac, ip, dns);
}

void EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns)
{
	// Assume the gateway will be the machine on the same network as the local IP
	// but with last octet being '1'
	IPAddress gateway = ip;
	gateway[3] = 1;
	begin(mac, ip, dns, gateway);
}

void EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway)
{
	IPAddress subnet(255, 255, 255, 0);
	begin(mac, ip, dns, gateway, subnet);
}

void EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet)
{
	W5100.init();
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.setMACAddress(mac);
#if ARDUINO > 106 || TEENSYDUINO > 121
	W5100.setIPAddress(ip._address.bytes);
	W5100.setGatewayIp(gateway._address.bytes);
	W5100.setSubnetMask(subnet._address.bytes);
#else
	W5100.setIPAddress(ip._address);
	W5100.setGatewayIp(gateway._address);
	W5100.setSubnetMask(subnet._address);
#endif
	SPI.endTransaction();
	_dnsServerAddress = dns;
	_initialized = 1;
	_releasing = 0;
}

void EthernetClass::init(uint8_t sspin)
{
	W5100.setSS(sspin);
}

int EthernetClass::maintain()
{
	int result = maintainNeeded();
	if (result == DHCP_CHECK_NONE) {
		return result;
	}
	if (result == DHCP_CHECK_REBIND_STARTED) {
		result = 3;
	}
	int res = maintainFinished();
	while (res == 0) {
		delay(10);
		res = maintainFinished();
	}
	return result + (res == 1 ? 1 : 0);
}

int EthernetClass::maintainFinished() {
	int result = _dhcp->successful();
	if (result == 1) {
		_releasing = 0;
		SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
		W5100.setIPAddress(_dhcp->getLocalIp().raw_address());
		W5100.setGatewayIp(_dhcp->getGatewayIp().raw_address());
		W5100.setSubnetMask(_dhcp->getSubnetMask().raw_address());
		SPI.endTransaction();
		_dnsServerAddress = _dhcp->getDnsServerIp();
	}
	return result;
}

int EthernetClass::maintainNeeded()
{
	int rc = DHCP_CHECK_NONE;
	if (_initialized && _dhcp != NULL) {
		if (_releasing) {
			return 1;
		}
		// we have a pointer to dhcp, use it
		rc = _dhcp->checkLease();
		if (rc != DHCP_CHECK_NONE) {
			_releasing = 1;
			return rc;
		}
	}
	return rc;
}

IPAddress EthernetClass::localIP()
{
	IPAddress ret;
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.getIPAddress(ret.raw_address());
	SPI.endTransaction();
	return ret;
}

IPAddress EthernetClass::subnetMask()
{
	IPAddress ret;
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.getSubnetMask(ret.raw_address());
	SPI.endTransaction();
	return ret;
}

IPAddress EthernetClass::gatewayIP()
{
	IPAddress ret;
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.getGatewayIp(ret.raw_address());
	SPI.endTransaction();
	return ret;
}












EthernetClass Ethernet;
