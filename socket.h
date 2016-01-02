#ifndef	_SOCKET_H_
#define	_SOCKET_H_

#include "w5100.h"

// Opens a socket(TCP or UDP or IP_RAW mode)
//uint8_t socket(SOCKET s, uint8_t protocol, uint16_t port, uint8_t flag);
uint8_t begin(uint8_t protocol, uint16_t port);

uint8_t socketStatus(uint8_t s);
// Close socket
void close(uint8_t s);
// Establish TCP connection (Active connection)
void connect(uint8_t s, uint8_t * addr, uint16_t port);
// disconnect the connection
void disconnect(uint8_t s);
// Establish TCP connection (Passive connection)
uint8_t listen(uint8_t s);
// Send data (TCP)
uint16_t send(uint8_t s, const uint8_t * buf, uint16_t len);
// Receive data (TCP)
int16_t recv(uint8_t s, uint8_t * buf, int16_t len);
void read_data(uint8_t s, uint16_t src, volatile uint8_t *dst, uint16_t len);
void updateRecvOffset(uint8_t s, uint16_t offset);
uint16_t recvAvailable(uint8_t s);
uint16_t recvOffset(uint8_t s);
uint16_t peek(uint8_t s, uint8_t *buf);
 // Send data (UDP/IP RAW)
uint16_t sendto(uint8_t s, const uint8_t * buf, uint16_t len, uint8_t * addr, uint16_t port);
// Receive data (UDP/IP RAW)
uint16_t recvfrom(uint8_t s, uint8_t * buf, uint16_t len, uint8_t * addr, uint16_t *port);

uint16_t igmpsend(uint8_t s, const uint8_t * buf, uint16_t len);

// Functions to allow buffered UDP send (i.e. where the UDP datagram is built up over a
// number of calls before being sent
/*
  @brief This function sets up a UDP datagram, the data for which will be provided by one
  or more calls to bufferData and then finally sent with sendUDP.
  @return 1 if the datagram was successfully set up, or 0 if there was an error
*/
int startUDP(uint8_t s, uint8_t* addr, uint16_t port);
/*
  @brief This function copies up to len bytes of data from buf into a UDP datagram to be
  sent later by sendUDP.  Allows datagrams to be built up from a series of bufferData calls.
  @return Number of bytes successfully buffered
*/
uint16_t bufferData(uint8_t s, uint16_t offset, const uint8_t* buf, uint16_t len);
/*
  @brief Send a UDP datagram built up from a sequence of startUDP followed by one or more
  calls to bufferData.
  @return 1 if the datagram was successfully sent, or 0 if there was an error
*/
int sendUDP(uint8_t s);

#endif
/* _SOCKET_H_ */
