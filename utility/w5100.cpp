/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>

#if defined(__arm__)
#include "SPIFIFO.h"
#ifdef  HAS_SPIFIFO
#define USE_SPIFIFO
#endif
#endif

// The W5200 really does require a proper reset pulse!
// Its SPI state machine remembers the previously started
// burst transfer, even after SS is deasserted.  Wiznet's
// documentation does not mention this very unfortunate
// fact, which means you to really must reset the chip if
// it may have ever heard an partial transfer (eg, from a
// previous run before clicking Upload in Arduino) or if
// its SS and SCK pins are ever left floating.
#define W5200_RESET_PIN  9
#define W5200_SS_PIN    10

#include "Arduino.h"

#include "w5100.h"

// W5100 controller instance
uint16_t W5100Class::SBASE[MAX_SOCK_NUM];
uint16_t W5100Class::RBASE[MAX_SOCK_NUM];
uint16_t W5100Class::CH_BASE;
uint16_t W5100Class::SSIZE;
uint16_t W5100Class::SMASK;
uint8_t  W5100Class::chip;
W5100Class W5100;


uint8_t W5100Class::init(void)
{
  uint16_t TXBUF_BASE, RXBUF_BASE;
  uint8_t i;

  delay(200);
  //Serial.println("w5100 init");

#ifdef USE_SPIFIFO
  SPI.begin();
  SPIFIFO.begin(W5200_SS_PIN, SPI_CLOCK_12MHz);  // W5100 is 14 MHz max
#else
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  initSS();
#endif
  
  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  if (isW5100()) {
    CH_BASE = 0x0400;
    SSIZE = 2048;
    SMASK = 0x07FF;
    TXBUF_BASE = 0x4000;
    RXBUF_BASE = 0x6000;
    writeTMSR(0x55);
    writeRMSR(0x55);

  } else if (isW5200()) {
    CH_BASE = 0x4000;
    SSIZE = 4096;
    SMASK = 0x0FFF;
    TXBUF_BASE = 0x8000;
    RXBUF_BASE = 0xC000;
    for (i=0; i<MAX_SOCK_NUM; i++) {
      writeSnRX_SIZE(i, SSIZE >> 10);
      writeSnTX_SIZE(i, SSIZE >> 10);
    }
    for (; i<8; i++) {
      writeSnRX_SIZE(i, 0);
      writeSnTX_SIZE(i, 0);
    }

  } else {
    //Serial.println("no chip :-(");
    chip = 0;
    SPI.endTransaction();
    return 0; // no known chip is responding :-(
  }
  for (int i=0; i<MAX_SOCK_NUM; i++) {
    SBASE[i] = TXBUF_BASE + SSIZE * i;
    RBASE[i] = RXBUF_BASE + SSIZE * i;
  }
  SPI.endTransaction();
  return 1; // successful init
}

void W5100Class::reset(void)
{
  uint16_t count=0;

  //Serial.println("W5100 reset");
  writeMR(1<<RST);
  while (++count < 20) {
    uint8_t mr = readMR();
    //Serial.print("mr=");
    //Serial.println(mr, HEX);
    if (mr == 0) break;
    delay(1);
  }
}

uint8_t W5100Class::isW5100(void)
{
  chip = 51;
  //Serial.println("W5100 detect W5100 chip");
  reset();
  writeMR(0x10);
  if (readMR() != 0x10) return 0;
  writeMR(0x12);
  if (readMR() != 0x12) return 0;
  writeMR(0x00);
  if (readMR() != 0x00) return 0;
  //Serial.println("chip is W5100");
  return 1;
}

uint8_t W5100Class::isW5200(void)
{
  uint8_t mr;
  chip = 52;
  //Serial.println("W5100 detect W5200 chip");
#ifdef W5200_RESET_PIN
  pinMode(W5200_RESET_PIN, OUTPUT);
  digitalWrite(W5200_RESET_PIN, LOW);
  delay(1);
  digitalWrite(W5200_RESET_PIN, HIGH);
  delay(150);
#endif
  reset();
  writeMR(0x08);
  mr = readMR();
  //Serial.print("mr=");
  //Serial.println(mr, HEX);
  if (mr != 0x08) return 0;
  writeMR(0x10);
  mr = readMR();
  //Serial.print("mr=");
  //Serial.println(mr, HEX);
  if (mr != 0x10) return 0;
  writeMR(0x00);
  mr = readMR();
  //Serial.print("mr=");
  //Serial.println(mr, HEX);
  if (mr != 0x00) return 0;
  //Serial.println("chip is W5200");
  return 1;
}




uint16_t W5100Class::getTXFreeSize(SOCKET s)
{
  uint16_t val=0, val1=0;
  do {
    val1 = readSnTX_FSR(s);
    if (val1 != 0)
      val = readSnTX_FSR(s);
  } 
  while (val != val1);
  return val;
}

uint16_t W5100Class::getRXReceivedSize(SOCKET s)
{
  uint16_t val=0,val1=0;
  do {
    val1 = readSnRX_RSR(s);
    if (val1 != 0)
      val = readSnRX_RSR(s);
  } 
  while (val != val1);
  return val;
}


void W5100Class::send_data_processing(SOCKET s, const uint8_t *data, uint16_t len)
{
  // This is same as having no offset in a call to send_data_processing_offset
  send_data_processing_offset(s, 0, data, len);
}

void W5100Class::send_data_processing_offset(SOCKET s, uint16_t data_offset, const uint8_t *data, uint16_t len)
{
  uint16_t ptr = readSnTX_WR(s);
  ptr += data_offset;
  uint16_t offset = ptr & SMASK;
  uint16_t dstAddr = offset + SBASE[s];

  if (offset + len > SSIZE) 
  {
    // Wrap around circular buffer
    uint16_t size = SSIZE - offset;
    write(dstAddr, data, size);
    write(SBASE[s], data + size, len - size);
  } 
  else {
    write(dstAddr, data, len);
  }

  ptr += len;
  writeSnTX_WR(s, ptr);
}


void W5100Class::recv_data_processing(SOCKET s, uint8_t *data, uint16_t len, uint8_t peek)
{
  uint16_t ptr;
  ptr = readSnRX_RD(s);
  read_data(s, ptr, data, len);
  if (!peek)
  {
    ptr += len;
    writeSnRX_RD(s, ptr);
  }
}

void W5100Class::read_data(SOCKET s, uint16_t src, volatile uint8_t *dst, uint16_t len)
{
  uint16_t size;
  uint16_t src_mask;
  uint16_t src_ptr;

  src_mask = (uint16_t)src & SMASK;
  src_ptr = RBASE[s] + src_mask;

  if( (src_mask + len) > SSIZE ) 
  {
    size = SSIZE - src_mask;
    read(src_ptr, (uint8_t *)dst, size);
    dst += size;
    read(RBASE[s], (uint8_t *) dst, len - size);
  } 
  else
    read(src_ptr, (uint8_t *) dst, len);
}


#ifdef USE_SPIFIFO
uint16_t W5100Class::write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
  uint32_t i;

  if (chip == 51) {
    for (i=0; i<len; i++) {
	SPIFIFO.write16(0xF000 | (addr >> 8), SPI_CONTINUE);
	SPIFIFO.write16((addr << 8) | buf[i]);
	addr++;
	SPIFIFO.read();
	SPIFIFO.read();
    }
  } else {
	SPIFIFO.clear();
	SPIFIFO.write16(addr, SPI_CONTINUE);
	SPIFIFO.write16(len | 0x8000, SPI_CONTINUE);
	for (i=0; i<len; i++) {
		SPIFIFO.write(buf[i], ((i+1<len) ? SPI_CONTINUE : 0));
		SPIFIFO.read();
	}
	SPIFIFO.read();
	SPIFIFO.read();
  }
  return len;
}
#else
uint16_t W5100Class::write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
  if (chip == 51) {
    for (uint16_t i=0; i<len; i++) {
      setSS();
      SPI.transfer(0xF0);
      SPI.transfer(addr >> 8);
      SPI.transfer(addr & 0xFF);
      addr++;
      SPI.transfer(buf[i]);
      resetSS();
    }
  } else {
    setSS();
    SPI.transfer(addr >> 8);
    SPI.transfer(addr & 0xFF);
    SPI.transfer(((len >> 8) & 0x7F) | 0x80);
    SPI.transfer(len & 0xFF);
    for (uint16_t i=0; i<len; i++) {
      SPI.transfer(buf[i]);
    }
    resetSS();
  }
  return len;
}
#endif


#ifdef USE_SPIFIFO
uint16_t W5100Class::read(uint16_t addr, uint8_t *buf, uint16_t len)
{
  uint32_t i;

  if (chip == 51) {
    for (i=0; i<len; i++) {
	#if 1
	SPIFIFO.write(0x0F, SPI_CONTINUE);
	SPIFIFO.write16(addr, SPI_CONTINUE);
	addr++;
	SPIFIFO.read();
	SPIFIFO.write(0);
	SPIFIFO.read();
	buf[i] = SPIFIFO.read();
	#endif
	#if 0
	// this does not work, but why?
	SPIFIFO.write16(0x0F00 | (addr >> 8), SPI_CONTINUE);
	SPIFIFO.write16(addr << 8);
	addr++;
	SPIFIFO.read();
	buf[i] = SPIFIFO.read();
	#endif
    }
  } else {
	// len = 1:  write header, write 1 byte, read
	// len = 2:  write header, write 2 byte, read
	// len = 3,5,7
	SPIFIFO.clear();
	SPIFIFO.write16(addr, SPI_CONTINUE);
	SPIFIFO.write16(len & 0x7FFF, SPI_CONTINUE);
	SPIFIFO.read();
	if (len == 1) {
		// read only 1 byte
		SPIFIFO.write(0);
		SPIFIFO.read();
		*buf = SPIFIFO.read();
	} else if (len == 2) {
		// read only 2 bytes
		SPIFIFO.write16(0);
		SPIFIFO.read();
		uint32_t val = SPIFIFO.read();
		*buf++ = val >> 8;
		*buf = val;
	} else if ((len & 1)) {
		// read 3 or more, odd length
  		//Serial.print("W5200 read, len=");
		//Serial.println(len);
		uint32_t count = len / 2;
		SPIFIFO.write16(0, SPI_CONTINUE);
		SPIFIFO.read();
		do {
			if (count > 1) SPIFIFO.write16(0, SPI_CONTINUE);
			else SPIFIFO.write(0);
			uint32_t val = SPIFIFO.read();
			//TODO: WebClient_speedtest with READSIZE 7 is
			//dramatically faster with this Serial.print(),
			//and the 2 above, but not without both.  Why?!
			//Serial.println(val, HEX);
			*buf++ = val >> 8;
			*buf++ = val;
		} while (--count > 0);
		*buf = SPIFIFO.read();
		//Serial.println(*buf, HEX);
	} else {
		// read 4 or more, odd length
  		//Serial.print("W5200 read, len=");
		//Serial.println(len);
		uint32_t count = len / 2 - 1;
		SPIFIFO.write16(0, SPI_CONTINUE);
		SPIFIFO.read();
		do {
			SPIFIFO.write16(0, (count > 0) ? SPI_CONTINUE : 0);
			uint32_t val = SPIFIFO.read();
			*buf++ = val >> 8;
			*buf++ = val;
		} while (--count > 0);
		uint32_t val = SPIFIFO.read();
		*buf++ = val >> 8;
		*buf++ = val;
	}
  }
  return len;
}
#else
uint16_t W5100Class::read(uint16_t addr, uint8_t *buf, uint16_t len)
{
  if (chip == 51) {
    for (uint16_t i=0; i<len; i++) {
      setSS();
      SPI.transfer(0x0F);
      SPI.transfer(addr >> 8);
      SPI.transfer(addr & 0xFF);
      addr++;
      buf[i] = SPI.transfer(0);
      resetSS();
    }
  } else {
    setSS();
    SPI.transfer(addr >> 8);
    SPI.transfer(addr & 0xFF);
    SPI.transfer((len >> 8) & 0x7F);
    SPI.transfer(len & 0xFF);
    for (uint16_t i=0; i<len; i++) {
      buf[i] = SPI.transfer(0);
    }
    resetSS();
  }
  return len;
}
#endif

void W5100Class::execCmdSn(SOCKET s, SockCMD _cmd) {
  // Send command to socket
  writeSnCR(s, _cmd);
  // Wait for command to complete
  while (readSnCR(s))
    ;
}
