/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef	W5100_H_INCLUDED
#define	W5100_H_INCLUDED

#include <avr/pgmspace.h>
#include <SPI.h>

#define SPI_ETHERNET_SETTINGS SPISettings(14000000, MSBFIRST, SPI_MODE0)
//#define SPI_ETHERNET_SETTINGS SPISettings(8000000, MSBFIRST, SPI_MODE0)
//#define SPI_ETHERNET_SETTINGS SPISettings(4000000, MSBFIRST, SPI_MODE0)

#define MAX_SOCK_NUM 4

typedef uint8_t SOCKET;

class SnMR {
public:
  static const uint8_t CLOSE  = 0x00;
  static const uint8_t TCP    = 0x01;
  static const uint8_t UDP    = 0x02;
  static const uint8_t IPRAW  = 0x03;
  static const uint8_t MACRAW = 0x04;
  static const uint8_t PPPOE  = 0x05;
  static const uint8_t ND     = 0x20;
  static const uint8_t MULTI  = 0x80;
};

enum SockCMD {
  Sock_OPEN      = 0x01,
  Sock_LISTEN    = 0x02,
  Sock_CONNECT   = 0x04,
  Sock_DISCON    = 0x08,
  Sock_CLOSE     = 0x10,
  Sock_SEND      = 0x20,
  Sock_SEND_MAC  = 0x21,
  Sock_SEND_KEEP = 0x22,
  Sock_RECV      = 0x40
};

class SnIR {
public:
  static const uint8_t SEND_OK = 0x10;
  static const uint8_t TIMEOUT = 0x08;
  static const uint8_t RECV    = 0x04;
  static const uint8_t DISCON  = 0x02;
  static const uint8_t CON     = 0x01;
};

class SnSR {
public:
  static const uint8_t CLOSED      = 0x00;
  static const uint8_t INIT        = 0x13;
  static const uint8_t LISTEN      = 0x14;
  static const uint8_t SYNSENT     = 0x15;
  static const uint8_t SYNRECV     = 0x16;
  static const uint8_t ESTABLISHED = 0x17;
  static const uint8_t FIN_WAIT    = 0x18;
  static const uint8_t CLOSING     = 0x1A;
  static const uint8_t TIME_WAIT   = 0x1B;
  static const uint8_t CLOSE_WAIT  = 0x1C;
  static const uint8_t LAST_ACK    = 0x1D;
  static const uint8_t UDP         = 0x22;
  static const uint8_t IPRAW       = 0x32;
  static const uint8_t MACRAW      = 0x42;
  static const uint8_t PPPOE       = 0x5F;
};

class IPPROTO {
public:
  static const uint8_t IP   = 0;
  static const uint8_t ICMP = 1;
  static const uint8_t IGMP = 2;
  static const uint8_t GGP  = 3;
  static const uint8_t TCP  = 6;
  static const uint8_t PUP  = 12;
  static const uint8_t UDP  = 17;
  static const uint8_t IDP  = 22;
  static const uint8_t ND   = 77;
  static const uint8_t RAW  = 255;
};

class W5100Class {

public:
  static uint8_t init(void);

  /**
   * @brief	This function is being used for copy the data form Receive buffer of the chip to application buffer.
   * 
   * It calculate the actual physical address where one has to read
   * the data from Receive buffer. Here also take care of the condition while it exceed
   * the Rx memory uper-bound of socket.
   */
  static void read_data(SOCKET s, uint16_t src, volatile uint8_t * dst, uint16_t len);
  
  /**
   * @brief	 This function is being called by send() and sendto() function also. 
   * 
   * This function read the Tx write pointer register and after copy the data in buffer update the Tx write pointer
   * register. User should read upper byte first and lower byte later to get proper value.
   */
  static void send_data_processing(SOCKET s, const uint8_t *data, uint16_t len);
  /**
   * @brief A copy of send_data_processing that uses the provided ptr for the
   *        write offset.  Only needed for the "streaming" UDP API, where
   *        a single UDP packet is built up over a number of calls to
   *        send_data_processing_ptr, because TX_WR doesn't seem to get updated
   *        correctly in those scenarios
   * @param ptr value to use in place of TX_WR.  If 0, then the value is read
   *        in from TX_WR
   * @return New value for ptr, to be used in the next call
   */
// FIXME Update documentation
  static void send_data_processing_offset(SOCKET s, uint16_t data_offset, const uint8_t *data, uint16_t len);

  /**
   * @brief	This function is being called by recv() also.
   * 
   * This function read the Rx read pointer register
   * and after copy the data from receive buffer update the Rx write pointer register.
   * User should read upper byte first and lower byte later to get proper value.
   */
  static void recv_data_processing(SOCKET s, uint8_t *data, uint16_t len, uint8_t peek = 0);

  inline void setGatewayIp(uint8_t * addr) { writeGAR(addr); }
  inline void getGatewayIp(uint8_t * addr) { readGAR(addr); }

  inline void setSubnetMask(uint8_t * addr) { writeSUBR(addr); }
  inline void getSubnetMask(uint8_t * addr) { readSUBR(addr); }

  inline void setMACAddress(uint8_t * addr) { writeSHAR(addr); }
  inline void getMACAddress(uint8_t * addr) { readSHAR(addr); }

  inline void setIPAddress(uint8_t * addr) { writeSIPR(addr); }
  inline void getIPAddress(uint8_t * addr) { readSIPR(addr); }

  inline void setRetransmissionTime(uint16_t timeout) { writeRTR(timeout); }
  inline void setRetransmissionCount(uint8_t retry) { writeRCR(retry); }

  static void execCmdSn(SOCKET s, SockCMD _cmd);
  
  static uint16_t getTXFreeSize(SOCKET s);
  static uint16_t getRXReceivedSize(SOCKET s);
  

  // W5100 Registers
  // ---------------
private:
  static uint16_t write(uint16_t addr, const uint8_t *buf, uint16_t len);
  static uint8_t write(uint16_t addr, uint8_t data) {
    return write(addr, &data, 1);
  }
  static uint16_t read(uint16_t addr, uint8_t *buf, uint16_t len);
  static uint8_t read(uint16_t addr) {
    uint8_t data;
    read(addr, &data, 1);
    return data;
  }
  
#define __GP_REGISTER8(name, address)             \
  static inline void write##name(uint8_t _data) { \
    write(address, _data);                        \
  }                                               \
  static inline uint8_t read##name() {            \
    return read(address);                         \
  }
#define __GP_REGISTER16(name, address)            \
  static void write##name(uint16_t _data) {       \
    uint8_t buf[2];                               \
    buf[0] = _data >> 8;                          \
    buf[1] = _data & 0xFF;                        \
    write(address, buf, 2);                       \
  }                                               \
  static uint16_t read##name() {                  \
    uint8_t buf[2];                               \
    read(address, buf, 2);                        \
    return (buf[0] << 8) | buf[1];                \
  }
#define __GP_REGISTER_N(name, address, size)      \
  static uint16_t write##name(uint8_t *_buff) {   \
    return write(address, _buff, size);           \
  }                                               \
  static uint16_t read##name(uint8_t *_buff) {    \
    return read(address, _buff, size);            \
  }

public:
  __GP_REGISTER8 (MR,     0x0000);    // Mode
  __GP_REGISTER_N(GAR,    0x0001, 4); // Gateway IP address
  __GP_REGISTER_N(SUBR,   0x0005, 4); // Subnet mask address
  __GP_REGISTER_N(SHAR,   0x0009, 6); // Source MAC address
  __GP_REGISTER_N(SIPR,   0x000F, 4); // Source IP address
  __GP_REGISTER8 (IR,     0x0015);    // Interrupt
  __GP_REGISTER8 (IMR,    0x0016);    // Interrupt Mask
  __GP_REGISTER16(RTR,    0x0017);    // Timeout address
  __GP_REGISTER8 (RCR,    0x0019);    // Retry count
  __GP_REGISTER8 (RMSR,   0x001A);    // Receive memory size (W5100 only)
  __GP_REGISTER8 (TMSR,   0x001B);    // Transmit memory size (W5100 only)
  __GP_REGISTER8 (PATR,   0x001C);    // Authentication type address in PPPoE mode
  __GP_REGISTER8 (PTIMER, 0x0028);    // PPP LCP Request Timer
  __GP_REGISTER8 (PMAGIC, 0x0029);    // PPP LCP Magic Number
  __GP_REGISTER_N(UIPR,   0x002A, 4); // Unreachable IP address in UDP mode (W5100 only)
  __GP_REGISTER16(UPORT,  0x002E);    // Unreachable Port address in UDP mode (W5100 only)
  
#undef __GP_REGISTER8
#undef __GP_REGISTER16
#undef __GP_REGISTER_N

  // W5100 Socket registers
  // ----------------------
private:
  static inline uint8_t readSn(SOCKET s, uint16_t addr) {
    return read(CH_BASE + s * CH_SIZE + addr);
  }
  static inline uint8_t writeSn(SOCKET s, uint16_t addr, uint8_t data) {
    return write(CH_BASE + s * CH_SIZE + addr, data);
  }
  static inline uint16_t readSn(SOCKET s, uint16_t addr, uint8_t *buf, uint16_t len) {
    return read(CH_BASE + s * CH_SIZE + addr, buf, len);
  }
  static inline uint16_t writeSn(SOCKET s, uint16_t addr, uint8_t *buf, uint16_t len) {
    return write(CH_BASE + s * CH_SIZE + addr, buf, len);
  }

  static uint16_t CH_BASE;
  static const uint16_t CH_SIZE = 0x0100;

#define __SOCKET_REGISTER8(name, address)                    \
  static inline void write##name(SOCKET _s, uint8_t _data) { \
    writeSn(_s, address, _data);                             \
  }                                                          \
  static inline uint8_t read##name(SOCKET _s) {              \
    return readSn(_s, address);                              \
  }
#define __SOCKET_REGISTER16(name, address)                   \
  static void write##name(SOCKET _s, uint16_t _data) {       \
    uint8_t buf[2];                                          \
    buf[0] = _data >> 8;                                     \
    buf[1] = _data & 0xFF;                                   \
    writeSn(_s, address, buf, 2);                            \
  }                                                          \
  static uint16_t read##name(SOCKET _s) {                    \
    uint8_t buf[2];                                          \
    readSn(_s, address, buf, 2);                             \
    return (buf[0] << 8) | buf[1];                           \
  }
#define __SOCKET_REGISTER_N(name, address, size)             \
  static uint16_t write##name(SOCKET _s, uint8_t *_buff) {   \
    return writeSn(_s, address, _buff, size);                \
  }                                                          \
  static uint16_t read##name(SOCKET _s, uint8_t *_buff) {    \
    return readSn(_s, address, _buff, size);                 \
  }
  
public:
  __SOCKET_REGISTER8(SnMR,        0x0000)        // Mode
  __SOCKET_REGISTER8(SnCR,        0x0001)        // Command
  __SOCKET_REGISTER8(SnIR,        0x0002)        // Interrupt
  __SOCKET_REGISTER8(SnSR,        0x0003)        // Status
  __SOCKET_REGISTER16(SnPORT,     0x0004)        // Source Port
  __SOCKET_REGISTER_N(SnDHAR,     0x0006, 6)     // Destination Hardw Addr
  __SOCKET_REGISTER_N(SnDIPR,     0x000C, 4)     // Destination IP Addr
  __SOCKET_REGISTER16(SnDPORT,    0x0010)        // Destination Port
  __SOCKET_REGISTER16(SnMSSR,     0x0012)        // Max Segment Size
  __SOCKET_REGISTER8(SnPROTO,     0x0014)        // Protocol in IP RAW Mode
  __SOCKET_REGISTER8(SnTOS,       0x0015)        // IP TOS
  __SOCKET_REGISTER8(SnTTL,       0x0016)        // IP TTL
  __SOCKET_REGISTER8(SnRX_SIZE,   0x001E)        // RX Memory Size (W5200 only)
  __SOCKET_REGISTER8(SnTX_SIZE,   0x001F)        // RX Memory Size (W5200 only)
  __SOCKET_REGISTER16(SnTX_FSR,   0x0020)        // TX Free Size
  __SOCKET_REGISTER16(SnTX_RD,    0x0022)        // TX Read Pointer
  __SOCKET_REGISTER16(SnTX_WR,    0x0024)        // TX Write Pointer
  __SOCKET_REGISTER16(SnRX_RSR,   0x0026)        // RX Free Size
  __SOCKET_REGISTER16(SnRX_RD,    0x0028)        // RX Read Pointer
  __SOCKET_REGISTER16(SnRX_WR,    0x002A)        // RX Write Pointer (supported?)
  
#undef __SOCKET_REGISTER8
#undef __SOCKET_REGISTER16
#undef __SOCKET_REGISTER_N


private:
  static uint8_t chip;
  static void reset(void);
  static uint8_t isW5100(void);
  static uint8_t isW5200(void);

  static const uint8_t  RST = 7; // Reset BIT

  static const int SOCKETS = 4;
  static uint16_t SMASK;
public:
  static uint16_t SSIZE;
private:
  //receive and transmit have same buffer sizes
  static uint16_t SBASE[SOCKETS]; // Tx buffer base address
  static uint16_t RBASE[SOCKETS]; // Rx buffer base address

private:
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  inline static void initSS()    { DDRB  |=  _BV(4); };
  inline static void setSS()     { PORTB &= ~_BV(4); };
  inline static void resetSS()   { PORTB |=  _BV(4); };
#elif defined(__AVR_ATmega32U4__) && defined(CORE_TEENSY)
  inline static void initSS()    { DDRB  |=  _BV(0); };
  inline static void setSS()     { PORTB &= ~_BV(0); };
  inline static void resetSS()   { PORTB |=  _BV(0); }; 
#elif defined(__AVR_ATmega32U4__)
  inline static void initSS()    { DDRB  |=  _BV(6); };
  inline static void setSS()     { PORTB &= ~_BV(6); };
  inline static void resetSS()   { PORTB |=  _BV(6); }; 
#elif defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB162__)
  inline static void initSS()    { DDRB  |=  _BV(0); };
  inline static void setSS()     { PORTB &= ~_BV(0); };
  inline static void resetSS()   { PORTB |=  _BV(0); }; 
#elif defined(__MK20DX128__) || defined(__MK20DX256__)
  inline static void initSS()    { pinMode(10, OUTPUT); };
  inline static void setSS()     { digitalWriteFast(10, LOW); };
  inline static void resetSS()   { digitalWriteFast(10, HIGH); };
#else
  inline static void initSS()    { DDRB  |=  _BV(2); };
  inline static void setSS()     { PORTB &= ~_BV(2); };
  inline static void resetSS()   { PORTB |=  _BV(2); };
#endif

};

extern W5100Class W5100;



#endif
