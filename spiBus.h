
/*

  ------------------------------------------------------------------------------
      spiBus.h, SPI Bus with multiple slave devices implementation
  ------------------------------------------------------------------------------

    Author: Adrian Brzezinski <iz0@poczta.onet.pl> (C)2010
	Copyright: GPL V2 (http://www.gnu.org/licenses/gpl.html)

*/

#ifndef __SPIBUS_H__
#define __SPIBUS_H__

#define SPI_SCK_PIN 		13
#define SPI_MISO_PIN		12
#define SPI_MOSI_PIN		11

#define SPI_SS0_PIN			10
#define SPI_SS1_PIN			9
#define SPI_SS2_PIN			8

#define SPI_DEV_ETH		0
#define SPI_DEV_RAM		2
#define SPI_DEV_NONE		7

#define spiSelectDev(addr)	( PORTB = (PORTB & 0xf8) | (addr & 0x07) )

#define memoff(addr, type, mem) (uint16_t)((uint8_t*)&((type*)(addr))->mem)

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

#define _STACK_PTR				((SPH << 8) | SPL)
#define _STACK_SIZE			(RAMEND-_STACK_PTR)
#define _HEAP_START_PTR	((uint16_t)&__heap_start)
#define _HEAP_END_PTR		((uint16_t)__brkval ? (uint16_t)__brkval : (uint16_t)&__bss_end)
#define _HEAP_SIZE				(_HEAP_END_PTR-_HEAP_START_PTR)
#define _FREE_MEM				(_STACK_PTR-_HEAP_END_PTR)

#define _FUNCTION_DBG_INFO_ Serial.print(__FUNCTION__); \
											Serial.print(" SP: "); Serial.print( _STACK_PTR , HEX);		\
											Serial.print(" SS: "); Serial.print( _STACK_SIZE , HEX);	\
											Serial.print(" HSP: "); Serial.print( _HEAP_START_PTR , HEX);	\
											Serial.print(" HEP: "); Serial.print( _HEAP_END_PTR , HEX);	\
											Serial.print(" HS: "); Serial.print( _HEAP_SIZE , HEX);	\
											Serial.print(" MEM: "); Serial.println( _FREE_MEM , HEX);		\

#endif /* __SPIBUS_H__ */