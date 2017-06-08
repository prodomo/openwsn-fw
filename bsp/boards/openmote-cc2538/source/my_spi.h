
#ifndef __MY_SPI_H__
#define __MY_SPI_H__

#include "openserial.h"
#include "cc1200_spi.h"

void my_SPI_init(void);
//int my_SPI_send(uint16_t);
uint8_t my_SPI_send(uint16_t addr, uint8_t data);
uint8_t my_SPI_recv(uint16_t addr);
void trxSpiCmdStrobe(uint8_t ui8Addr, uint8_t ui8Len);
void CC1200WriteReg(uint8_t ui8Addr, const uint8_t *pui8Buf, uint8_t ui8Len);

#endif
