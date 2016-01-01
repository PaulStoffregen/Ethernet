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

uint16_t EthernetClient::srcport = 49152;      //Use IANA recommended ephemeral port range 49152-65535

int EthernetClient::connect(const char * host, uint16_t port)
{
	DNSClient dns; // Look up the host first
	IPAddress remote_addr;

	dns.begin(Ethernet.dnsServerIP());
	if (!dns.getHostByName(host, remote_addr)) return 0;
	return connect(remote_addr, port);
}

int EthernetClient::connect(IPAddress ip, uint16_t port)
{
	if (ip == IPAddress(0ul) || ip == IPAddress(0xFFFFFFFFul)) return 0;
	if (++srcport == 0) srcport = 49152; // IANA recommended ephemeral range 49152-65535
	if (!sock.begin(SnMR::TCP, srcport)) return 0;
	sock.connect(rawIPAddress(ip), port);
	while (status() != SnSR::ESTABLISHED) {
		if (!sock) return 0;
		delay(1);
	}
	//recv_count = sock.recvAvailable();
	//recv_offset = sock.recvOffset();
	//recv_buffer_count = 0;
	return 1;
}

size_t EthernetClient::write(uint8_t b)
{
	return write(&b, 1);
}

size_t EthernetClient::write(const uint8_t *buf, size_t size)
{
	if (!sock || !sock.send(buf, size)) {
		setWriteError();
		return 0;
	}
	return size;
}

// registers
// Sn_RX_RSR  Number of bytes received
// Sn_RX_RD   Address to read
// 

int EthernetClient::available()
{
	if (!sock) return 0;
#if 0
	uint16_t count = recv_buffer_count + recv_count;
	if (count == 0) {
		count = recv_count = sock.recvAvailable();
	}
	return count;
#else
	return sock.recvAvailable();
#endif
}

int EthernetClient::read(uint8_t *buf, size_t size)
{
#if 0
	size_t count=0;
	if (!sock) return 0;

	// first, if we have any data buffered in memory, use it
	if (recv_buffer_count) {
		size_t len = recv_buffer_count;
		if (len > size) len = size;
		memcpy(buf, recv_buffer, len);
		buf += len;
		recv_buffer_count -= len;
		if (recv_buffer_count) {
			memmove(recv_buffer, recv_buffer + len, recv_buffer_count);
		}
		count = len;
		if (count >= size) return count;
	}
	// if our awareness of the available data isn't enough, update
	if (recv_count < size + 15) {
		recv_count = sock.recvAvailable();
		if (recv_count == 0) {
			if (count) return count;
			uint8_t status = sock.socketStatus();
			if (status == SnSR::LISTEN || status == SnSR::CLOSED ||
			   status == SnSR::CLOSE_WAIT ) {
				// The remote end has closed its side of the
				// connection, so this is the eof state
				return 0;
			} else {
				// The connection is still up, but there's
				// no data waiting to be read
				return -1;
			}
		}
	}
	// now transfer data directly from W5000 to the user buffer
	size_t len = recv_count;
	if (len > size - count) len = size - count;
	sock.read_data(recv_offset, buf, len);
	buf += len;
	recv_offset += len;
	recv_count -= len;
	count += len;
	// if the W5000 still has more data, refill our buffer
	if (recv_count > 0) {
		len = recv_count;
		if (len > 15) len = 15;
		sock.read_data(recv_offset, recv_buffer, len);
		recv_buffer_count = len;
		recv_offset += len;
		recv_count -= len;
	}
	recv_offset &= W5100.SMASK;
	sock.updateRecvOffset(recv_offset);
	return count;
#else
	if (!sock) return 0;
	return sock.recv(buf, size);
#endif
}

int EthernetClient::peek()
{
	if (!sock || !available()) return -1;
	uint8_t b;
	sock.peek(&b);
	return b;
}

int EthernetClient::read()
{
	uint8_t b;
	if (sock.recv(&b, 1) > 0) return b;
	return -1;
}




void EthernetClient::flush() {
  sock.flush();
}

void EthernetClient::stop() {
  if (!sock) return;

  // attempt to close the connection gracefully (send a FIN to other side)
  sock.disconnect();
  unsigned long start = millis();

  // wait up to a second for the connection to close
  uint8_t s;
  do {
    s = status();
    if (s == SnSR::CLOSED) break; // exit the loop
    delay(1);
  } while (millis() - start < 1000);

  // if it hasn't closed, close it forcefully
  if (s != SnSR::CLOSED) sock.close();
}

uint8_t EthernetClient::connected() {
  if (!sock) return 0;
  
  uint8_t s = status();
  return !(s == SnSR::LISTEN || s == SnSR::CLOSED || s == SnSR::FIN_WAIT ||
    (s == SnSR::CLOSE_WAIT && !available()));
}

uint8_t EthernetClient::status() {
  if (!sock) return SnSR::CLOSED;
  return sock.socketStatus();
}

// the next function allows us to use the client returned by
// EthernetServer::available() as the condition in an if-statement.

bool EthernetClient::operator==(const EthernetClient& rhs) {
  return sock && rhs.sock && getSocketNumber() == rhs.getSocketNumber();
}
