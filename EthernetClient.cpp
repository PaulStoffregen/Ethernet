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

int EthernetClient::connect(const char* host, uint16_t port) {
  // Look up the host first
  int ret = 0;
  DNSClient dns;
  IPAddress remote_addr;

  dns.begin(Ethernet.dnsServerIP());
  ret = dns.getHostByName(host, remote_addr);
  if (ret == 1) {
    return connect(remote_addr, port);
  } else {
    return ret;
  }
}

int EthernetClient::connect(IPAddress ip, uint16_t port) {
  if (ip == IPAddress(0ul) || ip == IPAddress(0xFFFFFFFFul)) return 0;
  if (++srcport == 0) srcport = 49152; // IANA recommended ephemeral port range 49152-65535

  if (!sock.begin(SnMR::TCP, srcport)) return 0;
  sock.connect(rawIPAddress(ip), port);
  while (status() != SnSR::ESTABLISHED) {
    if (!sock) return 0;
    delay(1);
  }
  return 1;
}

size_t EthernetClient::write(uint8_t b) {
  return write(&b, 1);
}

size_t EthernetClient::write(const uint8_t *buf, size_t size) {
  if (!sock || !sock.send(buf, size)) {
    setWriteError();
    return 0;
  }
  return size;
}

int EthernetClient::available() {
  if (!sock) return 0;
  return sock.recvAvailable();
}

int EthernetClient::read() {
  if (!sock) return -1;
  uint8_t b;
  if (sock.recv(&b, 1) > 0 ) {
    // recv worked
    return b;
  } else {
    // No data available
    return -1;
  }
}

int EthernetClient::read(uint8_t *buf, size_t size) {
  return sock.recv(buf, size);
}

int EthernetClient::peek() {
  // Unlike recv, peek doesn't check to see if there's any data available, so we must
  if (!sock || !available()) return -1;
  uint8_t b;
  sock.peek(&b);
  return b;
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
