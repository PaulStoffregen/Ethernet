#include "w5100.h"
#include "socket.h"
extern "C" {
#include "string.h"
}

#include "Ethernet.h"
#include "EthernetClient.h"
#include "EthernetServer.h"

EthernetServer::EthernetServer(uint16_t port)
{
  _port = port;
}

void EthernetServer::begin()
{
  sock.begin(SnMR::TCP, _port);
  if (sock) sock.listen();
}

void EthernetServer::accept()
{
  bool listening = false;

  if (sock) {
    uint8_t status = sock.socketStatus();
    if (status == SnSR::LISTEN) {
      listening = true;
    } else if (status == SnSR::CLOSE_WAIT && sock.recvAvailable() <= 0) {
      sock.disconnect();
      // TODO: wait?
    }
  }
  if (!listening) begin();
}

EthernetClient EthernetServer::available()
{
  accept();
  if (sock) {
    uint8_t stat = sock.socketStatus();
    if (stat == SnSR::ESTABLISHED || (stat == SnSR::CLOSE_WAIT && sock.recvAvailable() > 0)) {
      EthernetClient client(sock);
      return client;
    }
  }
  return EthernetClient();
/*
  for (int sock = 0; sock < MAX_SOCK_NUM; sock++) {
    EthernetClient client(sock);
    if (EthernetClass::_server_port[sock] == _port) {
      uint8_t s = client.status();
      if (s == SnSR::ESTABLISHED || s == SnSR::CLOSE_WAIT) {
        if (client.available()) {
          // XXX: don't always pick the lowest numbered socket.
          return client;
        }
      }
    }
  }

  return EthernetClient(MAX_SOCK_NUM);
*/
}

size_t EthernetServer::write(uint8_t b) 
{
  return write(&b, 1);
}

size_t EthernetServer::write(const uint8_t *buffer, size_t size) 
{
  //size_t n = 0;

  accept();
  if (sock) {
    uint8_t stat = sock.socketStatus();
    if (stat == SnSR::ESTABLISHED) {
      return sock.send(buffer, size);
    }
  }
  return 0;
}
/*
  for (int sock = 0; sock < MAX_SOCK_NUM; sock++) {
    EthernetClient client(sock);

    if (EthernetClass::_server_port[sock] == _port &&
      client.status() == SnSR::ESTABLISHED) {
      n += client.write(buffer, size);
    }
  }
  
  return n;
}
*/
