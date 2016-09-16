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

// TODO: randomize this when not using DHCP, but how?
static uint16_t local_port = 49152;  // 49152 to 65535

typedef struct {
	uint16_t RX_RSR; // Number of bytes received
	uint16_t RX_RD;  // Address to read
	uint16_t TX_FSR; // Free space ready for transmit
	uint8_t  RX_inc; // how much have we advanced RX_RD
} socketstate_t;

static socketstate_t state[MAX_SOCK_NUM];


static uint16_t getSnTX_FSR(uint8_t socket);
static uint16_t getSnRX_RSR(uint8_t socket);
static void send_data_processing(uint8_t socket, uint16_t offset, const uint8_t *data, uint16_t length);
static void read_data(uint8_t s, uint16_t source, volatile uint8_t *destination, uint16_t length);



/*****************************************/
/*          Socket management            */
/*****************************************/


void socketPortRand(uint16_t n)
{
	n &= 0x3FFF;
	local_port ^= n;
	//Serial.printf("socketPortRand %d, srcport=%d\n", n, local_port);
}

uint8_t socketBegin(uint8_t protocol, uint16_t port)
{
	uint8_t s, status[MAX_SOCK_NUM];

	//Serial.printf("W5000socket begin, protocol=%d, port=%d\n", protocol, port);
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	// look at all the hardware sockets, use any that are closed (unused)
	for (s=0; s < MAX_SOCK_NUM; s++) {
		status[s] = W5100.readSnSR(s);
		if (status[s] == SnSR::CLOSED) goto makesocket;
	}
	//Serial.printf("W5000socket step2\n");
	// as a last resort, forcibly close any already closing
	for (s=0; s < MAX_SOCK_NUM; s++) {
		uint8_t stat = status[s];
		if (stat == SnSR::LAST_ACK) goto closemakesocket;
		if (stat == SnSR::TIME_WAIT) goto closemakesocket;
		if (stat == SnSR::FIN_WAIT) goto closemakesocket;
		if (stat == SnSR::CLOSING) goto closemakesocket;
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
	SPI.endTransaction();
	return MAX_SOCK_NUM; // all sockets are in use
closemakesocket:
	//Serial.printf("W5000socket close\n");
	W5100.execCmdSn(s, Sock_CLOSE);
makesocket:
	//Serial.printf("W5000socket %d\n", s);
	EthernetServer::server_port[s] = 0;
	delayMicroseconds(250); // TODO: is this needed??
	W5100.writeSnMR(s, protocol);
	W5100.writeSnIR(s, 0xFF);
	if (port > 0) {
		W5100.writeSnPORT(s, port);
	} else {
		// if don't set the source port, set local_port number.
		if (++local_port < 49152) local_port = 49152;
		W5100.writeSnPORT(s, local_port);
	}
	W5100.execCmdSn(s, Sock_OPEN);
	state[s].RX_RSR = 0;
	state[s].RX_RD  = W5100.readSnRX_RD(s); // always zero?
	state[s].RX_inc = 0;
	state[s].TX_FSR = 0;
	//Serial.printf("W5000socket prot=%d, RX_RD=%d\n", W5100.readSnMR(s), state[s].RX_RD);
	SPI.endTransaction();
	return s;
}

// Return the socket's status
//
uint8_t socketStatus(uint8_t socket)
{
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	uint8_t status = W5100.readSnSR(socket);
	SPI.endTransaction();
	return status;
}

// Immediately close.  If a TCP connection is established, the
// remote host is left unaware we closed.
//
void socketClose(uint8_t socket)
{
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.execCmdSn(socket, Sock_CLOSE);
	SPI.endTransaction();
}


// Place the socket in listening (server) mode
//
uint8_t socketListen(uint8_t socket)
{
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	if (W5100.readSnSR(socket) != SnSR::INIT) {
		SPI.endTransaction();
		return 0;
	}
	W5100.execCmdSn(socket, Sock_LISTEN);
	SPI.endTransaction();
	return 1;
}


// establish a TCP connection in Active (client) mode.
//
void socketConnect(uint8_t socket, uint8_t * address, uint16_t port)
{
	// set destination IP
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.writeSnDIPR(socket, address);
	W5100.writeSnDPORT(socket, port);
	W5100.execCmdSn(socket, Sock_CONNECT);
	SPI.endTransaction();
}



// Gracefully disconnect a TCP connection.
//
void socketDisconnect(uint8_t s)
{
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.execCmdSn(s, Sock_DISCON);
	SPI.endTransaction();
}



/*****************************************/
/*    Socket Data Receive Functions      */
/*****************************************/


static uint16_t getSnRX_RSR(uint8_t socket)
{
#if 1
        uint16_t val, prev;

		prev = W5100.readSnRX_RSR(socket);
        while (1) {
				val = W5100.readSnRX_RSR(socket);
                if (val == prev) {
			state[socket].RX_RSR = val;
			return val;
		}
                prev = val;
        }
#else
	uint16_t val = W5100.readSnRX_RSR(s);
	state[s].RX_RSR = val;
	return val;
#endif
}

static void read_data(uint8_t socket, uint16_t source, volatile uint8_t *destination, uint16_t length)
{
	uint16_t size;
	uint16_t src_mask;
	uint16_t src_ptr;

	//Serial.printf("read_data, len=%d, at:%d\n", len, src);
	src_mask = (uint16_t)source & W5100.SMASK;
	src_ptr = W5100.RBASE[socket] + src_mask;

	if ((src_mask + length) > W5100.SSIZE) {
		size = W5100.SSIZE - src_mask;
		W5100.read(src_ptr, (uint8_t *)destination, size);
		destination += size;
		W5100.read(W5100.RBASE[socket], (uint8_t *) destination, length - size);
	} else {
		W5100.read(src_ptr, (uint8_t *) destination, length);
	}
}

// Receive data.  Returns size, or -1 for no data, or 0 if connection closed
//
int socketRecv(uint8_t socket, uint8_t *buffer, int16_t length)
{
	// Check how much data is available
	int result = state[socket].RX_RSR;
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	if (result < length) {
		result = getSnRX_RSR(socket);
	}
	if (result == 0) {
		// No data available.
		uint8_t status = W5100.readSnSR(socket);
		if ( status == SnSR::LISTEN || status == SnSR::CLOSED ||
		  status == SnSR::CLOSE_WAIT ) {
			// The remote end has closed its side of the connection,
			// so this is the eof state
			result = 0;
		} else {
			// The connection is still up, but there's no data waiting to be read
			result = -1;
		}
	} else {
		if (result > length) result = length; // more data available than buffer length
		uint16_t ptr = state[socket].RX_RD;
		read_data(socket, ptr, buffer, result);
		ptr += length;
		state[socket].RX_RD = ptr;
		state[socket].RX_RSR -= result;
		uint16_t inc = state[socket].RX_inc + result;
		if (inc >= 80 || state[socket].RX_RSR == 0) {
			state[socket].RX_inc = 0;
			W5100.writeSnRX_RD(socket, ptr);
			W5100.execCmdSn(socket, Sock_RECV);
			//Serial.printf("Sock_RECV cmd, RX_RD=%d\n", state[s].RX_RD);
		} else {
			state[socket].RX_inc = inc;
		}
	}
	SPI.endTransaction();
	return result;
}

uint16_t socketRecvAvailable(uint8_t socket)
{
	uint16_t ret = state[socket].RX_RSR;
	if (ret == 0) {
		SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
		ret = getSnRX_RSR(socket);
		SPI.endTransaction();
	}
	//Serial.printf("sock.recvAvailable s=%d, num=%d\n", s, ret);
	return ret;
}

// get the first byte in the receive queue (no checking)
//
uint8_t socketPeek(uint8_t socket)
{
	uint8_t result;
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	uint16_t ptr = state[socket].RX_RD;
	W5100.read((ptr & W5100.SMASK) + W5100.RBASE[socket], &result, 1);
	SPI.endTransaction();
	return result;
}



/*****************************************/
/*    Socket Data Transmit Functions     */
/*****************************************/

static uint16_t getSnTX_FSR(uint8_t socket)
{
        uint16_t val, prev;

		prev = W5100.readSnTX_FSR(socket);
        while (1) {
				val = W5100.readSnTX_FSR(socket);
                if (val == prev) {
			state[socket].TX_FSR = val;
			return val;
		}
                prev = val;
        }
}


static void send_data_processing(uint8_t socket, uint16_t data_offset, const uint8_t *data, uint16_t length)
{
	uint16_t ptr = W5100.readSnTX_WR(socket);
	ptr += data_offset;
	uint16_t offset = ptr & W5100.SMASK;
	uint16_t dstAddr = offset + W5100.SBASE[socket];

	if (offset + length > W5100.SSIZE) {
		// Wrap around circular buffer
		uint16_t size = W5100.SSIZE - offset;
		W5100.write(dstAddr, data, size);
		W5100.write(W5100.SBASE[socket], data + size, length - size);
	} else {
		W5100.write(dstAddr, data, length);
	}
	ptr += length;
	W5100.writeSnTX_WR(socket, ptr);
}


/**
 * @brief	This function used to send the data in TCP mode
 * @return	1 for success else 0.
 */
uint16_t socketSend(uint8_t socket, const uint8_t * buffer, const uint16_t length)
{
	uint8_t status=0;
	uint16_t result=0;
	uint16_t freesize=0;

	if (length > W5100.SSIZE) {
		result = W5100.SSIZE; // check size not to exceed MAX size.
	} else {
		result = length;
	}

	// if freebuf is available, start.
	do {
		SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
		freesize = getSnTX_FSR(socket);
		status = W5100.readSnSR(socket);
		SPI.endTransaction();
		if ((status != SnSR::ESTABLISHED) && (status != SnSR::CLOSE_WAIT)) {
			result = 0;
			break;
		}
		yield();
	} while (freesize < result);

	// copy data
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	send_data_processing(socket, 0, (uint8_t *)buffer, result);
	W5100.execCmdSn(socket, Sock_SEND);

	/* +2008.01 bj */
	while ( (W5100.readSnIR(socket) & SnIR::SEND_OK) != SnIR::SEND_OK ) {
		/* m2008.01 [bj] : reduce code */
		if ( W5100.readSnSR(socket) == SnSR::CLOSED ) {
			SPI.endTransaction();
			return 0;
		}
		SPI.endTransaction();
		yield();
		SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	}
	/* +2008.01 bj */
	W5100.writeSnIR(socket, SnIR::SEND_OK);
	SPI.endTransaction();
	return result;
}


uint16_t socketBufferData(uint8_t socket, uint16_t offset, const uint8_t* buffer, uint16_t length)
{
	//Serial.printf("  bufferData, offset=%d, len=%d\n", offset, len);
	uint16_t result =0;
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	uint16_t txfree = getSnTX_FSR(socket);
	if (length > txfree) {
		result = txfree; // check size not to exceed MAX size.
	} else {
		result = length;
	}
	send_data_processing(socket, offset, buffer, result);
	SPI.endTransaction();
	return result;
}

int socketStartUDP(uint8_t socket, uint8_t* address, uint16_t port)
{
	if ( ((address[0] == 0x00) && (address[1] == 0x00) && (address[2] == 0x00) && (address[3] == 0x00)) ||
	  ((port == 0x00)) ) {
		return 0;
	}
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.writeSnDIPR(socket, address);
	W5100.writeSnDPORT(socket, port);
	SPI.endTransaction();
	return 1;
}

int socketSendUDP(uint8_t socket)
{
	SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	W5100.execCmdSn(socket, Sock_SEND);

	/* +2008.01 bj */
	while ( (W5100.readSnIR(socket) & SnIR::SEND_OK) != SnIR::SEND_OK ) {
		if (W5100.readSnIR(socket) & SnIR::TIMEOUT) {
			/* +2008.01 [bj]: clear interrupt */
			W5100.writeSnIR(socket, (SnIR::SEND_OK|SnIR::TIMEOUT));
			SPI.endTransaction();
			//Serial.printf("sendUDP timeout\n");
			return 0;
		}
		SPI.endTransaction();
		yield();
		SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
	}

	/* +2008.01 bj */
	W5100.writeSnIR(socket, SnIR::SEND_OK);
	SPI.endTransaction();

	//Serial.printf("sendUDP ok\n");
	/* Sent ok */
	return 1;
}

