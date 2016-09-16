#include "w5100.h"
#include "Ethernet.h"
#include "Dhcp.h"

IPAddress EthernetClass::_dnsServerAddress;
DhcpClass* EthernetClass::_dhcp = nullptr;

int EthernetClass::begin(uint8_t *mac, const unsigned long timeout, const unsigned long responseTimeout)
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
	int result = _dhcp->beginWithDHCP(mac, timeout, responseTimeout);
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
	}
	return result;
}

void EthernetClass::begin(uint8_t *mac, const IPAddress ip)
{
	// Assume the DNS server will be the machine on the same network as the local IP
	// but with last octet being '1'
	IPAddress dns = ip;
	dns[3] = 1;
	begin(mac, ip, dns);
}

void EthernetClass::begin(uint8_t *mac, const IPAddress ip, const IPAddress dns)
{
	// Assume the gateway will be the machine on the same network as the local IP
	// but with last octet being '1'
	IPAddress gateway = ip;
	gateway[3] = 1;
	begin(mac, ip, dns, gateway);
}

void EthernetClass::begin(uint8_t *mac, const IPAddress ip, const IPAddress dns, const IPAddress gateway)
{
	IPAddress subnet(255, 255, 255, 0);
	begin(mac, ip, dns, gateway, subnet);
}

void EthernetClass::begin(uint8_t *mac, const IPAddress ip, const IPAddress dns, const IPAddress gateway, const IPAddress subnet)
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
}

int EthernetClass::maintain()
{
	int result = DHCP_CHECK_NONE;
	if (_dhcp != nullptr) {
		// we have a pointer to dhcp, use it
		result = _dhcp->checkLease();
		switch (result) {
		case DHCP_CHECK_NONE:
			//nothing done
			break;
		case DHCP_CHECK_RENEW_OK:
		case DHCP_CHECK_REBIND_OK:
			//we might have got a new IP.
			SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
			W5100.setIPAddress(_dhcp->getLocalIp().raw_address());
			W5100.setGatewayIp(_dhcp->getGatewayIp().raw_address());
			W5100.setSubnetMask(_dhcp->getSubnetMask().raw_address());
			SPI.endTransaction();
			_dnsServerAddress = _dhcp->getDnsServerIp();
			break;
		default:
			//this is actually a error, it will retry though
			break;
		}
	}
	return result;
}

IPAddress EthernetClass::localIP()
{
	IPAddress address;
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.getIPAddress(address.raw_address());
	SPI.endTransaction();
	return address;
}

IPAddress EthernetClass::subnetMask()
{
	IPAddress address;
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.getSubnetMask(address.raw_address());
	SPI.endTransaction();
	return address;
}

IPAddress EthernetClass::gatewayIP()
{
	IPAddress result;
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.getGatewayIp(result.raw_address());
	SPI.endTransaction();
	return result;
}












EthernetClass Ethernet;
