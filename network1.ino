 
/*

  --------------------------------------------------------------------
      network.cpp, Simple Ethernet and TCP/IP Interface
  --------------------------------------------------------------------

    Author: Adrian Brzezinski <iz0@poczta.onet.pl> (C)2010
	Copyright: GPL V2 (http://www.gnu.org/licenses/gpl.html)
*/

//#define __DBG__

#include "Arduino.h"
#include "spiBus.h"
#include "aSocket.h"

extern "C" {
	#include "enc28j60.h"
}

uint8_t hwaddr[ETH_ALEN] = {0x01,0x02,0x03,0x10,0x00,0x09};
uint32_t ipaddr = 0x0A000009;
uint8_t mask = 24;
uint32_t defgw = 0x0A000001;
char pass[10];

uint32_t authtime;
uint32_t authip;
uint8_t auth = 0;

aSocket sock = aSocket();

struct httpreq {

	PGM_P name;
	char *value;
	uint8_t vlen;
};

int strncmp_PP(PGM_P s1, PGM_P s2, size_t n) {

	if ( n == 0 ) return 0;

	while ( n-- )
		if ( pgm_read_byte(s1++) != pgm_read_byte(s2++) ) return -1;

return 0;
}

char* iptoa( char *buf, uint32_t ip ) {

	uint8_t i = 0;
	
	for ( uint8_t b = 0 ; b < 4 ; b++ ) {

		uint8_t n = ip & 0xff;
		char tmp[3];
		uint8_t d = 0;

		do {
			tmp[d++] = '0' + (n % 10);	// n % base
			n /= 10;								// n/= base
		} while ( n > 0 ) ;
		
		while ( d > 0 ) {
			buf[i++] = tmp[--d];
		}

		buf[i++] = '.';
		ip >>= 8;
	}
	
	buf[--i] = '\0';	// clear last dot

return buf;
}

uint32_t atoip( char *data ) {
	uint32_t ip = 0;
	
	for ( uint8_t b = 0 ; b < 4 ; b++ ) {

		uint8_t n = 0;

		while ( *data >= '0' && *data <= '9' ) {
			n = n*10 + (*data-'0');
			data++;
		}

		if ( *data == '.' ) data++;

		ip = (ip << 8) | n;
	}

return htonl(ip);
}

uint16_t findvar( char *data ) {

	char *st = data;

	while ( *data != '\0' || *data != ' ' || *data != '\n' ) {
	
		if ( *data == '=' || *data == '&' || *data == ' ' ) return data - st;

		data++;
	}

return 0;
}

void ParseRequests( struct httpreq *req, uint8_t req_num, char *data ) {

	uint16_t varlen;
	
	// clean up
	for ( uint8_t r = 0 ; r < req_num ; r++ ) {
		req[r].value = NULL;
		req[r].vlen = 0;
	}

	for ( uint8_t i = 0 ; i < req_num ; i++ ) {

		if ( !(varlen = findvar(data)) ) break;
		
		uint8_t r = 0;
		for ( ; r < req_num ; r++ )
			if ( !strncmp_P(data,req[r].name,varlen) ) break;

		data += varlen+1;
		varlen = findvar(data);

		// no match
		if ( r == req_num ) {
			data += varlen+1;
			continue;
		}

		req[r].value = data;
		req[r].vlen = varlen;
		data += varlen;

		if ( !(*data) || *data == ' ' ) break;
		data++;
	}
}

void psend( char *pdata, uint16_t plen ) {

	uint16_t bsent = 0;
	uint16_t i;
	
	while ( plen ) {
	
		for ( i = 0 ; i < plen ; i++ )
			if ( pgm_read_byte(pdata+i) == '{' ) break;

		while ( i ) {

			bsent = sock.write( (uint8_t*)pdata, i, ASOCKET_MORE_DATA|ASOCKET_PGM_DATA );

			pdata += bsent;
			plen -= bsent;
			i -= bsent;
		}

		if ( plen ) {
			pdata++;
			plen--;
			
			char buf[20];
			uint8_t *data = NULL;

			if ( !strncmp_PP(pdata, PSTR("IP}"), 3) ) {
				data = (uint8_t*)iptoa(buf,htonl(ipaddr));
			} else if ( !strncmp_PP(pdata, PSTR("M}"), 2) ) {
				data = (uint8_t*)utoa(mask, buf, 10);
			} else if ( !strncmp_PP(pdata, PSTR("GW}"), 3) ) {
				data = (uint8_t*)iptoa(buf,htonl(defgw));
			}

			if ( data ) {
				sock.write( data, strlen((char*)data), ASOCKET_MORE_DATA );
				while ( plen-- && pgm_read_byte(pdata++) != '}' ) ;
			}
		}
	}
}

void setup() {

#ifdef __DBG__
	// set serial for debug
	Serial.begin(115200);
	Serial.println("Ready");
#endif

	// setup hardware spi interface
	pinMode(SPI_SCK_PIN, OUTPUT);
	pinMode(SPI_MOSI_PIN, OUTPUT);
	pinMode(SPI_MISO_PIN, INPUT);

	pinMode(SPI_SS0_PIN, OUTPUT);
	pinMode(SPI_SS1_PIN, OUTPUT);
	pinMode(SPI_SS2_PIN, OUTPUT);

	spiSelectDev(SPI_DEV_NONE);

	// initialize spi
	char b;
	SPCR = (1<<SPE) | (1<<MSTR);
//	SPSR |= (1<<SPI2X);
	b = SPSR;
	b = SPDR;

	// initialize enc280j60 microchip
	enc28j60Init(hwaddr);
	delay(10);

	// on and off leds
	enc28j60PhyWrite(PHLCON,0x880);
	delay(500);
	enc28j60PhyWrite(PHLCON,0x990);
	delay(500);
	enc28j60PhyWrite(PHLCON,0x880);
	delay(500);
	enc28j60PhyWrite(PHLCON,0x990);
	delay(500);

	enc28j60PhyWrite(PHLCON,0x476);
	
	#ifdef __DBG__
	Serial.print("enc28j60 rev:0x");
	Serial.println(enc28j60getrev(),HEX);
	#endif
}

void loop() {

	uint16_t datasize;
	uint8_t *data;
	
	struct httpreq request[] = {
			{ PSTR("pass"), NULL, 0 },
			{ PSTR("ip"), NULL, 0 },
			{ PSTR("mask"), NULL, 0 },
			{ PSTR("gw"), NULL, 0 }
	};

#ifdef __DBG__
	Serial.print("Connecting...");
#endif

	// initialize aSocket
	sock.setup(htonl(ipaddr),hwaddr,mask,htonl(defgw));

	uint32_t clientip;
	if ( (clientip = sock.listen( ntohs(80), IPPROTO_TCP )) != INADDR_NONE ) {
	
		auth = 0;
	
		if ( clientip == authip && (millis() - authtime) < 1000*180 )
			auth = 1;

#ifdef __DBG__
		Serial.println("OK");
#endif

		while ( sock.available() < 16 && sock.state() != ASOCK_CLOSED ) ;

		if ( sock.state() != ASOCK_CLOSED ) {

			datasize = ASOCKET_BUFSIZE;
			data = sock.read( &datasize );

#ifdef __DBG__
			Serial.println( (char*)data );
#endif

			if ( !strncmp_P((char*)data, PSTR("GET "), 4) ) {
				data += 4;
				// got http request

				static char head[] PROGMEM = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<html><center>" \
							"<style type=\"text/css\">a { color: black;text-decoration: none }</style>" \
							"<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"600\" height=\"200\"><tr>" \
							"<td width=\"80\" valign=\"center\" align=\"left\">" \
							"<a href=\"/\">[Main page ]</a><br><a href=\"/setup.html\">[Setup page]</a></td>" \
							"<td width=\"520\" valign=\"center\" align=\"left\" style=\"border-left: 1px solid black; padding: 5px;\">";
				sock.write( (uint8_t*)head, sizeof(head)-1, ASOCKET_PGM_DATA|ASOCKET_MORE_DATA );

				if ( !strncmp_P((char*)data,PSTR("/ "),2) ) {

					// index page
					static char pindex[] PROGMEM = "Index...<br><br>";
					sock.write( (uint8_t*)pindex, sizeof(pindex)-1, ASOCKET_PGM_DATA|ASOCKET_MORE_DATA );

				}

				else if ( !strncmp_P((char*)data,PSTR("/setup.html"),11) ) {

					if ( !auth ) {
						static char login[] PROGMEM = "<FORM ACTION=\"/\" METHOD=\"GET\" name=\"form\">" \
									"<H2>Please login</H2>" \
									"Pass  <INPUT NAME=\"pass\" TYPE=\"password\"><br><br>" \
									"<INPUT TYPE=\"SUBMIT\" value=\"Login\">  <INPUT TYPE=\"RESET\" value=\"Clear\"></FORM>";
						sock.write( (uint8_t*)login, sizeof(login)-1, ASOCKET_PGM_DATA|ASOCKET_MORE_DATA );
					} else {
						static char setup[] PROGMEM = "<FORM ACTION=\"/\" METHOD=\"GET\" name=\"form\">" \
									"<H2>Setup</H2>" \
									"<table><tr><td style=\"text-align:right\">IP address: </td>" \
									"<td style=\"text-align:left\"><INPUT NAME=\"ip\" TYPE=\"text\" size=\"16\" value=\"{IP}\">" \
									" / <INPUT NAME=\"mask\" TYPE=\"text\" size=\"2\" value=\"{M}\"" \
									"></td></tr><tr><td style=\"text-align:right\">Getway IP: </td><td style=\"text-align:left\">" \
									"<INPUT NAME=\"gw\" TYPE=\"text\" size=\"16\" value=\"{GW}\">" \
									"</td></tr><tr><td style=\"text-align:right\">" \
									"Pass: </td><td style=\"text-align:left\"><INPUT NAME=\"pass\" TYPE=\"password\"></td></tr></table>" \
									"<br><INPUT TYPE=\"SUBMIT\" value=\"Save\">  <INPUT TYPE=\"RESET\" value=\"Clear\">" \
									"</FORM>";
						psend( setup, sizeof(setup)-1 );
					}

				}

				else if ( !strncmp_P((char*)data,PSTR("/?"),2) ) {

					ParseRequests( request, sizeof(request)/sizeof(struct httpreq), (char*)data+2 );

					if ( !auth ) {
						// login try
						if ( request[0].vlen ) {

							if ( !strncmp(pass,request[0].value,request[0].vlen) || strlen(pass) == 0 ) {
								authtime = millis();
								authip = clientip;
							}
						}
					} else {
					
					// save configuration
						if ( request[1].vlen && request[2].vlen && request[3].vlen ) {
							ipaddr = ntohl(atoip(request[1].value));
							mask = atoi(request[2].value);
							defgw = ntohl(atoip(request[3].value));
						}

						if ( request[0].vlen ) {
							if ( request[0].vlen > sizeof(pass)-1 ) request[0].vlen = sizeof(pass)-1;

							for ( uint8_t i = 0; i < request[0].vlen ; i++ )
								pass[i] = request[0].value[i];

							pass[request[0].vlen] = '\0';
						}
						
						// TODO: save to eeprom
					}

					// refresh page
					static char refresh[] PROGMEM = "<meta http-equiv=\"refresh\" content=\"5;url=http://{IP}/setup.html\">Reconnecting to {IP}";
					psend( refresh, sizeof(refresh)-1 );
				}

				static char tail[] PROGMEM = "</td></tr></table><hr width=\"600\"><small><i>Adrian Brzezinski (c) 2010</i></small></center></html>";
				sock.write( (uint8_t*)tail, sizeof(tail)-1, ASOCKET_PGM_DATA );
				sock.close();
			}
		}

#ifdef __DBG__
		Serial.println();
		Serial.println("Connection closed.");
#endif
	}
}

