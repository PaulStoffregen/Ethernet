#ifndef ethernetserver_h
#define ethernetserver_h

#include "Server.h"
#include "socket.h"

class EthernetClient;

class EthernetServer : 
public Server {
private:
	uint16_t _port;
	uint8_t sockindex;
	void accept();
public:
	EthernetServer(uint16_t port) : _port(port), sockindex(MAX_SOCK_NUM) { }
	EthernetClient available();
	virtual void begin();
	virtual size_t write(uint8_t);
	virtual size_t write(const uint8_t *buf, size_t size);
	using Print::write;

	// TODO: make private when socket allocation moves to EthernetClass
	static uint16_t server_port[MAX_SOCK_NUM];
};

#endif
