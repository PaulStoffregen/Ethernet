#ifndef	_SOCKET_H_
#define	_SOCKET_H_

#include "w5100.h"

class W5000socket {
public:
	W5000socket() : s(MAX_SOCK_NUM) { }

	// Opens a socket(TCP or UDP or IP_RAW mode)
	//uint8_t socket(SOCKET s, uint8_t protocol, uint16_t port, uint8_t flag);
	uint8_t begin(uint8_t protocol, uint16_t port);

	uint8_t socketStatus();
	// Close socket
	void close();
	// Establish TCP connection (Active connection)
	void connect(uint8_t * addr, uint16_t port);
	// disconnect the connection
	void disconnect();
	// Establish TCP connection (Passive connection)
	uint8_t listen();
	// Send data (TCP)
	uint16_t send(const uint8_t * buf, uint16_t len);
	// Receive data (TCP)
	int16_t recv(uint8_t * buf, int16_t len);
	void read_data(uint16_t src, volatile uint8_t *dst, uint16_t len);
	void updateRecvOffset(uint16_t offset);
	uint16_t recvAvailable();
	uint16_t recvOffset();
	uint16_t peek(uint8_t *buf);
	 // Send data (UDP/IP RAW)
	uint16_t sendto(const uint8_t * buf, uint16_t len, uint8_t * addr, uint16_t port);
	// Receive data (UDP/IP RAW)
	uint16_t recvfrom(uint8_t * buf, uint16_t len, uint8_t * addr, uint16_t *port);
	// Wait for transmission to complete
	void flush();

	uint16_t igmpsend(const uint8_t * buf, uint16_t len);

	// Functions to allow buffered UDP send (i.e. where the UDP datagram is built up over a
	// number of calls before being sent
/*
  @brief This function sets up a UDP datagram, the data for which will be provided by one
  or more calls to bufferData and then finally sent with sendUDP.
  @return 1 if the datagram was successfully set up, or 0 if there was an error
*/
	int startUDP(uint8_t* addr, uint16_t port);
/*
  @brief This function copies up to len bytes of data from buf into a UDP datagram to be
  sent later by sendUDP.  Allows datagrams to be built up from a series of bufferData calls.
  @return Number of bytes successfully buffered
*/
	uint16_t bufferData(uint16_t offset, const uint8_t* buf, uint16_t len);
/*
  @brief Send a UDP datagram built up from a sequence of startUDP followed by one or more
  calls to bufferData.
  @return 1 if the datagram was successfully sent, or 0 if there was an error
*/
	int sendUDP();

	void moveTo(W5000socket &rhs);
	operator bool() const { return s < MAX_SOCK_NUM; }
	SOCKET getSocketNumber() const { return s; }
private:
	SOCKET s;
};


#endif
/* _SOCKET_H_ */
