// use SdFat SPI functions for unconnected test, edit ctar each time

#define CS 10

#define SPI_BUFF_SIZE 1000
uint8_t rx_buffer[SPI_BUFF_SIZE];
uint8_t tx_buffer[SPI_BUFF_SIZE];

void setup() {
	Serial.begin(9600);
	pinMode(CS,OUTPUT);
	digitalWrite(CS,HIGH);
	spiBegin();
	spiInit(0);
}

void loop() {
	uint32_t t1;
	double mbs;
	char str[64];

	digitalWrite(CS,LOW);
	t1 = micros();
	spiSend(tx_buffer,SPI_BUFF_SIZE);
	t1 = micros() - t1;
	digitalWrite(CS,HIGH);
	mbs = 8*SPI_BUFF_SIZE/(float)t1;
	sprintf(str,"%d us  %.2f mbs %0x",t1,mbs,SPI0_CTAR0);
	Serial.println(str);
	delay(3000);
}


// Teensy 3.0 functions
#include <mk20dx128.h>
// use 16-bit frame if SPI_USE_8BIT_FRAME is zero
#define SPI_USE_8BIT_FRAME 0
// Limit initial fifo to three entries to avoid fifo overrun
#define SPI_INITIAL_FIFO_DEPTH 3
// define some symbols that are not in mk20dx128.h
#ifndef SPI_SR_RXCTR
#define SPI_SR_RXCTR 0XF0
#endif  // SPI_SR_RXCTR
#ifndef SPI_PUSHR_CONT
#define SPI_PUSHR_CONT 0X80000000
#endif   // SPI_PUSHR_CONT
#ifndef SPI_PUSHR_CTAS
#define SPI_PUSHR_CTAS(n) (((n) & 7) << 28)
#endif  // SPI_PUSHR_CTAS

//------------------------------------------------------------------------------
/**
 * initialize SPI pins
 */
static void spiBegin() {
  SIM_SCGC6 |= SIM_SCGC6_SPI0;
}
//------------------------------------------------------------------------------
/**
 * Initialize hardware SPI
 * Set SCK rate to F_CPU/pow(2, 1 + spiRate) for spiRate [0,6]
 */
static void spiInit(uint8_t spiRate) {
  // spiRate = 0 or 1 : 24 or 12 Mbit/sec
  // spiRate = 2 or 3 : 12 or 6 Mbit/sec
  // spiRate = 4 or 5 : 6 or 3 Mbit/sec
  // spiRate = 6 or 7 : 3 or 1.5 Mbit/sec    thd  4mbs
  // spiRate = 8 or 9 : 1.5 or 0.75 Mbit/sec
  // spiRate = 10 or 11 : 250 kbit/sec
  // spiRate = 12 or more : 125 kbit/sec
  uint32_t ctar, ctar0, ctar1;
  switch (spiRate/2) {
    case 0: ctar = SPI_CTAR_DBR | SPI_CTAR_BR(0) | SPI_CTAR_CSSCK(0); break;
    case 1: ctar = SPI_CTAR_BR(0) | SPI_CTAR_CSSCK(0); break;
    case 2: ctar = SPI_CTAR_BR(1) | SPI_CTAR_CSSCK(1); break;
    case 3: ctar = SPI_CTAR_BR(2) | SPI_CTAR_CSSCK(2); break;
    case 4: ctar = SPI_CTAR_BR(3) | SPI_CTAR_CSSCK(3); break;
#if F_BUS == 48000000
    case 5: ctar = SPI_CTAR_PBR(1) | SPI_CTAR_BR(5) | SPI_CTAR_CSSCK(5); break;
    default: ctar = SPI_CTAR_PBR(1) | SPI_CTAR_BR(6) | SPI_CTAR_CSSCK(6);
#elif F_BUS == 24000000
    case 5: ctar = SPI_CTAR_PBR(1) | SPI_CTAR_BR(4) | SPI_CTAR_CSSCK(4); break;
    default: ctar = SPI_CTAR_PBR(1) | SPI_CTAR_BR(5) | SPI_CTAR_CSSCK(5);
#else
#error "MK20DX128 bus frequency must be 48 or 24 MHz"
#endif
  }

  // thd divisors of 48: BR 2,4,6,8,16...  PBR 2,3,5,7    DBR *2
  //ctar = SPI_CTAR_DBR | SPI_CTAR_BR(0) | SPI_CTAR_PBR(1);  // thd
  //ctar =  SPI_CTAR_BR(3) | SPI_CTAR_PBR(1);  // thd
  //ctar = SPI_CTAR_BR(2) | SPI_CTAR_CSSCK(2);  // thd 4 mbs
  //ctar = SPI_CTAR_PBR(1) | SPI_CTAR_CSSCK(1);  // thd 4 mbs

  // CTAR0 - 8 bit transfer
  ctar0 = ctar | SPI_CTAR_FMSZ(7);
  // CTAR1 - 16 bit transfer
  ctar1 = ctar | SPI_CTAR_FMSZ(15);

  if (SPI0_CTAR0 != ctar0 || SPI0_CTAR1 != ctar1 ) {
    SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_MDIS | SPI_MCR_HALT;
    SPI0_CTAR0 = ctar0;
    SPI0_CTAR1 = ctar1;
  }
  SPI0_MCR = SPI_MCR_MSTR;
  CORE_PIN11_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2);
  CORE_PIN12_CONFIG = PORT_PCR_MUX(2);
  CORE_PIN13_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2);
}
//------------------------------------------------------------------------------
/** SPI receive a byte */
static  uint8_t spiRec() {
  SPI0_MCR |= SPI_MCR_CLR_RXF;
  SPI0_SR = SPI_SR_TCF;
  SPI0_PUSHR = 0xFF;
  while (!(SPI0_SR & SPI_SR_TCF)) {}
  return SPI0_POPR;
}
//------------------------------------------------------------------------------
/** SPI receive multiple bytes */
static uint8_t spiRec(uint8_t* buf, size_t len) {
  // clear any data in RX FIFO
  SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_CLR_RXF;
#if SPI_USE_8BIT_FRAME
  // initial number of bytes to push into TX FIFO
  int nf = len < SPI_INITIAL_FIFO_DEPTH ? len : SPI_INITIAL_FIFO_DEPTH;
  for (int i = 0; i < nf; i++) {
    SPI0_PUSHR = 0XFF;
  }
  // limit for pushing dummy data into TX FIFO
  uint8_t* limit = buf + len - nf;
  while (buf < limit) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    SPI0_PUSHR = 0XFF;
    *buf++ = SPI0_POPR;
  }
  // limit for rest of RX data
  limit += nf;
  while (buf < limit) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    *buf++ = SPI0_POPR;
  }
#else  // SPI_USE_8BIT_FRAME
  // use 16 bit frame to avoid TD delay between frames
  // get one byte if len is odd
  if (len & 1) {
    *buf++ = spiRec();
    len--;
  }
  // initial number of words to push into TX FIFO
  int nf = len/2 < SPI_INITIAL_FIFO_DEPTH ? len/2 : SPI_INITIAL_FIFO_DEPTH;
  for (int i = 0; i < nf; i++) {
    SPI0_PUSHR = SPI_PUSHR_CONT | SPI_PUSHR_CTAS(1) | 0XFFFF;
  }
  uint8_t* limit = buf + len - 2*nf;
  while (buf < limit) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    SPI0_PUSHR = SPI_PUSHR_CONT | SPI_PUSHR_CTAS(1) | 0XFFFF;
    uint16_t w = SPI0_POPR;
    *buf++ = w >> 8;
    *buf++ = w & 0XFF;
  }
  // limit for rest of RX data
  limit += 2*nf;
  while (buf < limit) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    uint16_t w = SPI0_POPR;
    *buf++ = w >> 8;
    *buf++ = w & 0XFF;
  }
#endif  // SPI_USE_8BIT_FRAME
  return 0;
}
//------------------------------------------------------------------------------
/** SPI send a byte */
static void spiSend(uint8_t b) {
  SPI0_MCR |= SPI_MCR_CLR_RXF;
  SPI0_SR = SPI_SR_TCF;
  SPI0_PUSHR = b;
  while (!(SPI0_SR & SPI_SR_TCF)) {}
}
//------------------------------------------------------------------------------
/** SPI send multiple bytes */
static void spiSend(const uint8_t* output, size_t len) {
  // clear any data in RX FIFO
  SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_CLR_RXF;
#if SPI_USE_8BIT_FRAME
  // initial number of bytes to push into TX FIFO
  int nf = len < SPI_INITIAL_FIFO_DEPTH ? len : SPI_INITIAL_FIFO_DEPTH;
  // limit for pushing data into TX fifo
  const uint8_t* limit = output + len;
  for (int i = 0; i < nf; i++) {
    SPI0_PUSHR = *output++;
  }
  // write data to TX FIFO
  while (output < limit) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    SPI0_PUSHR = *output++;
    SPI0_POPR;
  }
  // wait for data to be sent
  while (nf) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    SPI0_POPR;
    nf--;
  }
#else  // SPI_USE_8BIT_FRAME
  // use 16 bit frame to avoid TD delay between frames
  // send one byte if len is odd
  if (len & 1) {
    spiSend(*output++);
    len--;
  }
  // initial number of words to push into TX FIFO
  int nf = len/2 < SPI_INITIAL_FIFO_DEPTH ? len/2 : SPI_INITIAL_FIFO_DEPTH;
  // limit for pushing data into TX fifo
  const uint8_t* limit = output + len;
  for (int i = 0; i < nf; i++) {
    uint16_t w = (*output++) << 8;
    w |= *output++;
    SPI0_PUSHR = SPI_PUSHR_CONT | SPI_PUSHR_CTAS(1) | w;
  }
  // write data to TX FIFO
  while (output < limit) {
    uint16_t w = *output++ << 8;
    w |= *output++;
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    SPI0_PUSHR = SPI_PUSHR_CONT | SPI_PUSHR_CTAS(1) | w;
    SPI0_POPR;
  }
  // wait for data to be sent
  while (nf) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    SPI0_POPR;
    nf--;
  }
#endif  // SPI_USE_8BIT_FRAME
}
