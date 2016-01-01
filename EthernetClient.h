#ifndef ethernetclient_h
#define ethernetclient_h
#include "Arduino.h"	
#include "Print.h"
#include "Client.h"
#include "IPAddress.h"
#include "socket.h"

class EthernetClient : public Client {

public:
  EthernetClient() { };
  EthernetClient(W5000socket &socket) { sock.moveTo(socket); }

  uint8_t status();
  virtual int connect(IPAddress ip, uint16_t port);
  virtual int connect(const char *host, uint16_t port);
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  virtual int available();
  virtual int read();
  virtual int read(uint8_t *buf, size_t size);
  virtual int peek();
  virtual void flush();
  virtual void stop();
  virtual uint8_t connected();
  virtual operator bool() { return (bool)sock; }
  virtual bool operator==(const bool value) { return bool() == value; }
  virtual bool operator!=(const bool value) { return bool() != value; }
  virtual bool operator==(const EthernetClient&);
  virtual bool operator!=(const EthernetClient& rhs) { return !this->operator==(rhs); };
  uint8_t getSocketNumber() const { return sock.getSocketNumber(); }

  friend class EthernetServer;
  
  using Print::write;

private:
	uint16_t recv_count;
	uint16_t recv_offset;
	uint8_t  recv_buffer_count;
	uint8_t  recv_buffer[15];
	W5000socket sock;
	static uint16_t srcport;
};

#endif
