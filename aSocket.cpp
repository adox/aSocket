
/*

  -------------------------------------------------------------------
      aSocket.c, TCP/IP stack implementation for Arduino
  -------------------------------------------------------------------
   
	Version: 1.1

    Author: Adrian Brzezinski <iz0@poczta.onet.pl> (C)2010
	Copyright: GPL V2 (http://www.gnu.org/licenses/gpl.html)

*/

#include "Arduino.h"
#include "aSocket.h"
#include "spiBus.h"


uint16_t checksum(uint16_t* addr, uint16_t len) {

	uint32_t sum = 0;

	while ( len > 1 ) {

		sum += *addr++;
		len -= 2;
	}

	if ( len > 0 ) sum += (uint16_t) *(uint8_t*)addr;

	while ( sum>>16 )
			sum = (sum & 0xffff) + (sum >> 16);

return ~sum;
}

void aSocket::MakeEthReply( struct ethhdr *eth ) {

	for ( uint8_t i = 0 ; i < ETH_ALEN ; i++ ) {
		eth->h_dest[i] = eth->h_source[i];
		eth->h_source[i] = hwaddr[i];
	}
}

void aSocket::MakeIpReply( struct iphdr *ip, uint16_t tot_len ) {

	if ( tot_len ) {

		tot_len = htons(tot_len);

		if ( ip->tot_len != tot_len ) {
			ip->tot_len = tot_len;

			ip->check = 0;
			ip->check = checksum((uint16_t*)ip, IPHDR_SIZE);
		}
	}

	ip->id += 1;
	ip->check -= 1;

	ip->daddr = ip->saddr;
	ip->saddr = ipaddr;
}

void aSocket::MakeEth( struct ethhdr *eth, uint16_t h_proto ) {

	for ( uint8_t i = 0 ; i < ETH_ALEN ; i++ ) {
		eth->h_source[i] = hwaddr[i];
		eth->h_dest[i] = peerhwaddr[i];
	}
	eth->h_proto = htons(h_proto);
}

void aSocket::MakeIp( struct iphdr *ip, uint16_t tot_len, uint8_t protocol ) {

	ip->version = IPVERSION;
	ip->ihl = 5;
	ip->tos = 0;
	ip->tot_len = htons(tot_len);
	ip->id = rand();
	ip->frag_off = 0x0040;			// Don't fragment set
	ip->ttl = IPDEFTTL;
	ip->protocol = protocol;
	ip->check = 0;
	ip->saddr = ipaddr;
	ip->daddr = peeripaddr;
	ip->check = checksum((uint16_t*)ip, IPHDR_SIZE);
}

#ifdef ASOCKET_COMPILE_TCP
void aSocket::MakeTcp( struct tcphdr *tcp, uint8_t tcpflags, uint16_t datalen, uint8_t flags ) {

	tcp->source = port;
	tcp->dest = peerport;
	tcp->seq = seq;
	tcp->ack_seq = ack;
	tcp->doff = (TCPHDR_SIZE+((flags&ASOCKET_TCP_OPT) ? 8 : 0))>>2;
	tcp->res1 = 0;
	tcp->flags = tcpflags;
	tcp->window = htons(TCP_MSS_DEFAULT);
	tcp->urg_ptr = 0;

	if ( flags & ASOCKET_TCP_OPT ) {

		// include MSS option
		uint8_t *opt = ((uint8_t*)tcp+TCPHDR_SIZE);
		opt[0] = 2;
		opt[1] = 4;
		opt[2] = TCP_MSS_DEFAULT>>8;
		opt[3] = TCP_MSS_DEFAULT&0xff;
		// include SACK option
		opt[4] = 4;
		opt[5] = 2;
		// EOL
		opt[6] = 0;
		opt[7] = 0;
	}

	// Be carefull here!
	if ( flags & ASOCKET_CHECKSUM ) {
		tcp->check = htons((tcp->doff<<2) + IPPROTO_TCP + datalen);
		tcp->check = checksum( (uint16_t*)(((uint8_t*)tcp)-8), 8+(tcp->doff<<2)+datalen );
	} else 
		tcp->check = 0;
}
#endif

#ifdef ASOCKET_COMPILE_UDP
void aSocket::MakeUdp( struct udphdr *udp, uint16_t datalen, uint8_t flags ) {

	udp->source = port;
	udp->dest = peerport;
	udp->len = htons(UDPHDR_SIZE+datalen);

	// Be carefull here!
	if ( flags & ASOCKET_CHECKSUM ) {
		udp->check = htons(UDPHDR_SIZE + IPPROTO_UDP + datalen);
		udp->check = checksum( (uint16_t*)(((uint8_t*)udp)-8), 8+UDPHDR_SIZE+datalen );
	} else
		udp->check = 0;
}
#endif

uint16_t aSocket::OnChipChecksum( uint16_t pktaddr, uint8_t prot, uint16_t datalen ) {

	uint16_t cs;
	uint16_t csoff;
	
	pktaddr += ETHHDR_SIZE + IPHDR_SIZE;

	switch ( prot ) {
	
	case IPPROTO_TCP:
		csoff = memoff(pktaddr,struct tcphdr,check);
		datalen += TCPHDR_SIZE;
		cs =  IPPROTO_TCP + datalen;
	break;
	
	case IPPROTO_UDP:
		csoff = memoff(pktaddr,struct udphdr,check);
		datalen += UDPHDR_SIZE;
		cs = IPPROTO_UDP + datalen;
	break;
	}
	
	cs = htons(cs);
	enc28j60_WriteMem( csoff, (uint8_t*)&cs, sizeof(uint16_t) );

	cs = htons( enc28j60_checksum( pktaddr-8, 8+datalen ) );

	enc28j60_WriteMem( csoff, (uint8_t*)&cs, sizeof(uint16_t) );
	
return cs;
}

void aSocket::DispatchPacket( uint16_t pktlen ) {

	enc28j60_NewPacket( pktlen );
	enc28j60_WritePacketData( 0, pktbuf, pktlen, 0 );
	enc28j60_SendNewPacket();
}

void aSocket::copyhwa( uint8_t *srchwa, uint8_t *dsthwa ) {

		for ( uint8_t i = 0 ; i < ETH_ALEN ; i++ ) dsthwa[i] = srchwa[i];
}

uint32_t aSocket::subnet( uint32_t ip ) {

	return ((((uint32_t)-1) >> (32-netmask)) << (32-netmask)) & ntohl(ip);
}

#ifdef ASOCKET_COMPILE_TCP
// generate pseudo random number for seq
void aSocket::InitSEQ() {

	seq = millis();		// millis() invoked here will add network jitter
	for ( uint8_t i = 0 ; i < 2 ; i++ ) ((uint16_t*)&seq)[i] ^= rand();

}

uint32_t aSocket::IncNetNum( uint32_t &num, uint16_t addval ) {

	return htonl( ntohl(num) + addval);
}

void aSocket::SendTCPSYN() {

	InitSEQ();
	ack = 0;

	MakeEth( (struct ethhdr*)pktbuf, ETH_P_IP );

	// +4 for MSS option, +4 for SACK
	MakeIp( (struct iphdr*)(pktbuf+ETHHDR_SIZE), IPHDR_SIZE+TCPHDR_SIZE+8, IPPROTO_TCP );
	MakeTcp( (struct tcphdr*)(pktbuf+ETHHDR_SIZE+IPHDR_SIZE), TCP_FLAG_SYN, 0, ASOCKET_TCP_OPT|ASOCKET_CHECKSUM );

	seq = IncNetNum(seq,1);

	DispatchPacket( ETHHDR_SIZE+IPHDR_SIZE+TCPHDR_SIZE+8 );
}
#endif

void aSocket::QueryARP() {

	struct ethhdr *eth = (struct ethhdr*)pktbuf;

	eth->h_proto = htons(ETH_P_ARP);

	struct arphdr *arp = (struct arphdr*)(pktbuf+ETHHDR_SIZE);
	arp->ar_hrd = htons(ARPHRD_ETHER);
	arp->ar_pro = htons(ETH_P_IP);
	arp->ar_hln = ETH_ALEN;
	arp->ar_pln = IP_ALEN;
	arp->ar_op = htons(ARPOP_REQUEST);

	arp->ar_sip = ipaddr;
	arp->ar_tip = peeripaddr;

	for ( uint8_t i = 0 ; i < ETH_ALEN ; i++ ) {
		eth->h_source[i] = hwaddr[i];
		eth->h_dest[i] = 0xff;
		arp->ar_sha[i] = hwaddr[i];
		arp->ar_tha[i] = 0;
	}

	DispatchPacket( ETHHDR_SIZE+ARPHDR_SIZE );

}

void aSocket::HandleInetStack( uint32_t timeout ) {

	#ifdef __ASOCK_DBG__
		_FUNCTION_DBG_INFO_
	#endif

	uint16_t datalen;
	uint32_t m = millis();
	constate_t	initialcs = constate;

	while ( ((millis() - m) < timeout) && initialcs == constate ) {
	
		uint16_t pktlen;

		if ( !(pktlen = enc28j60_ReceivePkt()) || pktlen > ETH_DATA_LEN ) continue;
		enc28j60_ReadPacketData( 0, pktbuf, (pktlen < ASOCKET_BUFSIZE) ? pktlen : ASOCKET_BUFSIZE );

		struct ethhdr *eth = (struct ethhdr*)pktbuf;

		#ifdef __ASOCK_DBG_ETH__
			Serial.print("ETH len: ");
			Serial.println(pktlen,DEC);
		#endif
		
		if ( ntohs(eth->h_proto) == ETH_P_ARP ) {
		
			struct arphdr *arp = (struct arphdr*)(pktbuf+ETHHDR_SIZE);
			
			#ifdef __ASOCK_DBG_ARP__
				Serial.print("ARP op: ");
				Serial.println(ntohs(arp->ar_op),HEX);
			#endif

			switch ( ntohs(arp->ar_op) ) {
	
			case ARPOP_REQUEST:

				if ( arp->ar_tip != ipaddr ) break;

				// make reply frame
				for ( uint8_t i = 0 ; i < ETH_ALEN ; i++ ) {

					eth->h_dest[i] = eth->h_source[i];
					eth->h_source[i] = hwaddr[i];

					arp->ar_tha[i] = arp->ar_sha[i];
					arp->ar_sha[i] = hwaddr[i];
				}

				arp->ar_tip = arp->ar_sip;
				arp->ar_sip = ipaddr;
				arp->ar_op = htons(ARPOP_REPLY);

				DispatchPacket( pktlen );
			break;

			case ARPOP_REPLY:
				if ( constate != ASOCK_QUERYARP || peeripaddr != arp->ar_sip ) break;

				copyhwa(arp->ar_sha,peerhwaddr);

				// we can initialize connection now
				constate = ASOCK_INIT;
			break;
			}

		} else if ( ntohs(eth->h_proto) == ETH_P_IP ) {
		
			struct iphdr *ip = (struct iphdr*)(pktbuf+ETHHDR_SIZE);
			
			#ifdef __ASOCK_DBG_IP__
				Serial.print("IP src: ");
				Serial.print(ntohl(ip->saddr),HEX);
				Serial.print(" dst: ");
				Serial.println(ntohl(ip->daddr),HEX);
			#endif

			if ( ip->version != IPVERSION || ip->ihl != 5 || ip->daddr != ipaddr ) {
				enc28j60_FreeReceivedPkt();
				continue;
			}

			// We can't rely on received bytes count for frames smaller than 60 bytes (+4 for crc).
			// (for eg. tcp syn+ack, or udp packet)
			pktlen = ntohs(ip->tot_len) + ETHHDR_SIZE;

			if ( ip->protocol == IPPROTO_ICMP ) {

				struct icmphdr *icmp = (struct icmphdr*)( (uint8_t*)ip + (ip->ihl << 2) );

				#ifdef __ASOCK_DBG_ICMP__
					Serial.print("ICMP type: ");
					Serial.println(icmp->type,HEX);
				#endif

				if ( icmp->type == ICMP_ECHO ) {

					// make reply frame
					MakeEthReply( eth );
					MakeIpReply( ip, 0 );

					// icmp
					icmp->type = ICMP_ECHOREPLY;
					icmp->checksum += 8;		// add our change to checksum

					DispatchPacket( pktlen );
				}

			} else

#ifdef ASOCKET_COMPILE_UDP
			if ( ip->protocol == IPPROTO_UDP && protocol == IPPROTO_UDP ) {
				struct udphdr *udp = (struct udphdr*)( (uint8_t*)ip + (ip->ihl << 2) );
				datalen = ntohs(udp->len) - UDPHDR_SIZE;
				
				#ifdef __ASOCK_DBG_UDP__
				Serial.print("tcp: src ");
				Serial.print(ntohs(udp->source),DEC);
				Serial.print(" dst ");
				Serial.println(ntohs(udp->dest),DEC);
				Serial.print(" pktlen ");
				Serial.println(pktlen,DEC);
				#endif

				if ( port != udp->dest || OnChipChecksum(enc28j60_ReceivedPktAddr(),IPPROTO_UDP,datalen) != udp->check ) {
					enc28j60_FreeReceivedPkt();
					continue;
				}

				if ( constate == ASOCK_ESTABLISHED || constate == ASOCK_LISTEN ) {

					if ( datalen ) {

						if ( constate == ASOCK_LISTEN ) {
							copyhwa(eth->h_source,peerhwaddr);
							peeripaddr = ip->saddr;
							peerport = udp->source;
							constate = ASOCK_ESTABLISHED;
						}

						if ( availdata+datalen > RXBUFSIZE ) datalen = RXBUFSIZE-availdata;

						enc28j60_CopyMem( enc28j60_ReceivedPktAddr()+pktlen-datalen, RXBUFFER+availdata, datalen );
						availdata += datalen;

						enc28j60_FreeReceivedPkt();
					return;
					}
				}

			} else
#endif
#ifdef ASOCKET_COMPILE_TCP
			if ( ip->protocol == IPPROTO_TCP && protocol == IPPROTO_TCP ) {
				struct tcphdr *tcp = (struct tcphdr*)( (uint8_t*)ip + (ip->ihl << 2) );

				uint16_t tcpoffset = ((uint8_t*)tcp-(uint8_t*)eth);
				datalen = pktlen - (tcpoffset+TCPHDR_SIZE);

				#ifdef __ASOCK_DBG_TCP__
				Serial.print("tcp: src ");
				Serial.print(ntohs(tcp->source),DEC);
				Serial.print(" dst ");
				Serial.println(ntohs(tcp->dest),DEC);
				Serial.print(" flags ");
				Serial.print(tcp->flags,HEX);
				Serial.print(" pktlen ");
				Serial.println(pktlen,DEC);
				Serial.print(" check ");
				Serial.print(OnChipChecksum(enc28j60_ReceivedPktAddr(),IPPROTO_TCP,datalen),HEX);
				Serial.print(' ');
				Serial.println(tcp->check,HEX);
				#endif

				if ( port != tcp->dest || OnChipChecksum(enc28j60_ReceivedPktAddr(),IPPROTO_TCP,datalen) != tcp->check ) {
					enc28j60_FreeReceivedPkt();
					continue;
				}

				// now we can proceed
				switch ( constate ) {

				case ASOCK_INIT:
					if ( !(tcp->flags == (TCP_FLAG_SYN|TCP_FLAG_ACK)) || tcp->ack_seq != seq || peerport != tcp->source ) break;

					ack = IncNetNum( tcp->seq, 1 );

					// make reply frame (we may need to cut off possible tcp options)
					MakeEthReply( eth );
					MakeIpReply( ip, tcpoffset+TCPHDR_SIZE-ETHHDR_SIZE );
					MakeTcp( tcp, TCP_FLAG_ACK, 0, ASOCKET_CHECKSUM );

					DispatchPacket( tcpoffset+TCPHDR_SIZE );

					constate = ASOCK_ESTABLISHED;
				break;

				case ASOCK_LISTEN:
					if ( tcp->flags != TCP_FLAG_SYN ) break;

					copyhwa(eth->h_source,peerhwaddr);
					peeripaddr = ip->saddr;
					peerport = tcp->source;

					InitSEQ();
					ack = IncNetNum( tcp->seq, 1 );

					MakeEthReply( eth );
					MakeIpReply( ip, (tcpoffset+TCPHDR_SIZE+8)-ETHHDR_SIZE );
					MakeTcp( tcp, TCP_FLAG_SYN|TCP_FLAG_ACK, 0, ASOCKET_TCP_OPT|ASOCKET_CHECKSUM );

					DispatchPacket( tcpoffset+TCPHDR_SIZE+8 );

					seq = IncNetNum( seq, 1 );
					constate = ASOCK_ESTABLISHED;
				break;

				case ASOCK_ESTABLISHED:
					if ( tcp->seq != ack || peerport != tcp->source ) break;

					// is it carry proper ack?
					if (  (tcp->flags & TCP_FLAG_ACK) && tcp->ack_seq == IncNetNum(seq,seq_adv) )
						seq = tcp->ack_seq;
					else
						// no, ignore this packet
						break;

					MakeEthReply( eth );
					MakeIpReply( ip, tcpoffset+TCPHDR_SIZE-ETHHDR_SIZE );

					if ( tcp->flags & (TCP_FLAG_RST|TCP_FLAG_FIN) ) {

						// send rst
						if ( tcp->flags & TCP_FLAG_FIN ) {
							MakeTcp( tcp, TCP_FLAG_RST|TCP_FLAG_ACK, 0, ASOCKET_CHECKSUM );
							DispatchPacket( tcpoffset+TCPHDR_SIZE );
						}

						constate = ASOCK_CLOSED;
						break;
					}

					if ( tcp->flags & TCP_FLAG_ACK ) {
					
						// we may need to cut off possible tcp options
						datalen = pktlen - (tcpoffset+(tcp->doff<<2));

						if ( availdata+datalen > RXBUFSIZE ) datalen = RXBUFSIZE-availdata;

						// send ack if we got any data
						if ( datalen ) {

							enc28j60_CopyMem( enc28j60_ReceivedPktAddr()+pktlen-datalen, RXBUFFER+availdata, datalen );
							availdata += datalen;

							ack = IncNetNum( ack, datalen );

							MakeTcp( tcp, TCP_FLAG_ACK, 0, ASOCKET_CHECKSUM );
							DispatchPacket( tcpoffset+TCPHDR_SIZE );
						}
					}
					
					enc28j60_FreeReceivedPkt();
				return;
				}
			}
#else
		{ }
#endif
		}

		enc28j60_FreeReceivedPkt();
	}
}

// --------------- public members

aSocket::aSocket( ) {
}

void aSocket::setup( uint32_t ip, uint8_t hwa[ETH_ALEN], uint8_t mask, uint32_t gw ) {

	constate = ASOCK_CLOSED;

	copyhwa( hwa, hwaddr );
	ipaddr = ip;
	netmask = (mask > 32) ? 32 : mask;
	gatewayip = gw;

	peeripaddr = INADDR_NONE;

#ifdef __ASOCK_DBG__
	Serial.print("aSocket::setup> hwa ");
	for (uint8_t i = 0; i < ETH_ALEN ; i++) { Serial.print(hwaddr[i],HEX); Serial.print(':'); }
	Serial.print(" ip ");
	Serial.print(ntohl(ipaddr),HEX);
	Serial.print(" mask ");
	Serial.print(netmask,DEC);
	Serial.print(" gw ");
	Serial.println(ntohl(gatewayip),HEX);
#endif
}

uint32_t aSocket::listen( uint16_t portnum, uint8_t prot ) {

	port = portnum;
	protocol = prot;
	constate = ASOCK_LISTEN;

#ifdef ASOCKET_COMPILE_TCP
	seq = 0;
	seq_adv = 0;
	ack = 0;
#endif
	availdata = 0;

	while ( constate == ASOCK_LISTEN ) HandleInetStack(ASOCKET_CONTO);

	if ( constate != ASOCK_ESTABLISHED ) close();

return peeripaddr;
}

uint8_t aSocket::connect( uint32_t ip, uint16_t portnum, uint8_t prot ) {

	peerport = portnum;
	protocol = prot;

	// ports below 1024 are reserved
	port = rand();
	port = htons((port < 1024) ? port + 1024 : port);

#ifdef ASOCKET_COMPILE_TCP
	seq = 0;
	seq_adv = 0;
	ack = 0;
#endif
	availdata = 0;
	constate = ASOCK_QUERYARP;

	while ( 1 ) {

		// while doing assignment here we will optimize code size
		peeripaddr = ip;

		switch ( constate ) {

		case ASOCK_QUERYARP:

			// do we need to use gateway?
			if ( subnet(ipaddr) != subnet(peeripaddr) ) peeripaddr = gatewayip;

			for ( uint8_t counter = 0 ; counter < ASOCKET_RETRIES && constate == ASOCK_QUERYARP ; counter++ ) {

				QueryARP();
				HandleInetStack(ASOCKET_REQTO);
			}

			if ( constate == ASOCK_QUERYARP ) constate = ASOCK_CLOSED;

		break;

		case ASOCK_INIT:
#ifdef ASOCKET_COMPILE_UDP
			if ( protocol == IPPROTO_UDP ) {
				constate = ASOCK_ESTABLISHED;
				break;
			}
#endif
#ifdef ASOCKET_COMPILE_TCP
			for ( uint8_t counter = 0 ; counter < ASOCKET_RETRIES && constate == ASOCK_INIT ; counter++ ) {

				SendTCPSYN();
				HandleInetStack(ASOCKET_REQTO);
			}

			if ( constate == ASOCK_INIT ) constate = ASOCK_CLOSED;

		break;
#endif
		case ASOCK_ESTABLISHED:
		return 0;

		case ASOCK_CLOSED:
			close();
		return 1;
		}
	}

return 1;
}

uint16_t aSocket::available() {

	HandleInetStack(1);

	if ( constate != ASOCK_ESTABLISHED ) close();

return availdata;
}

uint8_t* aSocket::read( uint16_t *datasize ) {

	if ( *datasize > ASOCKET_BUFSIZE ) *datasize = ASOCKET_BUFSIZE;

	// invoking available() will read possible pendings
	if ( *datasize > available() ) *datasize = availdata;

	if ( *datasize ) {
	
		enc28j60_ReadMem( RXBUFFER, pktbuf, *datasize );

		availdata -= *datasize;
		if ( availdata ) enc28j60_CopyMem( RXBUFFER+*datasize, RXBUFFER, availdata );
		
	return pktbuf;
	}

return NULL;
}

uint16_t aSocket::write( uint8_t *data, uint16_t datasize, uint8_t flags ) {

	static uint16_t dataoff = 0;

	if ( constate != ASOCK_ESTABLISHED ) return 0;
	
	if ( !dataoff ) {
	
		// create new packet
		dataoff = ETHHDR_SIZE + IPHDR_SIZE + ((protocol == IPPROTO_TCP) ? TCPHDR_SIZE : UDPHDR_SIZE);
		enc28j60_NewPacket( dataoff );
	}

	// adjust packet size
	if ( (dataoff+datasize) > MAX_FRAMELEN ) {
		datasize = MAX_FRAMELEN - dataoff;

		// packet oversized, send it immediately
		flags &= ~ASOCKET_MORE_DATA;
	}

	if ( datasize ) {
		enc28j60_SetNewPacketLen( dataoff+datasize );
		enc28j60_WritePacketData(dataoff,data,datasize, (flags&ASOCKET_PGM_DATA) );

		dataoff += datasize;
	}

	// wait for more data
	if ( flags & ASOCKET_MORE_DATA ) return datasize;
	
	MakeEth( (struct ethhdr*)pktbuf, ETH_P_IP );
	MakeIp( (struct iphdr*)(pktbuf+ETHHDR_SIZE), dataoff-ETHHDR_SIZE, protocol );
	
	uint16_t datalen;

	if ( protocol == IPPROTO_TCP ) {
#ifdef ASOCKET_COMPILE_TCP
		datalen = dataoff - ETHHDR_SIZE - IPHDR_SIZE - TCPHDR_SIZE;

		MakeTcp( (struct tcphdr*)(pktbuf+ETHHDR_SIZE+IPHDR_SIZE), TCP_FLAG_PSH|TCP_FLAG_ACK, datalen, ASOCKET_NOFLAGS );
		enc28j60_WritePacketData(0,pktbuf,ETHHDR_SIZE+IPHDR_SIZE+TCPHDR_SIZE, 0 );
#endif
	} else {
#ifdef ASOCKET_COMPILE_UDP
		datalen = dataoff - ETHHDR_SIZE - IPHDR_SIZE - UDPHDR_SIZE;

		MakeUdp( (struct udphdr*)(pktbuf+ETHHDR_SIZE+IPHDR_SIZE), datalen, ASOCKET_NOFLAGS );
		enc28j60_WritePacketData(0,pktbuf,ETHHDR_SIZE+IPHDR_SIZE+UDPHDR_SIZE, 0 );
#endif
	}

	OnChipChecksum( enc28j60_NewPktAddr(), protocol, datalen );
	seq_adv = datalen;

	uint8_t counter = 0;
	for ( ; counter < ASOCKET_RETRIES && constate == ASOCK_ESTABLISHED ; counter++ ) {

		enc28j60_SendNewPacket();

		if ( protocol == IPPROTO_TCP ) {
#ifdef ASOCKET_COMPILE_TCP

			uint32_t initialseq = seq;
			HandleInetStack(ASOCKET_REQTO);		// wait for ack

			if ( seq == IncNetNum(initialseq,seq_adv) ) break;
			
			// no ack received, loop back for retransmission
#endif
		} else {
#ifdef ASOCKET_COMPILE_UDP
			break;
#endif
		}
	}

	dataoff = 0;
	seq_adv = 0;

	if ( constate != ASOCK_ESTABLISHED || counter == ASOCKET_RETRIES ) {
		close();
	}

return datasize;
}

void aSocket::close() {
#ifdef ASOCKET_COMPILE_TCP
	if ( constate == ASOCK_ESTABLISHED && protocol == IPPROTO_TCP ) {

		// send RST
		MakeEth( (struct ethhdr*)pktbuf, ETH_P_IP );
		MakeIp( (struct iphdr*)(pktbuf+ETHHDR_SIZE), IPHDR_SIZE+TCPHDR_SIZE, IPPROTO_TCP );
		MakeTcp( (struct tcphdr*)(pktbuf+ETHHDR_SIZE+IPHDR_SIZE), TCP_FLAG_RST|TCP_FLAG_ACK, 0, ASOCKET_CHECKSUM );
		DispatchPacket( ETHHDR_SIZE+IPHDR_SIZE+TCPHDR_SIZE );
	}
#endif
	constate = ASOCK_CLOSED;
	peeripaddr = INADDR_NONE;
	peerport = 0;
}