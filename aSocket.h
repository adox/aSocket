
/*

  -------------------------------------------------------------------
      aSocket.h, TCP/IP stack implementation for Arduino
  -------------------------------------------------------------------
  
	Version: 1.1

    Author: Adrian Brzezinski <iz0@poczta.onet.pl> (C)2010
	Copyright: GPL V2 (http://www.gnu.org/licenses/gpl.html)
		  
	TODO: TCP and UDP checksum for received packets?
*/

#ifndef __ASOCKET_H__
#define __ASOCKET_H__

#define ASOCKET_COMPILE_UDP
#define ASOCKET_COMPILE_TCP

#define ASOCKET_BUFSIZE	160
#define ASOCKET_CONTO		30000		// time out for whole connection
#define ASOCKET_REQTO		3000			// time out for various requests
#define ASOCKET_RETRIES	3

#define ASOCKET_NOFLAGS		0x0
#define ASOCKET_PGM_DATA	0x1
#define ASOCKET_MORE_DATA	0x2
#define ASOCKET_TCP_OPT		0x4
#define ASOCKET_CHECKSUM	0x8

#include <inttypes.h>
#include <avr/pgmspace.h>
#include "aInet.h"

extern "C" {
	#include "enc28j60.h"
}

//#define __ASOCK_DBG__
//#define __ASOCK_DBG_ETH__
//#define __ASOCK_DBG_ARP__
//#define __ASOCK_DBG_IP__
//#define __ASOCK_DBG_ICMP__
//#define __ASOCK_DBG_UDP__
//#define __ASOCK_DBG_TCP__

typedef enum constate_e {
		ASOCK_LISTEN=0,
		ASOCK_QUERYARP,
		ASOCK_INIT,
		ASOCK_ESTABLISHED,
		ASOCK_CLOSED
} constate_t;

class aSocket {

	// transmission control block, all of those data are in network order (big endian)
	uint8_t		hwaddr[ETH_ALEN];
	uint32_t	ipaddr;
	uint8_t		netmask;
	uint16_t	port;

	uint32_t	gatewayip;

	uint8_t		peerhwaddr[ETH_ALEN];
	uint32_t	peeripaddr;
	uint16_t	peerport;

	uint8_t		protocol;
	constate_t	constate;

#ifdef ASOCKET_COMPILE_TCP
	uint32_t	seq;
	uint16_t	seq_adv;		// expected advance for seq number
	uint32_t	ack;
#endif

	uint16_t	availdata;
	uint8_t		pktbuf[ASOCKET_BUFSIZE];

	void MakeEthReply( struct ethhdr *eth );
	void MakeIpReply( struct iphdr *ip, uint16_t tot_len );

	void MakeEth( struct ethhdr *eth, uint16_t h_proto );
	void MakeIp( struct iphdr *ip, uint16_t tot_len, uint8_t protocol );

#ifdef ASOCKET_COMPILE_TCP
	void MakeTcp( struct tcphdr *tcp, uint8_t tcpflags, uint16_t datalen, uint8_t flags );

	void InitSEQ();
	uint32_t IncNetNum( uint32_t &num, uint16_t addval );
	void SendTCPSYN();
#endif

#ifdef ASOCKET_COMPILE_UDP
	void MakeUdp( struct udphdr *udp, uint16_t datalen, uint8_t flags );
#endif

	uint16_t OnChipChecksum( uint16_t pktaddr, uint8_t prot, uint16_t datalen );
	void DispatchPacket( uint16_t pktlen );

	void copyhwa( uint8_t *srchwa, uint8_t *dsthwa );
	uint32_t subnet( uint32_t ip );

	void QueryARP();

	void HandleInetStack( uint32_t timeout );

public:
    aSocket( void );

	constate_t state() { return constate; }

	void setup( uint32_t ip, uint8_t hwa[ETH_ALEN], uint8_t mask, uint32_t gw );

	uint32_t listen( uint16_t portnum, uint8_t prot );
	uint8_t connect( uint32_t ip, uint16_t portnum, uint8_t prot );

	void flush() { availdata = 0; }
	uint16_t available();
	uint8_t* read( uint16_t *datasize );
	uint16_t write( uint8_t *data, uint16_t datasize, uint8_t flags );

	void close();
};

#endif * __ASOCKET_H__ */