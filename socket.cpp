#include "w5100.h"
#include "socket.h"
#include "IPAddress.h"
#include "EthernetServer.h"

#include "Arduino.h"

#if ARDUINO >= 156 || TEENSYDUINO >= 120
extern void yield(void);
#else
#define yield()
#endif

static uint16_t local_port;

typedef struct {
	uint16_t RX_RSR;
	uint16_t RX_RD;
} socketstate_t;

static socketstate_t state[MAX_SOCK_NUM];


static uint16_t getTXFreeSize(uint8_t s);
static uint16_t getRXReceivedSize(uint8_t s);
static void send_data_processing(uint8_t s, uint16_t data_offset, const uint8_t *data, uint16_t len);
static void read_data(uint8_t s, uint16_t src, volatile uint8_t *dst, uint16_t len);
static void recv_data_processing(uint8_t s, uint8_t *data, uint16_t len, uint8_t peek);



/*****************************************/
/*          Socket management            */
/*****************************************/


uint8_t socketBegin(uint8_t protocol, uint16_t port)
{
	//Serial.printf("W5000socket begin, protocol=%d, port=%d\n", protocol, port);
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	uint8_t s, status[MAX_SOCK_NUM];
#if 0
	if (s < MAX_SOCK_NUM) {
		//Serial.printf("W5000socket step1\n");
		// we already have a hardware socket
		uint8_t stat;
		stat = W5100.readSnSR(s);
		if (stat == SnSR::CLOSED) {
			// closed, so just use it
			goto makesocket;
		} else if (stat == SnSR::ESTABLISHED || stat == SnSR::INIT
		  || stat == SnSR::CLOSE_WAIT) {
			// connected (TCP), try disconnect
  			W5100.execCmdSn(s, Sock_DISCON);
		}
		status[s] = stat;
	}
#endif
	// look at all the hardware sockets, use any closed
	for (s=0; s < MAX_SOCK_NUM; s++) {
		status[s] = W5100.readSnSR(s);
		if (status[s] == SnSR::CLOSED) goto makesocket;
	}
#if 0
	Serial.printf("W5000socket step3\n");
	// next, use any that are effectively closed
	for (s=0; s < MAX_SOCK_NUM; s++) {
		uint8_t stat = status[s];
		// TODO: this also needs to check if no more data
		if (stat == SnSR::CLOSE_WAIT) goto closemakesocket;
	}
#endif
	//Serial.printf("W5000socket step4\n");
	// as a last resort, forcibly close any already closing
	for (s=0; s < MAX_SOCK_NUM; s++) {
		uint8_t stat = status[s];
		if (stat == SnSR::LAST_ACK) goto closemakesocket;
		if (stat == SnSR::TIME_WAIT) goto closemakesocket;
		if (stat == SnSR::FIN_WAIT) goto closemakesocket;
		if (stat == SnSR::CLOSING) goto closemakesocket;
	}
	SPI.endTransaction();
	return MAX_SOCK_NUM; // all sockets are in use
closemakesocket:
	//Serial.printf("W5000socket close\n");
	W5100.execCmdSn(s, Sock_CLOSE);
makesocket:
	//Serial.printf("W5000socket %d\n", s);
	EthernetServer::server_port[s] = 0;
	delayMicroseconds(25); // TODO: is this needed??
	W5100.writeSnMR(s, protocol);
	W5100.writeSnIR(s, 0xFF);
	if (port > 0) {
		W5100.writeSnPORT(s, port);
	} else {
		// if don't set the source port, set local_port number.
		W5100.writeSnPORT(s, ++local_port);
	}
	W5100.execCmdSn(s, Sock_OPEN);
	//state[s] = W5100.
	//Serial.printf("W5000socket prot=%d\n", W5100.readSnMR(s));
	SPI.endTransaction();
	return s;
}


uint8_t socketStatus(uint8_t s)
{
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	uint8_t status = W5100.readSnSR(s);
	SPI.endTransaction();
	return status;
}

/**
 * @brief	This function close the socket and parameter is "s" which represent the socket number
 */
void socketClose(uint8_t s)
{
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.execCmdSn(s, Sock_CLOSE);
	SPI.endTransaction();
}


/**
 * @brief	This function established  the connection for the channel in passive (server) mode. This function waits for the request from the peer.
 * @return	1 for success else 0.
 */
uint8_t socketListen(uint8_t s)
{
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	if (W5100.readSnSR(s) != SnSR::INIT) {
		SPI.endTransaction();
		return 0;
	}
	W5100.execCmdSn(s, Sock_LISTEN);
	SPI.endTransaction();
	return 1;
}


/**
 * @brief	This function established  the connection for the channel in Active (client) mode.
 */
void socketConnect(uint8_t s, uint8_t * addr, uint16_t port)
{
	// set destination IP
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.writeSnDIPR(s, addr);
	W5100.writeSnDPORT(s, port);
	W5100.execCmdSn(s, Sock_CONNECT);
	SPI.endTransaction();
}



/**
 * @brief	This function used for disconnect the socket and parameter is "s" which represent the socket number
 * @return	1 for success else 0.
 */
void socketDisconnect(uint8_t s)
{
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.execCmdSn(s, Sock_DISCON);
	SPI.endTransaction();
}



/*****************************************/
/*         Socket Data Functions         */
/*****************************************/


static uint16_t getTXFreeSize(uint8_t s)
{
        uint16_t val, prev;

        prev = W5100.readSnTX_FSR(s);
        while (1) {
                val = W5100.readSnTX_FSR(s);
                if (val == prev) return val;
                prev = val;
        }
}

static uint16_t getRXReceivedSize(uint8_t s)
{
        uint16_t val, prev;

        prev = W5100.readSnRX_RSR(s);
        while (1) {
                val = W5100.readSnRX_RSR(s);
                if (val == prev) return val;
                prev = val;
        }
}


static void send_data_processing(uint8_t s, uint16_t data_offset, const uint8_t *data, uint16_t len)
{
	uint16_t ptr = W5100.readSnTX_WR(s);
	ptr += data_offset;
	uint16_t offset = ptr & W5100.SMASK;
	uint16_t dstAddr = offset + W5100.SBASE[s];

	if (offset + len > W5100.SSIZE) {
		// Wrap around circular buffer
		uint16_t size = W5100.SSIZE - offset;
		W5100.write(dstAddr, data, size);
		W5100.write(W5100.SBASE[s], data + size, len - size);
	} else {
		W5100.write(dstAddr, data, len);
	}
	ptr += len;
	W5100.writeSnTX_WR(s, ptr);
}


static void read_data(uint8_t s, uint16_t src, volatile uint8_t *dst, uint16_t len)
{
	uint16_t size;
	uint16_t src_mask;
	uint16_t src_ptr;

	src_mask = (uint16_t)src & W5100.SMASK;
	src_ptr = W5100.RBASE[s] + src_mask;

	if ((src_mask + len) > W5100.SSIZE) {
		size = W5100.SSIZE - src_mask;
		W5100.read(src_ptr, (uint8_t *)dst, size);
		dst += size;
		W5100.read(W5100.RBASE[s], (uint8_t *) dst, len - size);
	} else {
		W5100.read(src_ptr, (uint8_t *) dst, len);
	}
}

static void recv_data_processing(uint8_t s, uint8_t *data, uint16_t len, uint8_t peek)
{
	uint16_t ptr;
	ptr = W5100.readSnRX_RD(s);
	read_data(s, ptr, data, len);
	if (!peek) {
		ptr += len;
		W5100.writeSnRX_RD(s, ptr);
	}
}



/**
 * @brief	This function used to send the data in TCP mode
 * @return	1 for success else 0.
 */
uint16_t socketSend(uint8_t s, const uint8_t * buf, uint16_t len)
{
	uint8_t status=0;
	uint16_t ret=0;
	uint16_t freesize=0;

	if (len > W5100.SSIZE) {
		ret = W5100.SSIZE; // check size not to exceed MAX size.
	} else {
		ret = len;
	}

	// if freebuf is available, start.
	do {
		SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
		freesize = getTXFreeSize(s);
		status = W5100.readSnSR(s);
		SPI.endTransaction();
		if ((status != SnSR::ESTABLISHED) && (status != SnSR::CLOSE_WAIT)) {
			ret = 0;
			break;
		}
		yield();
	} while (freesize < ret);

	// copy data
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	send_data_processing(s, 0, (uint8_t *)buf, ret);
	W5100.execCmdSn(s, Sock_SEND);

	/* +2008.01 bj */
	while ( (W5100.readSnIR(s) & SnIR::SEND_OK) != SnIR::SEND_OK ) {
		/* m2008.01 [bj] : reduce code */
		if ( W5100.readSnSR(s) == SnSR::CLOSED ) {
			SPI.endTransaction();
			return 0;
		}
		SPI.endTransaction();
		yield();
		SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	}
	/* +2008.01 bj */
	W5100.writeSnIR(s, SnIR::SEND_OK);
	SPI.endTransaction();
	return ret;
}


/**
 * @brief	This function is an application I/F function which is used to receive the data in TCP mode.
 * 		It continues to wait for data as much as the application wants to receive.
 *
 * @return	received data size for success else -1.
 */
int socketRecv(uint8_t s, uint8_t *buf, int16_t len)
{
	// Check how much data is available
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	int16_t ret = getRXReceivedSize(s);
	if (ret == 0) {
		// No data available.
		uint8_t status = W5100.readSnSR(s);
		if ( status == SnSR::LISTEN || status == SnSR::CLOSED || status == SnSR::CLOSE_WAIT ) {
			// The remote end has closed its side of the connection, so this is the eof state
			ret = 0;
		} else {
			// The connection is still up, but there's no data waiting to be read
			ret = -1;
		}
	} else if (ret > len) {
		ret = len;
	}

	if (ret > 0) {
		recv_data_processing(s, buf, ret, 0);
		W5100.execCmdSn(s, Sock_RECV);
	}
	SPI.endTransaction();
	return ret;
}

uint16_t socketRecvAvailable(uint8_t s)
{
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	uint16_t ret = getRXReceivedSize(s);
	//uint8_t ir = W5100.readSnIR(s);
	SPI.endTransaction();
	//Serial.printf("sock.recvAvailable s=%d, ir=%02X, num=%d\n", s, ir, ret);
	return ret;
}

/**
 * @brief	Returns the first byte in the receive queue (no checking)
 *
 * @return
 */
uint16_t socketPeek(uint8_t s, uint8_t *buf)
{
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	recv_data_processing(s, buf, 1, 1);
	SPI.endTransaction();
	return 1;
}


/**
 * @brief	This function is an application I/F function which is used to send the data for other then TCP mode.
 * 		Unlike TCP transmission, The peer's destination address and the port is needed.
 *
 * @return	This function return send data size for success else -1.
 */
uint16_t socketSendto(uint8_t s, const uint8_t *buf, uint16_t len, uint8_t *addr, uint16_t port)
{
	uint16_t ret=0;

	if (len > W5100.SSIZE) ret = W5100.SSIZE; // check size not to exceed MAX size.
	else ret = len;

	if ( ((addr[0] == 0x00) && (addr[1] == 0x00) && (addr[2] == 0x00) && (addr[3] == 0x00)) ||
	((port == 0x00)) ||(ret == 0) ) {
		return 0;
	}
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.writeSnDIPR(s, addr);
	W5100.writeSnDPORT(s, port);

	// copy data
	send_data_processing(s, 0, (uint8_t *)buf, ret);
	W5100.execCmdSn(s, Sock_SEND);

	while ( (W5100.readSnIR(s) & SnIR::SEND_OK) != SnIR::SEND_OK ) {
		if (W5100.readSnIR(s) & SnIR::TIMEOUT) {
			/* clear SEND_OK & TIMEOUT */
			W5100.writeSnIR(s, (SnIR::SEND_OK | SnIR::TIMEOUT));
			SPI.endTransaction();
		return 0;
		}
		SPI.endTransaction();
		yield();
		SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	}
	W5100.writeSnIR(s, SnIR::SEND_OK);
	SPI.endTransaction();
	return ret;
}


/**
 * @brief	This function is an application I/F function which is used to receive the data in other then
 * 	TCP mode. This function is used to receive UDP, IP_RAW and MAC_RAW mode, and handle the header as well.
 *
 * @return	This function return received data size for success else -1.
 */
uint16_t socketRecvfrom(uint8_t s, uint8_t *buf, uint16_t len, uint8_t *addr, uint16_t *port)
{
  uint8_t head[8];
  uint16_t data_len=0;
  uint16_t ptr=0;

  if ( len > 0 )
  {
    SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
    ptr = W5100.readSnRX_RD(s);
    switch (W5100.readSnMR(s) & 0x07)
    {
    case SnMR::UDP :
      read_data(s, ptr, head, 0x08);
      ptr += 8;
      // read peer's IP address, port number.
      addr[0] = head[0];
      addr[1] = head[1];
      addr[2] = head[2];
      addr[3] = head[3];
      *port = head[4];
      *port = (*port << 8) + head[5];
      data_len = head[6];
      data_len = (data_len << 8) + head[7];

      read_data(s, ptr, buf, data_len); // data copy.
      ptr += data_len;

      W5100.writeSnRX_RD(s, ptr);
      break;

    case SnMR::IPRAW :
      read_data(s, ptr, head, 0x06);
      ptr += 6;

      addr[0] = head[0];
      addr[1] = head[1];
      addr[2] = head[2];
      addr[3] = head[3];
      data_len = head[4];
      data_len = (data_len << 8) + head[5];

      read_data(s, ptr, buf, data_len); // data copy.
      ptr += data_len;

      W5100.writeSnRX_RD(s, ptr);
      break;

    case SnMR::MACRAW:
      read_data(s, ptr, head, 2);
      ptr+=2;
      data_len = head[0];
      data_len = (data_len<<8) + head[1] - 2;

      read_data(s, ptr, buf, data_len);
      ptr += data_len;
      W5100.writeSnRX_RD(s, ptr);
      break;

    default :
      break;
    }
    W5100.execCmdSn(s, Sock_RECV);
    SPI.endTransaction();
  }
  return data_len;
}

uint16_t socketBufferData(uint8_t s, uint16_t offset, const uint8_t* buf, uint16_t len)
{
	//Serial.printf("  bufferData, offset=%d, len=%d\n", offset, len);
	uint16_t ret =0;
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	uint16_t txfree = getTXFreeSize(s);
	if (len > txfree) {
		ret = txfree; // check size not to exceed MAX size.
	} else {
		ret = len;
	}
	send_data_processing(s, offset, buf, ret);
	SPI.endTransaction();
	return ret;
}

int socketStartUDP(uint8_t s, uint8_t* addr, uint16_t port)
{
	if ( ((addr[0] == 0x00) && (addr[1] == 0x00) && (addr[2] == 0x00) && (addr[3] == 0x00)) ||
	  ((port == 0x00)) ) {
		return 0;
	}
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.writeSnDIPR(s, addr);
	W5100.writeSnDPORT(s, port);
	SPI.endTransaction();
	return 1;
}

int socketSendUDP(uint8_t s)
{
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.execCmdSn(s, Sock_SEND);

	/* +2008.01 bj */
	while ( (W5100.readSnIR(s) & SnIR::SEND_OK) != SnIR::SEND_OK ) {
		if (W5100.readSnIR(s) & SnIR::TIMEOUT) {
			/* +2008.01 [bj]: clear interrupt */
			W5100.writeSnIR(s, (SnIR::SEND_OK|SnIR::TIMEOUT));
			SPI.endTransaction();
			//Serial.printf("sendUDP timeout\n");
			return 0;
		}
		SPI.endTransaction();
		yield();
		SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	}

	/* +2008.01 bj */
	W5100.writeSnIR(s, SnIR::SEND_OK);
	SPI.endTransaction();

	//Serial.printf("sendUDP ok\n");
	/* Sent ok */
	return 1;
}

