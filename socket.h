#ifndef	_SOCKET_H_
#define	_SOCKET_H_

#include "w5100.h"

void socketPortRand(uint16_t n);

// Opens a socket(TCP or UDP or IP_RAW mode)
uint8_t  socketBegin(uint8_t protocol, uint16_t port);
uint8_t  socketStatus(uint8_t s);
// Close socket
void     socketClose(uint8_t s);
// Establish TCP connection (Active connection)
void     socketConnect(uint8_t s, uint8_t * addr, uint16_t port);
// disconnect the connection
void     socketDisconnect(uint8_t s);
// Establish TCP connection (Passive connection)
uint8_t  socketListen(uint8_t s);
// Send data (TCP)
uint16_t socketSend(uint8_t s, const uint8_t * buf, uint16_t len);
// Receive data (TCP)
int      socketRecv(uint8_t s, uint8_t * buf, int16_t len);
uint16_t socketRecvAvailable(uint8_t s);
uint8_t  socketPeek(uint8_t s);

/*
  @brief This function sets up a UDP datagram, the data for which will be provided by one
  or more calls to bufferData and then finally sent with sendUDP.
  @return 1 if the datagram was successfully set up, or 0 if there was an error
*/
int      socketStartUDP(uint8_t s, uint8_t* addr, uint16_t port);
/*
  @brief This function copies up to len bytes of data from buf into a UDP datagram to be
  sent later by sendUDP.  Allows datagrams to be built up from a series of bufferData calls.
  @return Number of bytes successfully buffered
*/
uint16_t socketBufferData(uint8_t s, uint16_t offset, const uint8_t* buf, uint16_t len);
/*
  @brief Send a UDP datagram built up from a sequence of startUDP followed by one or more
  calls to bufferData.
  @return 1 if the datagram was successfully sent, or 0 if there was an error
*/
int      socketSendUDP(uint8_t s);

#endif
/* _SOCKET_H_ */
