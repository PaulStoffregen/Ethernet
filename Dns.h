// Arduino DNS client for WizNet5100-based Ethernet shield
// (c) Copyright 2009-2010 MCQN Ltd.
// Released under Apache License, version 2.0

#ifndef DNSClient_h
#define DNSClient_h

#include <EthernetUdp.h>

class DNSClient
{
public:
    // ctor
    void begin(const IPAddress& aDNSServer);

    /** Convert a numeric IP address string into a four-byte IP address.
        @param aIPAddrString IP address to convert
        @param aResult IPAddress structure to store the returned IP address
        @result 1 if aIPAddrString was successfully converted to an IP address,
                else error code
    */
    int inet_aton(const char *aIPAddrString, IPAddress& aResult);

    /** Resolve the given hostname to an IP address.
        @param aHostname Name to be resolved
        @param aResult IPAddress structure to store the returned IP address
        @result 1 if aIPAddrString was successfully converted to an IP address,
                else error code
    */
    int getHostByName(const char* aHostname, IPAddress& aResult);

protected:
    uint16_t BuildRequest(const char* aName);
	uint16_t ProcessResponse(const uint16_t aTimeout, const IPAddress& aAddress);

    IPAddress iDNSServer;
    uint16_t iRequestId;
    EthernetUDP iUdp;

	private:
	static constexpr uint8_t SOCKET_NONE = 255;
	// Various flags and header field values for a DNS message
	static constexpr uint8_t UDP_HEADER_SIZE          = 8;
	static constexpr uint8_t DNS_HEADER_SIZE          = 12;
	static constexpr uint8_t TTL_SIZE                 = 4;
	static constexpr uint8_t QUERY_FLAG               = (0);
	static constexpr uint8_t RESPONSE_FLAG            = (1<<15);
	static constexpr uint8_t QUERY_RESPONSE_MASK      = (1<<15);
	static constexpr uint8_t OPCODE_STANDARD_QUERY    = (0);
	static constexpr uint8_t OPCODE_INVERSE_QUERY     = (1<<11);
	static constexpr uint8_t OPCODE_STATUS_REQUEST    = (2<<11);
	static constexpr uint8_t OPCODE_MASK              = (15<<11);
	static constexpr uint8_t AUTHORITATIVE_FLAG       = (1<<10);
	static constexpr uint8_t TRUNCATION_FLAG          = (1<<9);
	static constexpr uint8_t RECURSION_DESIRED_FLAG   = (1<<8);
	static constexpr uint8_t RECURSION_AVAILABLE_FLAG = (1<<7);
	static constexpr uint8_t RESP_NO_ERROR            = (0);
	static constexpr uint8_t RESP_FORMAT_ERROR        = (1);
	static constexpr uint8_t RESP_SERVER_FAILURE      = (2);
	static constexpr uint8_t RESP_NAME_ERROR          = (3);
	static constexpr uint8_t RESP_NOT_IMPLEMENTED     = (4);
	static constexpr uint8_t RESP_REFUSED             = (5);
	static constexpr uint8_t RESP_MASK                = (15);
	static constexpr uint8_t TYPE_A                   = (0x0001);
	static constexpr uint8_t CLASS_IN                 = (0x0001);
	static constexpr uint8_t LABEL_COMPRESSION_MASK   = (0xC0);
	// Port number that DNS servers listen on
	static constexpr uint8_t DNS_PORT                  = 53;

	// Possible return codes from ProcessResponse
	static constexpr uint8_t SUCCESS                   = 1;
	static constexpr uint8_t TIMED_OUT                 = -1;
	static constexpr uint8_t INVALID_SERVER            = -2;
	static constexpr uint8_t TRUNCATED                 = -3;
	static constexpr uint8_t INVALID_RESPONSE          = -4;
};

#endif
