#include "w5100.h"
#include "socket.h"

extern "C" {
  #include "string.h"
}

#include "Arduino.h"

#include "Ethernet.h"
#include "EthernetClient.h"
#include "EthernetServer.h"
#include "Dns.h"


int EthernetClient::connect(const char * host, uint16_t port)
{
	DNSClient dns; // Look up the host first
	IPAddress remote_addr;

	if (sockindex < MAX_SOCK_NUM) {
		if (socketStatus(sockindex) != SnSR::CLOSED) socketDisconnect(sockindex);
		sockindex = MAX_SOCK_NUM;
	}
	dns.begin(Ethernet.dnsServerIP());
	if (!dns.getHostByName(host, remote_addr)) return 0;
	return connect(remote_addr, port);
}

int EthernetClient::connect(IPAddress ip, uint16_t port)
{
	if (sockindex < MAX_SOCK_NUM) {
		if (socketStatus(sockindex) != SnSR::CLOSED) socketDisconnect(sockindex);
		sockindex = MAX_SOCK_NUM;
	}
	if (ip == IPAddress(0ul) || ip == IPAddress(0xFFFFFFFFul)) return 0;
	sockindex = socketBegin(SnMR::TCP, 0);
	if (sockindex >= MAX_SOCK_NUM) return 0;
	socketConnect(sockindex, rawIPAddress(ip), port);
	while (1) {
		uint8_t stat = socketStatus(sockindex);
		if (stat == SnSR::ESTABLISHED) return 1;
		if (stat == SnSR::CLOSED) return 0;
		delay(1);
	}
}

size_t EthernetClient::write(uint8_t b)
{
	return write(&b, 1);
}

size_t EthernetClient::write(const uint8_t *buf, size_t size)
{
	if (sockindex >= MAX_SOCK_NUM) return 0;
	if (socketSend(sockindex, buf, size)) return size;
	setWriteError();
	return 0;
}

int EthernetClient::available()
{
	if (sockindex >= MAX_SOCK_NUM) return 0;
	return socketRecvAvailable(sockindex);
}

int EthernetClient::read(uint8_t *buf, size_t size)
{
	if (sockindex >= MAX_SOCK_NUM) return 0;
	return socketRecv(sockindex, buf, size);
}

int EthernetClient::peek()
{
	if (sockindex >= MAX_SOCK_NUM) return ERROR;
	if (!available()) return -1;
	return socketPeek(sockindex);
}

int EthernetClient::read()
{
	uint8_t b;
	if (socketRecv(sockindex, &b, 1) > 0) return b;
	return ERROR;
}

void EthernetClient::flush()
{
	// TODO: Wait for transmission to complete
}

void EthernetClient::stop()
{
	if (sockindex >= MAX_SOCK_NUM) return;

	// attempt to close the connection gracefully (send a FIN to other side)
	socketDisconnect(sockindex);
	unsigned long start = millis();

	// wait up to a second for the connection to close
	do {
		if (socketStatus(sockindex) == SnSR::CLOSED) return; // exit the loop
		delay(1);
	} while (millis() - start < 1000);

	// if it hasn't closed, close it forcefully
	socketClose(sockindex);
}

uint8_t EthernetClient::connected()
{
	if (sockindex >= MAX_SOCK_NUM) return 0;
  
	uint8_t s = socketStatus(sockindex);
	return !(s == SnSR::LISTEN || s == SnSR::CLOSED || s == SnSR::FIN_WAIT ||
		(s == SnSR::CLOSE_WAIT && !available()));
}

uint8_t EthernetClient::status()
{
	if (sockindex >= MAX_SOCK_NUM) return SnSR::CLOSED;
	return socketStatus(sockindex);
}

// the next function allows us to use the client returned by
// EthernetServer::available() as the condition in an if-statement.

bool EthernetClient::operator==(const EthernetClient& rhs) {
	if (sockindex != rhs.sockindex) return false;
	if (sockindex >= MAX_SOCK_NUM) return false;
	if (rhs.sockindex >= MAX_SOCK_NUM) return false;
	return true;
}




