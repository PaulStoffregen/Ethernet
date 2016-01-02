#include "w5100.h"
#include "socket.h"
extern "C" {
#include "string.h"
}

#include "Ethernet.h"
#include "EthernetClient.h"
#include "EthernetServer.h"

uint16_t EthernetServer::server_port[MAX_SOCK_NUM];


void EthernetServer::begin()
{
	sockindex = socketBegin(SnMR::TCP, _port);
	if (sockindex < MAX_SOCK_NUM) {
		socketListen(sockindex);
		server_port[sockindex] = _port;
	}
}

void EthernetServer::accept()
{
	bool listening = false;

	if (sockindex < MAX_SOCK_NUM) {
		uint8_t status = socketStatus(sockindex);
		if (status == SnSR::LISTEN) {
			listening = true;
		} else if (status == SnSR::CLOSE_WAIT && socketRecvAvailable(sockindex) <= 0) {
			socketDisconnect(sockindex);
		}
	}
	if (!listening) begin();
}

EthernetClient EthernetServer::available()
{
	accept();
	for (uint8_t i=0; i < MAX_SOCK_NUM; i++) {
		if (server_port[i] == _port) {
			uint8_t stat = socketStatus(i);
			if (stat == SnSR::ESTABLISHED || stat == SnSR::CLOSE_WAIT) {
				if (socketRecvAvailable(i) > 0) {
					EthernetClient client(i);
					return client;
				}
			}
		}
	}
	return EthernetClient();
}

size_t EthernetServer::write(uint8_t b) 
{
  return write(&b, 1);
}

size_t EthernetServer::write(const uint8_t *buffer, size_t size) 
{
	accept();
	for (uint8_t i=0; i < MAX_SOCK_NUM; i++) {
		if (server_port[i] == _port) {
			if (socketStatus(i) == SnSR::ESTABLISHED) {
				socketSend(i, buffer, size);
			}
		}
	}
	return size;
}
