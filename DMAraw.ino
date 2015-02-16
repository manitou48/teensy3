//  try FIFO ?   DMA ?  16-bit
// MOSI MISO CLK CS   11 12 13  10
// see fastLED
#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

#define CS 10

#define SHORTS 500
#define BYTES SHORTS*2
uint16_t shorts[SHORTS];
uint8_t *bytes = (uint8_t *) shorts;

void spi_begin() {
	uint32_t sim4 = SIM_SCGC4;
	if (!(sim4 & SIM_SCGC4_SPI0)) SIM_SCGC4 = sim4 | SIM_SCGC4_SPI0;
	SPI0_C1 = SPI_C1_SPE | SPI_C1_MSTR;   // enable master MODEn 
	SPI0_C2 = 0;
	SPI0_BR =  SPI_BR_SPPR(2) | SPI_BR_SPR(0); // baud
	uint8_t tmp __attribute__((unused)) = SPI0_S;
	// enable pins
	CORE_PIN11_CONFIG = PORT_PCR_MUX(2);  // MOSI
	CORE_PIN12_CONFIG = PORT_PCR_MUX(2);  // MISO
	CORE_PIN13_CONFIG = PORT_PCR_MUX(2); // CLK
    // DMA
    SIM_SCGC7 |= SIM_SCGC7_DMA;   //Enable clock to the DMA module
    SIM_SCGC6 |= SIM_SCGC6_DMAMUX;  // clock to the DMAMUX module
    DMAMUX0_CHCFG0 =  0;
    DMAMUX0_CHCFG0 =  DMAMUX_ENABLE | DMAMUX_SOURCE_SPI0_TX;
}

// from libraries/SPI KINETISL
void spi_transfer(void *buf, size_t count) {
        if (count == 0) return;
        uint8_t *p = (uint8_t *)buf;
        while (!(SPI0_S & SPI_S_SPTEF)) ; // wait
        SPI0_DL = *p;
        while (--count > 0) {
            uint8_t out = *(p + 1);
            while (!(SPI0_S & SPI_S_SPTEF)) ; // wait
            __disable_irq();
            SPI0_DL = out;
            while (!(SPI0_S & SPI_S_SPRF)) ; // wait
            uint8_t in = SPI0_DL;
            __enable_irq();
            *p++ = in;
        }
        while (!(SPI0_S & SPI_S_SPRF)) ; // wait
        *p = SPI0_DL;
}

// from FastLED
void writeBytes(register uint8_t *data, int len) {
    while(len--) {
        while (!(SPI0_S & SPI_S_SPTEF)) ; // wait
        SPI0_DL = *data++;
    }
}


void write16s(uint16_t *data, int len) {
    SPI0_C2 = SPI_C2_SPIMODE;
    while(len--) {
        SPI0_DH = (*data) >> 8;
        SPI0_DL =  *data++;
        while (!(SPI0_S & SPI_S_SPTEF)) ; // wait
    }
}

void writeDMA8(uint8_t *data, int len) {
    SPI0_C1 = 0; // SPI reset
    SPI0_C2 |= SPI_C2_TXDMAE;  // xmit DMA
    SPI0_C1 = SPI_C1_SPE | SPI_C1_MSTR;   // enable
    DMA_DSR_BCR0 = DMA_DSR_BCR_DONE ;  // reset
    DMA_SAR0 = data;
    DMA_DAR0 = &SPI0_DL;
    DMA_DSR_BCR0 =  len;
//  DMA_DCR0 = /*DMA_DCR_CS |*/ DMA_DCR_ERQ | DMA_DCR_D_REQ | DMA_DCR_SINC | DMA_DCR_SSIZE(1) | DMA_DCR_DSIZE(1) ; //| DMA_DCR_START;
    DMA_DCR0 =  DMA_DCR_CS | DMA_DCR_D_REQ | DMA_DCR_SINC | DMA_DCR_SSIZE(1) | DMA_DCR_DSIZE(1) ;
#if 0
   PRREG(SPI0_BR);
       PRREG(SPI0_C1);
    PRREG(SPI0_C2);
    PRREG(SPI0_S);
    PRREG(DMA_DSR_BCR0);
    PRREG(DMA_DCR0);
#endif
     DMA_DCR0 |= DMA_DCR_ERQ;   // enable
    // DMA_DCR0 |= DMA_DCR_START;
    //while (!(DMA_DSR_BCR0 & DMA_DSR_BCR_DONE)) /* wait */ ;
    while(DMA_DCR0 & DMA_DCR_ERQ); // wait for BCR to zero
}



void setup() {
	int i;

	Serial.begin(9600);
	while(!Serial);
	delay(4000);
	pinMode(CS,OUTPUT);
	digitalWrite(CS,HIGH);
	for (i=0;i<BYTES;i++) bytes[i]=i;
	spi_begin();
}

void loop() {
	uint32_t t1;
	double mbs;

	digitalWrite(CS,LOW);
	t1 = micros();
//	spi_transfer(bytes,BYTES);
//  writeBytes(bytes,BYTES);
//  write16s(shorts,SHORTS);
   writeDMA8(bytes,BYTES);
	t1 = micros() - t1;
	digitalWrite(CS,HIGH);
	mbs = 8*BYTES/(float)t1;
	Serial.print(t1);   
	Serial.print(" us mbs: ");Serial.println(mbs,2);
	delay(3000);
}
