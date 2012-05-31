
/*

  -------------------------------------------------------------------
      aInet.h, TCP/IP stack implementation for Arduino
  -------------------------------------------------------------------
    
	Version: 1.1

    Author: Adrian Brzezinski <iz0@poczta.onet.pl> (C)2010
	Copyright: GPL V2 (http://www.gnu.org/licenses/gpl.html)

*/

#ifndef __AINET_H__
#define __AINET_H__

#include <inttypes.h>

#define __LITTLE_ENDIAN	1234
#define __BIG_ENDIAN		4321
#define __AVR_ENDIAN		__LITTLE_ENDIAN

// ---------------------------------------------------------------------------
// ethernet stuff (based on linux/if_ether.h)

/*
 *      IEEE 802.3 Ethernet magic constants.  The frame sizes omit the preamble
 *      and FCS/CRC (frame check sequence).
 */
#define ETH_ALEN				6			/* Octets in one ethernet addr   */
#define ETH_HLEN				14		/* Total octets in header.       */
#define ETH_ZLEN				60		/* Min. octets in frame sans FCS */
#define ETH_DATA_LEN		1500		/* Max. octets in payload        */
#define ETH_FRAME_LEN	1514		/* Max. octets in frame sans FCS */
#define ETH_FCS_LEN		4			/* Octets in the FCS             */

 /*
  *      These are the defined Ethernet Protocol ID's.
  */
#define ETH_P_IP				0x0800          /* Internet Protocol packet     */
#define ETH_P_ARP			0x0806          /* Address Resolution packet    */
#define ETH_P_RARP			0x8035          /* Reverse Addr Res packet      */
#define ETH_P_8021Q		0x8100          /* 802.1Q VLAN Extended Header  */
#define ETH_P_PAUSE		0x8808          /* IEEE Pause frames. See 802.3 31B */

/*
 *      This is an Ethernet frame header.
 */
struct ethhdr {
	uint8_t		h_dest[ETH_ALEN];		/* destination eth addr */
	uint8_t		h_source[ETH_ALEN];		/* source ether addr    */
	uint16_t	h_proto;						/* packet type ID field */
} __attribute__((packed));

#define ETHHDR_SIZE	sizeof(struct ethhdr)

// ---------------------------------------------------------------------------
// arp stuff (based on linux/if_arp.h)

/* ARP protocol HARDWARE identifiers. */
#define ARPHRD_ETHER						1				/* Ethernet 10Mbps              */
#define ARPHRD_TUNNEL					768			/* IPIP tunnel                  */
#define ARPHRD_IEEE80211 				801			/* IEEE 802.11                  */
#define ARPHRD_IEEE80211_PRISM	802			/* IEEE 802.11 + Prism2 header  */
#define ARPHRD_VOID						0xFFFF		/* Void type, nothing is known */
#define ARPHRD_NONE						0xFFFE		/* zero header length */

/* ARP protocol opcodes. */
#define ARPOP_REQUEST		1		/* ARP request                  */
#define ARPOP_REPLY			2		/* ARP reply                    */
#define ARPOP_RREQUEST		3		/* RARP request                 */
#define ARPOP_RREPLY			4		/* RARP reply                   */
#define ARPOP_InREQUEST	8		/* InARP request                */
#define ARPOP_InREPLY			9		/* InARP reply                  */
#define ARPOP_NAK				10	/* (ATM)ARP NAK                 */

/*
 *      This structure defines an ethernet arp header.
 */
struct arphdr {
	uint16_t 	ar_hrd;			/* format of hardware address   */
	uint16_t	ar_pro;			/* format of protocol address   */
	uint8_t		ar_hln;			/* length of hardware address   */
	uint8_t		ar_pln;			/* length of protocol address   */
	uint16_t	ar_op;			/* ARP opcode (command)         */

// arp payload
	uint8_t		ar_sha[ETH_ALEN];	/* sender hardware address      */
	uint32_t	ar_sip;
//	uint8_t		ar_sip[IP_ALEN];		/* sender IP address            */
	uint8_t		ar_tha[ETH_ALEN];	/* target hardware address      */
	uint32_t	ar_tip;
//	uint8_t		ar_tip[IP_ALEN];		/* target IP address            */
} __attribute__((packed));

#define ARPHDR_SIZE	sizeof(struct arphdr)

// ---------------------------------------------------------------------------
// ipv4 stuff (based on linux/ip.h and netinet/in.h)

#define IP_ALEN		4

#define IPVERSION	4
#define IPMAXTTL		255
#define IPDEFTTL		64

/*
 * Protocols (RFC 1700)
 */
#define	IPPROTO_IP			0			/* dummy for IP */
#define	IPPROTO_ICMP		1			/* control message protocol */
#define	IPPROTO_IGMP		2			/* group mgmt protocol */
#define IPPROTO_IPV4		4			/* IPv4 encapsulation */
#define IPPROTO_IPIP		IPPROTO_IPV4	/* for compatibility */
#define	IPPROTO_TCP		6			/* tcp */
#define	IPPROTO_UDP		17		/* user datagram protocol */
#define	IPPROTO_RAW		255		/* raw IP packet */
#define	IPPROTO_MAX		256

/*
 * Definitions of bits in internet address integers.
 * On subnets, the decomposition of addresses to host and net parts
 * is done according to subnet mask, not the masks here.
 */
#define	IN_CLASSA(i)			(((uint32_t)(i) & 0x80000000) == 0)
#define	IN_CLASSA_NET		0xff000000
#define	IN_CLASSA_NSHIFT	24
#define	IN_CLASSA_HOST		0x00ffffff
#define	IN_CLASSA_MAX		128

#define	IN_CLASSB(i)			(((uint32_t)(i) & 0xc0000000) == 0x80000000)
#define	IN_CLASSB_NET		0xffff0000
#define	IN_CLASSB_NSHIFT	16
#define	IN_CLASSB_HOST		0x0000ffff
#define	IN_CLASSB_MAX		65536

#define	IN_CLASSC(i)			(((uint32_t)(i) & 0xe0000000) == 0xc0000000)
#define	IN_CLASSC_NET		0xffffff00
#define	IN_CLASSC_NSHIFT	8
#define	IN_CLASSC_HOST		0x000000ff

#define	IN_CLASSD(i)			(((uint32_t)(i) & 0xf0000000) == 0xe0000000)
#define	IN_CLASSD_NET		0xf0000000		/* These ones aren't really */
#define	IN_CLASSD_NSHIFT	28					/* net and host fields, but */
#define	IN_CLASSD_HOST		0x0fffffff			/* routing needn't know.    */
#define	IN_MULTICAST(i)		IN_CLASSD(i)

#define	IN_EXPERIMENTAL(i)	(((uint32_t)(i) & 0xf0000000) == 0xf0000000)
#define	IN_BADCLASS(i)		(((uint32_t)(i) & 0xf0000000) == 0xf0000000)

#define	INADDR_ANY					(uint32_t)0x00000000
#define	INADDR_LOOPBACK		(uint32_t)0x7f000001
#define	INADDR_BROADCAST	(uint32_t)0xffffffff	/* must be masked */
#define	INADDR_NONE				0xffffffff				/* -1 return */

#define	INADDR_UNSPEC_GROUP		(uint32_t)0xe0000000	/* 224.0.0.0 */
#define	INADDR_ALLHOSTS_GROUP	(uint32_t)0xe0000001	/* 224.0.0.1 */
#define	INADDR_ALLRTRS_GROUP		(uint32_t)0xe0000002	/* 224.0.0.2 */
#define	INADDR_MAX_LOCAL_GROUP	(uint32_t)0xe00000ff		/* 224.0.0.255 */

struct iphdr {
#if (__AVR_ENDIAN == __LITTLE_ENDIAN)
	uint8_t		ihl:4,version:4;
#else
	uint8_t		version:4,ihl:4;
#endif
	uint8_t		tos;
	uint16_t	tot_len;
	uint16_t	id;
	uint16_t	frag_off;
	uint8_t		ttl;
	uint8_t		protocol;
	uint16_t	check;
	uint32_t	saddr;
	uint32_t	daddr;
} __attribute__((packed));

#define IPHDR_SIZE	sizeof(struct iphdr)

// ---------------------------------------------------------------------------
// icmp stuff (based on linux/icmp.h)

#define ICMP_ECHOREPLY          0       /* Echo Reply                   */
#define ICMP_DEST_UNREACH       3       /* Destination Unreachable      */
#define ICMP_SOURCE_QUENCH      4       /* Source Quench                */
#define ICMP_REDIRECT           5       /* Redirect (change route)      */
#define ICMP_ECHO               8       /* Echo Request                 */
#define ICMP_TIME_EXCEEDED      11      /* Time Exceeded                */
#define ICMP_PARAMETERPROB      12      /* Parameter Problem            */
#define ICMP_TIMESTAMP          13      /* Timestamp Request            */
#define ICMP_TIMESTAMPREPLY     14      /* Timestamp Reply              */
#define ICMP_INFO_REQUEST       15      /* Information Request          */
#define ICMP_INFO_REPLY         16      /* Information Reply            */
#define ICMP_ADDRESS            17      /* Address Mask Request         */
#define ICMP_ADDRESSREPLY       18      /* Address Mask Reply           */
#define NR_ICMP_TYPES           18
 
/* Codes for UNREACH. */
#define ICMP_NET_UNREACH        0       /* Network Unreachable          */
#define ICMP_HOST_UNREACH       1       /* Host Unreachable             */
#define ICMP_PROT_UNREACH       2       /* Protocol Unreachable         */
#define ICMP_PORT_UNREACH       3       /* Port Unreachable             */
#define ICMP_FRAG_NEEDED        4       /* Fragmentation Needed/DF set  */
#define ICMP_SR_FAILED          5       /* Source Route failed          */
#define ICMP_NET_UNKNOWN        6
#define ICMP_HOST_UNKNOWN       7
#define ICMP_HOST_ISOLATED      8
#define ICMP_NET_ANO            9
#define ICMP_HOST_ANO           10
#define ICMP_NET_UNR_TOS        11
#define ICMP_HOST_UNR_TOS       12
#define ICMP_PKT_FILTERED       13      /* Packet filtered */
#define ICMP_PREC_VIOLATION     14      /* Precedence violation */
#define ICMP_PREC_CUTOFF        15      /* Precedence cut off */
#define NR_ICMP_UNREACH         15      /* instead of hardcoding immediate value */

/* Codes for REDIRECT. */
#define ICMP_REDIR_NET          0       /* Redirect Net                 */
#define ICMP_REDIR_HOST         1       /* Redirect Host                */
#define ICMP_REDIR_NETTOS       2       /* Redirect Net for TOS         */
#define ICMP_REDIR_HOSTTOS      3       /* Redirect Host for TOS        */
 
/* Codes for TIME_EXCEEDED. */
#define ICMP_EXC_TTL            0       /* TTL count exceeded           */
#define ICMP_EXC_FRAGTIME       1       /* Fragment Reass time exceeded */
 
struct icmphdr {
	uint8_t		type;
	uint8_t		code;
	uint16_t	checksum;

	union {
		struct {
			uint16_t	id;
			uint16_t	sequence;
		} echo;

		uint32_t	gateway;
		struct {
			uint16_t	__unused;
			uint16_t	mtu;
		} frag;
	} un;
} __attribute__((packed));

#define ICMPHDR_SIZE	sizeof(struct icmphdr)

// ---------------------------------------------------------------------------
// udp stuff (based on linux/udp.h)

struct udphdr {
	uint16_t	source;
	uint16_t	dest;
	uint16_t	len;
	uint16_t	check;
} __attribute__((packed));

#define UDPHDR_SIZE	sizeof(struct udphdr)

// ---------------------------------------------------------------------------
// tcp stuff (based on linux/tcp.h)

/*
 * TCP general constants
 */
#define TCP_MSS_DEFAULT          536U   /* IPv4 (RFC1122, RFC2581) */
#define TCP_MSS_DESIRED         1220U   /* IPv6 (tunneled), EDNS0 (RFC3226) */

#define TCP_FLAG_FIN		0x01
#define TCP_FLAG_SYN		0x02
#define TCP_FLAG_RST		0x04
#define TCP_FLAG_PSH		0x08
#define TCP_FLAG_ACK		0x10
#define TCP_FLAG_URG		0x20
#define TCP_FLAG_ECE		0x40
#define TCP_FLAG_CWR	0x80

struct tcphdr {
	uint16_t	source;
	uint16_t	dest;
	uint32_t	seq;
	uint32_t	ack_seq;
 
 #if (__AVR_ENDIAN == __LITTLE_ENDIAN)
	uint8_t		res1:4,
					doff:4;
#else
	uint8_t		doff:4,
					res1:4;
#endif
	uint8_t		flags;
	uint16_t	window;
	uint16_t	check;
	uint16_t	urg_ptr;
} __attribute__((packed));

#define TCPHDR_SIZE	sizeof(struct tcphdr)

// ---------------------------------------------------------------------------
// functions

#if (__AVR_ENDIAN == __LITTLE_ENDIAN)

#define htons(n) ( ((uint16_t)(n) << 8) | ((uint16_t)(n) >> 8) )
#define ntohs(n) htons(n)

#define htonl(n) (((((uint32_t)(n) & 0xFF)) << 24) | \
                  ((((uint32_t)(n) & 0xFF00)) << 8) | \
                  ((((uint32_t)(n) & 0xFF0000)) >> 8) | \
                  ((((uint32_t)(n) & 0xFF000000)) >> 24))
#define ntohl(n) htonl(n)

#else

#define htons(n) (n)
#define ntohs(n) (n)
#define htonl(n) (n)
#define ntohl(n) (n)

#endif

#endif /* __AINET_H__ */