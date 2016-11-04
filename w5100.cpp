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
  Serial.println("w5100 init");

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

  } else if (isW5500()) {
    CH_BASE = 0x1000;
    SSIZE = 2048;
    SMASK = 0x07FF;
    TXBUF_BASE = 0x8000;
    RXBUF_BASE = 0xC000;

  } else {
    Serial.println("no chip :-(");
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

  Serial.println("W5100 reset");
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
  Serial.println("w5100.cpp: detect W5100 chip");
  reset();
  writeMR(0x10);
  if (readMR() != 0x10) return 0;
  writeMR(0x12);
  if (readMR() != 0x12) return 0;
  writeMR(0x00);
  if (readMR() != 0x00) return 0;
  Serial.println("chip is W5100");
  return 1;
}

uint8_t W5100Class::isW5200(void)
{
  uint8_t mr;
  chip = 52;
  Serial.println("w5100.cpp: detect W5200 chip");
#ifdef W5200_RESET_PIN
  pinMode(W5200_RESET_PIN, OUTPUT);
  digitalWrite(W5200_RESET_PIN, LOW);
  delay(1);
  digitalWrite(W5200_RESET_PIN, HIGH);
  delay(450);
#endif
  reset();
  writeMR(0x08);
  mr = readMR();
  Serial.print("mr=");
  Serial.println(mr, HEX);
  if (mr != 0x08) return 0;
  writeMR(0x10);
  mr = readMR();
  Serial.print("mr=");
  Serial.println(mr, HEX);
  if (mr != 0x10) return 0;
  writeMR(0x00);
  mr = readMR();
  Serial.print("mr=");
  Serial.println(mr, HEX);
  if (mr != 0x00) return 0;
  Serial.println("chip is W5200");
  return 1;
}

uint8_t W5100Class::isW5500(void)
{
  uint8_t mr, count=0;

  chip = 55;
  Serial.println("w5100.cpp: detect W5500 chip");
  reset();
  writeMR(0x08);
  mr = readMR();
  Serial.print("mr=");
  Serial.println(mr, HEX);
  if (mr != 0x08) return 0;
  writeMR(0x10);
  mr = readMR();
  Serial.print("mr=");
  Serial.println(mr, HEX);
  if (mr != 0x10) return 0;
  writeMR(0x00);
  mr = readMR();
  Serial.print("mr=");
  Serial.println(mr, HEX);
  if (mr != 0x00) return 0;
  writeMR(0x80);
  while (1) {
    if (readMR() == 0) break;
    if (++count > 100) return 0;
  }
  Serial.print("count=");
  Serial.println(count);
  Serial.println("chip is W5500");
  return 1;
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
  } else if (chip == 52) {
	SPIFIFO.clear();
	SPIFIFO.write16(addr, SPI_CONTINUE);
	SPIFIFO.write16(len | 0x8000, SPI_CONTINUE);
	for (i=0; i<len; i++) {
		SPIFIFO.write(buf[i], ((i+1<len) ? SPI_CONTINUE : 0));
		SPIFIFO.read();
	}
	SPIFIFO.read();
	SPIFIFO.read();
  } else {
	//SPIFIFO.clear();
	SPIFIFO.write16(addr & 0x03FF, SPI_CONTINUE);
	if (addr < 0x100) {
		// common registers 00nn
		SPIFIFO.write16(0x0400 | *buf++,
			((len > 1) ? SPI_CONTINUE : 0));
	} else if (addr < 0x8000) {
		// socket registers  10nn, 11nn, 12nn, 13nn, etc
		SPIFIFO.write16(((addr << 5) & 0xE000) | 0x0C00 | *buf++,
			((len > 1) ? SPI_CONTINUE : 0));
	} else if (addr < 0xC000) {
		// transmit buffers  8000-87FF, 8800-8FFF, 9000-97FF, etc
		SPIFIFO.write16(((addr << 2) & 0xE000) | 0x1400 | *buf++,
			((len > 1) ? SPI_CONTINUE : 0));
	} else {
		// receive buffers
		SPIFIFO.write16(((addr << 2) & 0xE000) | 0x1C00 | *buf++,
			((len > 1) ? SPI_CONTINUE : 0));
	}
#if 0
	SPIFIFO.read();
	do {
		if (len > 1) SPIFIFO.write(*buf++, (len > 2) ? SPI_CONTINUE : 0);
		SPIFIFO.read();
		len--;
	} while (len > 0);
#else
	len--;
	while (len >= 2) {
		len -= 2;
		SPIFIFO.write16((*buf << 8) | *(buf+1), (len == 0) ? 0 : SPI_CONTINUE);
		buf += 2;
		SPIFIFO.read();
	}
	if (len) {
		SPIFIFO.write(*buf);
		SPIFIFO.read();
	}
	SPIFIFO.read();
	SPIFIFO.read();
#endif
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
  } else if (chip == 52) {
    setSS();
    SPI.transfer(addr >> 8);
    SPI.transfer(addr & 0xFF);
    SPI.transfer(((len >> 8) & 0x7F) | 0x80);
    SPI.transfer(len & 0xFF);
    for (uint16_t i=0; i<len; i++) {
      SPI.transfer(buf[i]);
    }
    resetSS();
  } else {
    setSS();
    if (addr < 0x100) {
      // common registers 00nn
      SPI.transfer(0);
      SPI.transfer(addr & 0xFF);
      SPI.transfer(0x04);
    } else if (addr < 0x8000) {
      // socket registers  10nn, 11nn, 12nn, 13nn, etc
      SPI.transfer(0);
      SPI.transfer(addr & 0xFF);
      SPI.transfer(((addr >> 3) & 0xE0) | 0x0C);
    } else if (addr < 0xC000) {
      // transmit buffers  8000-87FF, 8800-8FFF, 9000-97FF, etc
      //  10## #nnn nnnn nnnn
      SPI.transfer((addr >> 8) & 0x03);
      SPI.transfer(addr & 0xFF);
      SPI.transfer(((addr >> 6) & 0xE0) | 0x14);
    } else {
      // receive buffers
      SPI.transfer((addr >> 8) & 0x03);
      SPI.transfer(addr & 0xFF);
      SPI.transfer(((addr >> 6) & 0xE0) | 0x1C);
    }
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
  } else if (chip == 52) {
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
		// read 4 or more, even length
  		//Serial.print("W5200 read, len=");
		//Serial.println(len);
		uint32_t count = len / 2 - 1;
		SPIFIFO.write16(0, SPI_CONTINUE);
		SPIFIFO.read();
		do {
			SPIFIFO.write16(0, (count > 1) ? SPI_CONTINUE : 0);
			uint32_t val = SPIFIFO.read();
			*buf++ = val >> 8;
			*buf++ = val;
		} while (--count > 0);
		uint32_t val = SPIFIFO.read();
		*buf++ = val >> 8;
		*buf++ = val;
	}
  } else {
	//SPIFIFO.clear();
	SPIFIFO.write16(addr & 0x03FF, SPI_CONTINUE);
	if (addr < 0x100) {
		// common registers 00nn
		SPIFIFO.write16(0,
			((len > 1) ? SPI_CONTINUE : 0));
	} else if (addr < 0x8000) {
		// socket registers  10nn, 11nn, 12nn, 13nn, etc
		SPIFIFO.write16(((addr << 5) & 0xE000) | 0x0800,
			((len > 1) ? SPI_CONTINUE : 0));
	} else if (addr < 0xC000) {
		// transmit buffers  8000-87FF, 8800-8FFF, 9000-97FF, etc
		SPIFIFO.write16(((addr << 2) & 0xE000) | 0x1000,
			((len > 1) ? SPI_CONTINUE : 0));
	} else {
		// receive buffers
		SPIFIFO.write16(((addr << 2) & 0xE000) | 0x1800,
			((len > 1) ? SPI_CONTINUE : 0));
	}
	SPIFIFO.read();
#if 0
	do {
		if (len > 1) SPIFIFO.write(0, (len > 2) ? SPI_CONTINUE : 0);
		*buf++ = SPIFIFO.read();
		len--;
	} while (len > 0);
#else
	if (len <= 1) {
		*buf++ = SPIFIFO.read();
	} else if (len == 2) {
		SPIFIFO.write(0);
		*buf++ = SPIFIFO.read();
		*buf++ = SPIFIFO.read();
	} else if (len & 1) {
		#if 1
		uint32_t count = len >> 1;
		SPIFIFO.write16(0, (count > 1) ? SPI_CONTINUE : 0);
		*buf++ = SPIFIFO.read();
		while (count > 1) {
			count--;
			SPIFIFO.write16(0, (count > 1) ? SPI_CONTINUE : 0);
			uint32_t val = SPIFIFO.read();
			*buf++ = val >> 8;
			*buf++ = val;
		}
		uint32_t val = SPIFIFO.read();
		*buf++ = val >> 8;
		*buf++ = val;
		#else
		do {
			if (len > 1) SPIFIFO.write(0, (len > 2) ? SPI_CONTINUE : 0);
			*buf++ = SPIFIFO.read();
			len--;
		} while (len > 0);
		#endif
	} else {
		#if 1
		SPIFIFO.write16(0, SPI_CONTINUE);
		*buf++ = SPIFIFO.read();
		uint32_t count = len >> 1;
		while (count > 1) {
			count--;
			if (count > 1) {
				SPIFIFO.write16(0, SPI_CONTINUE);
			} else {
				SPIFIFO.write(0, 0);
			}
			uint32_t val = SPIFIFO.read();
			*buf++ = val >> 8;
			*buf++ = val;
		}
		*buf = SPIFIFO.read();
		#else
		do {
			if (len > 1) SPIFIFO.write(0, (len > 2) ? SPI_CONTINUE : 0);
			*buf++ = SPIFIFO.read();
			len--;
		} while (len > 0);
		#endif
	}
#endif
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
  } else if (chip == 52) {
    setSS();
    SPI.transfer(addr >> 8);
    SPI.transfer(addr & 0xFF);
    SPI.transfer((len >> 8) & 0x7F);
    SPI.transfer(len & 0xFF);
    for (uint16_t i=0; i<len; i++) {
      buf[i] = SPI.transfer(0);
    }
    resetSS();
  } else {
    setSS();
    if (addr < 0x100) {
      // common registers 00nn
      SPI.transfer(0);
      SPI.transfer(addr & 0xFF);
      SPI.transfer(0x00);
    } else if (addr < 0x8000) {
      // socket registers  10nn, 11nn, 12nn, 13nn, etc
      SPI.transfer(0);
      SPI.transfer(addr & 0xFF);
      SPI.transfer(((addr >> 3) & 0xE0) | 0x08);
    } else if (addr < 0xC000) {
      // transmit buffers  8000-87FF, 8800-8FFF, 9000-97FF, etc
      //  10## #nnn nnnn nnnn
      SPI.transfer((addr >> 8) & 0x03);
      SPI.transfer(addr & 0xFF);
      SPI.transfer(((addr >> 6) & 0xE0) | 0x10);
    } else {
      // receive buffers
      SPI.transfer((addr >> 8) & 0x03);
      SPI.transfer(addr & 0xFF);
      SPI.transfer(((addr >> 6) & 0xE0) | 0x18);
    }
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
