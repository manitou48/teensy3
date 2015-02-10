// LC spiperf  for SPI1
//  SPI1 can do 24mhz  (speed affected by F_CPU)
// does 8-bit, 16-bit, FIFO,  DMA ?
//  try   fifo+dma+16-bit?
// MOSI1 MISO1 CLK1 CS1   0 1 20  6
// see fastLED  libraries/SPI

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

#define CS 6

#define SHORTS 500
#define BYTES SHORTS*2
uint16_t shorts[SHORTS];
uint8_t *bytes = (uint8_t *) shorts;

void spi1_begin() {
	uint32_t sim4 = SIM_SCGC4;
	if (!(sim4 & SIM_SCGC4_SPI1)) SIM_SCGC4 = sim4 | SIM_SCGC4_SPI1;
	SPI1_C1 = SPI_C1_SPE | SPI_C1_MSTR;   // MODEn 
	SPI1_C2 = 0;
	SPI1_BR =  SPI_BR_SPPR(0) | SPI_BR_SPR(0); // SPI CLK speed
	uint8_t tmp __attribute__((unused)) = SPI1_S;
	// enable pins
	CORE_PIN0_CONFIG = PORT_PCR_MUX(2);  // MOSI
	CORE_PIN1_CONFIG = PORT_PCR_MUX(2);  // MISO
	CORE_PIN20_CONFIG = PORT_PCR_MUX(2); // CLK
}

// from libraries/SPI KINETISL
void spi1_transfer(void *buf, size_t count) {
        if (count == 0) return;
        uint8_t *p = (uint8_t *)buf;
        while (!(SPI1_S & SPI_S_SPTEF)) ; // wait
        SPI1_DL = *p;
        while (--count > 0) {
            uint8_t out = *(p + 1);
            while (!(SPI1_S & SPI_S_SPTEF)) ; // wait
            __disable_irq();
            SPI1_DL = out;
            while (!(SPI1_S & SPI_S_SPRF)) ; // wait
            uint8_t in = SPI1_DL;
            __enable_irq();
            *p++ = in;
        }
        while (!(SPI1_S & SPI_S_SPRF)) ; // wait
        *p = SPI1_DL;
}

// from FastLED
void writeBytes(register uint8_t *data, int len) {
	while(len--) {
		while (!(SPI1_S & SPI_S_SPTEF)) ; // wait
		SPI1_DL = *data++;
	}
}

void write16s(uint16_t *data, int len) {
	SPI1_C2 = SPI_C2_SPIMODE;
	while(len--) {
		SPI1_DH = (*data) >> 8;
		SPI1_DL =  *data++;
		while (!(SPI1_S & SPI_S_SPTEF)) ; // wait
	}
}

void writeFIFO(register uint8_t *data, int len) {
	SPI1_C3 = SPI_C3_FIFOMODE;
	while(len--) {
		while (!(SPI1_S & SPI_S_TNEAREF)) ; // wait til space in FIFO
		SPI1_DL = *data++;
	}
}

inline static uint16_t transfer16(uint16_t data) {
		SPI1_C2 = SPI_C2_SPIMODE;
		SPI1_DH = data >> 8;
		SPI1_DL = data;
		while (!(SPI1_S & SPI_S_SPRF)) ; // wait
		uint16_t r = (SPI1_DH << 8) | SPI1_DL;
		SPI1_C2 = 0;
		return r;
}

  // DMA  TODO   need 2nd DMA channel for input??
  // see paul's DMAChannel.h
void spiwriteDMA(uint8_t *data, int len) {
	SPI1_C1 = 0; // SPI reset
	SPI1_C2 |= SPI_C2_TXDMAE;  // xmit DMA
	SPI1_C1 = SPI_C1_SPE | SPI_C1_MSTR;   // enable 
	DMA_SAR0 = data;
	DMA_DAR0 = &SPI1_DL;
	DMA_DSR_BCR0 = DMA_DSR_BCR_DONE ;  // reset
	DMA_DSR_BCR0 =  len;
//	DMA_DCR0 = /*DMA_DCR_CS |*/ DMA_DCR_ERQ | DMA_DCR_D_REQ | DMA_DCR_SINC | DMA_DCR_SSIZE(1) | DMA_DCR_DSIZE(1) ; //| DMA_DCR_START;
	DMA_DCR0 =  DMA_DCR_D_REQ | DMA_DCR_SINC | DMA_DCR_SSIZE(1) | DMA_DCR_DSIZE(1) ; 
	 DMA_DCR0 |= DMA_DCR_START;
#if 0
	PRREG(SPI1_C1);
	PRREG(SPI1_C2);
	PRREG(SPI1_S);
	PRREG(DMA_DSR_BCR0);
	PRREG(DMA_DCR0);
#endif
	while (!(DMA_DSR_BCR0 & DMA_DSR_BCR_DONE)) /* wait */ ;
//	while(DMA_DCR0 & DMA_DCR_ERQ); // wait for BCR to zero
}

void setup() {
	int i;

	Serial.begin(9600);
	while(!Serial);
	delay(4000);
	pinMode(CS,OUTPUT);
	digitalWrite(CS,HIGH);
	for (i=0;i<BYTES;i++) bytes[i]=i;
	spi1_begin();
}

void loop() {
	uint32_t t1;
	double mbs;

	digitalWrite(CS,LOW);
	t1 = micros();
	spi1_transfer(bytes,BYTES);
//	writeBytes(bytes,BYTES);
//	writeFIFO(bytes,BYTES);
//	write16s(shorts,SHORTS);
//	spiwriteDMA(bytes,BYTES);   // TODO
	t1 = micros() - t1;
	digitalWrite(CS,HIGH);
	mbs = 8*BYTES/(float)t1;
	Serial.print(t1);   
	Serial.print(" us mbs: ");Serial.println(mbs,2);
	delay(3000);
}
